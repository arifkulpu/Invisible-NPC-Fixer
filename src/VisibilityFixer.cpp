#include <SKSE/SKSE.h>
#include "VisibilityFixer.h"
#include "Settings.h"

#include <RE/A/Actor.h>
#include <RE/P/ProcessLists.h>
#include <RE/M/Main.h>
#include <RE/N/NiNode.h>
#include <RE/T/TESObjectREFR.h>
#include <RE/P/PlayerCharacter.h>
#include <RE/T/TESDataHandler.h>
#include <RE/U/UI.h>
#include <REL/Relocation.h>
#include <SKSE/Trampoline.h>

#include <unordered_map>
#include <unordered_set>

using namespace std::string_literals;

namespace VisibilityFixer
{
    // Cooldown map: FormID -> Last Fix Time
    std::unordered_map<RE::FormID, float> fixCooldowns;

    static float updateTimer  = 0.0f;
    static float globalTime   = 0.0f;

    // -----------------------------------------------------------------------
    // Mod Presence Detection
    // -----------------------------------------------------------------------

    /// Returns true if a plugin (ESP/ESM/ESL) with the given name is loaded.
    static bool IsPluginLoaded(std::string_view pluginName)
    {
        auto* handler = RE::TESDataHandler::GetSingleton();
        if (!handler) return false;
        return handler->LookupModByName(pluginName) != nullptr;
    }

    /// Cached flags so we don't call LookupModByName every frame.
    static bool s_ostimPresent        = false;
    static bool s_sexlabPresent       = false;
    static bool s_furnitureModPresent = false;
    static bool s_nffPresent          = false;
    static bool s_aftPresent          = false;
    static bool s_effPresent          = false;
    static bool s_presenceChecked     = false;

    static void CheckModPresence()
    {
        if (s_presenceChecked) return;
        s_presenceChecked = true;

        // OStim — main plugin and NG variant
        s_ostimPresent = IsPluginLoaded("OStim.esp") ||
                         IsPluginLoaded("OStimNG.esp") ||
                         IsPluginLoaded("OSex.esp");

        // SexLab
        s_sexlabPresent = IsPluginLoaded("SexLab.esm") ||
                          IsPluginLoaded("SexLabFramework.esm");

        // Furniture animation mods that temporarily make actors "invisible"
        s_furnitureModPresent = IsPluginLoaded("AnimatedFurnitureAIO.esp") ||
                                IsPluginLoaded("SexLabFurnitures.esp");

        // Follower managers
        s_nffPresent = IsPluginLoaded("Nether_Follower_Framework.esp") ||
                       IsPluginLoaded("NetherFollowerFramework.esp");

        s_aftPresent = IsPluginLoaded("AmazingFollowerTweaks.esp") ||
                       IsPluginLoaded("AFT.esp");

        s_effPresent = IsPluginLoaded("EFF.esp") ||
                       IsPluginLoaded("EnhancedFollowerFramework.esp");

        SKSE::log::info("[ModDetect] OStim={} SexLab={} FurnitureMod={} NFF={} AFT={} EFF={}",
            s_ostimPresent, s_sexlabPresent, s_furnitureModPresent,
            s_nffPresent, s_aftPresent, s_effPresent);
    }

    // -----------------------------------------------------------------------
    // Scene / Animation State Detection
    // -----------------------------------------------------------------------

    /// Check if an actor is currently in an OStim / SexLab animation scene.
    /// We do this by inspecting the actor's keyword list for known scene keywords.
    static bool IsActorInSexScene(RE::Actor* a_actor)
    {
        if (!a_actor) return false;

        // OStim attaches a magic effect / keyword during a scene.
        // FormIDs vary by version, so we use keyword strings instead.
        static constexpr std::array<std::string_view, 5> sceneKeywords = {
            "OStimScene",
            "SexLabScene",
            "SexLabAnimating",
            "OSexScene",
            "IsAnimating"
        };

        auto* keywords = a_actor->As<RE::BGSKeywordForm>();
        if (!keywords) return false;

        for (const auto& kw : sceneKeywords) {
            if (keywords->HasKeywordString(kw)) {
                return true;
            }
        }
        return false;
    }


    // -----------------------------------------------------------------------
    // Follower Faction Detection
    // -----------------------------------------------------------------------

    /// NFF, AFT, and EFF all use dedicated follower factions to track managed
    /// followers.  We resolve those faction FormIDs once after data is loaded
    /// so the per-actor check is O(1).
    static RE::TESFaction* s_nffFaction = nullptr;
    static RE::TESFaction* s_aftFaction = nullptr;
    static RE::TESFaction* s_effFaction = nullptr;

    static void ResolveFollowerFactions()
    {
        auto* handler = RE::TESDataHandler::GetSingleton();
        if (!handler) return;

        // NFF — "PlayerFollowerFaction" in Nether_Follower_Framework.esp
        if (s_nffPresent) {
            // NFF uses the vanilla CurrentFollowerFaction (0x0005C84E) plus its
            // own managed faction.  We look up the vanilla one as a safe proxy.
            s_nffFaction = handler->LookupForm<RE::TESFaction>(0x0005C84E, "Skyrim.esm");
        }

        // AFT stores followers in its own faction — FormID 0x020084D2 (AFT.esp)
        if (s_aftPresent) {
            s_aftFaction = handler->LookupForm<RE::TESFaction>(0x0084D2, "AmazingFollowerTweaks.esp");
            if (!s_aftFaction)
                s_aftFaction = handler->LookupForm<RE::TESFaction>(0x0084D2, "AFT.esp");
        }

        // EFF — uses CurrentFollowerFaction as well as its own registry
        if (s_effPresent) {
            s_effFaction = handler->LookupForm<RE::TESFaction>(0x0005C84E, "Skyrim.esm");
        }

        SKSE::log::info("[FollowerFactions] NFF={} AFT={} EFF={}",
            s_nffFaction != nullptr, s_aftFaction != nullptr, s_effFaction != nullptr);
    }

    /// Returns true if the actor is a follower managed by NFF / AFT / EFF.
    /// We check faction membership first; if the faction pointer could not be
    /// resolved we fall back to the vanilla CurrentFollowerFaction.
    static bool IsFollowerManaged(RE::Actor* a_actor)
    {
        if (!a_actor) return false;

        auto& cfg = Settings::GetSingleton();

        auto* handler = RE::TESDataHandler::GetSingleton();
        RE::TESFaction* vanillaFollowerFaction =
            handler ? handler->LookupForm<RE::TESFaction>(0x0005C84E, "Skyrim.esm") : nullptr;

        // --- Always check vanilla CurrentFollowerFaction first ---
        // This catches ANY follower regardless of which follower manager is used.
        // Without this, vanilla followers (no NFF/AFT/EFF) were never protected.
        if (vanillaFollowerFaction && a_actor->IsInFaction(vanillaFollowerFaction)) {
            if (cfg.verboseLogging)
                SKSE::log::debug("[Vanilla] Skipping vanilla follower {:08X}", a_actor->GetFormID());
            return true;
        }

        // --- NFF: also checks its own managed faction ---
        if (cfg.skipNFFFollowers && s_nffPresent && s_nffFaction) {
            if (a_actor->IsInFaction(s_nffFaction)) {
                if (cfg.verboseLogging)
                    SKSE::log::debug("[NFF] Skipping managed follower {:08X}", a_actor->GetFormID());
                return true;
            }
        }

        // --- AFT ---
        if (cfg.skipAFTFollowers && s_aftPresent && s_aftFaction) {
            if (a_actor->IsInFaction(s_aftFaction)) {
                if (cfg.verboseLogging)
                    SKSE::log::debug("[AFT] Skipping managed follower {:08X}", a_actor->GetFormID());
                return true;
            }
        }

        // --- EFF ---
        if (cfg.skipEFFFollowers && s_effPresent && s_effFaction) {
            if (a_actor->IsInFaction(s_effFaction)) {
                if (cfg.verboseLogging)
                    SKSE::log::debug("[EFF] Skipping managed follower {:08X}", a_actor->GetFormID());
                return true;
            }
        }

        return false;
    }

    // -----------------------------------------------------------------------
    // Core Fix Logic
    // -----------------------------------------------------------------------

    void FixInvisibleNPCs()
    {
        auto& cfg = Settings::GetSingleton();

        auto* processLists = RE::ProcessLists::GetSingleton();
        auto* player       = RE::PlayerCharacter::GetSingleton();
        if (!processLists || !player) return;

        for (auto& handle : processLists->highActorHandles) {
            auto actor = handle.get();
            if (!actor || !actor->Is3DLoaded()) continue;
            if (actor->IsPlayerRef())  continue;
            if (actor->IsDisabled() || actor->IsDead() || actor->IsDeleted()) continue;
            if ((actor->formFlags & RE::TESForm::RecordFlags::kDeleted) != 0) continue;

            // Distance check (4096 units)
            auto p1 = player->GetPosition();
            auto p2 = actor->GetPosition();
            float distSq = (p1.x-p2.x)*(p1.x-p2.x)
                         + (p1.y-p2.y)*(p1.y-p2.y)
                         + (p1.z-p2.z)*(p1.z-p2.z);
            if (distSq > (4096.0f * 4096.0f)) continue;

            // Skip actors that are in a sex/animation scene
            if ((cfg.disableWithOStim  && s_ostimPresent)  ||
                (cfg.disableWithSexLab && s_sexlabPresent)  ||
                (cfg.disableWithFurnitureMod && s_furnitureModPresent))
            {
                if (IsActorInSexScene(actor.get())) {
                    if (cfg.verboseLogging)
                        SKSE::log::debug("[Scene] Actor {:08X} is in a scene, skipping.", actor->GetFormID());
                    continue;
                }
            }

            // Skip followers managed by NFF / AFT / EFF
            if (IsFollowerManaged(actor.get())) continue;

            // The actual ghost check
            if (actor->Get3D() == nullptr) {
                auto it = fixCooldowns.find(actor->GetFormID());
                if (it != fixCooldowns.end() && globalTime - it->second < 10.0f)
                    continue;

                if (cfg.logFixes)
                    SKSE::log::info("Ghost NPC Detected: {} ({:08X}). Queueing Refresh...",
                        actor->GetName(), actor->GetFormID());

                auto* taskInterface = SKSE::GetTaskInterface();
                if (taskInterface) {
                    RE::FormID fid = actor->GetFormID();
                    taskInterface->AddTask([fid]() {
                        auto* a = RE::TESForm::LookupByID<RE::Actor>(fid);
                        if (!a || a->IsDeleted() || a->IsDisabled() || a->IsDead()) return;

                        // Re-check follower status inside the task — NPC may have
                        // joined CurrentFollowerFaction between the scan and now.
                        auto* handler = RE::TESDataHandler::GetSingleton();
                        auto* followerFaction = handler
                            ? handler->LookupForm<RE::TESFaction>(0x0005C84E, "Skyrim.esm")
                            : nullptr;
                        if (followerFaction && a->IsInFaction(followerFaction)) return;

                        auto* root = a->Get3D();
                        if (root) {
                            // 3D node exists but is culled/hidden — just unhide it.
                            // SetAppCulled does NOT touch inventory or outfit.
                            root->SetAppCulled(false);
                        }
                        // If Get3D() is still null at task time, do nothing.
                        // Calling Update3DModel() here would risk adding outfit items
                        // to the actor's inventory. The next scan cycle will retry.
                    });
                }

                fixCooldowns[actor->GetFormID()] = globalTime;
            }
        }

        // Cleanup stale cooldown entries
        if (fixCooldowns.size() > 100) {
            for (auto it = fixCooldowns.begin(); it != fixCooldowns.end(); ) {
                if (globalTime - it->second > 60.0f)
                    it = fixCooldowns.erase(it);
                else
                    ++it;
            }
        }
    }

    /// Returns true if any menu that temporarily strips NPC 3D models is open.
    /// Opening inventory/container/barter causes the engine to unload nearby
    /// actors' 3D nodes for performance — we must not scan during that window.
    static bool IsAnyMenuOpen()
    {
        auto* ui = RE::UI::GetSingleton();
        if (!ui) return false;

        // Game paused = some menu is blocking gameplay
        if (ui->GameIsPaused()) return true;

        // Explicit checks for menus that strip actor 3D
        static constexpr std::array<std::string_view, 6> dangerMenus = {
            "InventoryMenu",
            "ContainerMenu",
            "BarterMenu",
            "GiftMenu",
            "FavoritesMenu",
            "MagicMenu"
        };
        for (const auto& name : dangerMenus) {
            if (ui->IsMenuOpen(RE::BSFixedString(name))) return true;
        }
        return false;
    }

    void Update(float a_delta)
    {
        updateTimer += a_delta;
        globalTime  += a_delta;

        auto& cfg = Settings::GetSingleton();
        float interval = static_cast<float>(cfg.scanInterval < 1 ? 1 : cfg.scanInterval) * 2.0f;

        if (updateTimer > interval) {
            // Never scan while a menu is open — the engine strips NPC 3D models
            // during inventory/container/barter screens as a performance optimisation.
            // Scanning at that moment misidentifies every nearby NPC as a ghost.
            if (!IsAnyMenuOpen()) {
                FixInvisibleNPCs();
            }
            updateTimer = 0.0f;
        }
    }

    // -----------------------------------------------------------------------
    // Hook
    // -----------------------------------------------------------------------

    struct MainUpdateHook
    {
        static void thunk(RE::Main* a_this, float a_delta)
        {
            func(a_this, a_delta);
            Update(a_delta);
        }
        static inline REL::Relocation<decltype(thunk)> func;
    };

    void Install()
    {
        // Load settings first
        Settings::GetSingleton().Load();

        // Detect which mods are present in this load order
        CheckModPresence();

        // Resolve follower faction pointers
        ResolveFollowerFactions();

        // Hook Main::Update (SSE/AE ID 35551, offset 0x11F)
        REL::Relocation<std::uintptr_t> target{ REL::ID(35551), 0x11F };

        SKSE::AllocTrampoline(14);
        MainUpdateHook::func = SKSE::GetTrampoline().write_call<5>(target.address(), MainUpdateHook::thunk);

        SKSE::log::info("Main::Update hook installed. Ghost NPC Fixer active.");
    }
}
