#include <SKSE/SKSE.h>
#include "VisibilityFixer.h"

#include <RE/A/Actor.h>
#include <RE/P/ProcessLists.h>
#include <RE/M/Main.h>
#include <RE/N/NiNode.h>
#include <REL/Relocation.h>
#include <SKSE/Trampoline.h>

#include <RE/T/TESObjectREFR.h>

#include <map>
#include <chrono>

using namespace std::string_literals;
using namespace std::chrono_literals;

namespace VisibilityFixer
{
    // Cooldown map: FormID -> Last Fix Time
    std::map<RE::FormID, std::chrono::steady_clock::time_point> fixCooldowns;

    void FixInvisibleNPCs()
    {
        auto processLists = RE::ProcessLists::GetSingleton();
        if (!processLists) return;

        auto now = std::chrono::steady_clock::now();

        // High actor handles are the ones usually around the player and being updated
        for (auto& handle : processLists->highActorHandles) {
            auto actor = handle.get();
            // We only care about actors whose 3D *should* be there (Is3DLoaded check)
            if (!actor || !actor->Is3DLoaded()) continue;

            if (actor->IsPlayerRef()) continue;

            // Check if actor is not disabled and not dead
            if (!actor->IsDisabled() && !actor->IsDead()) {
                
                // CRITICAL CHECK: If Get3D() returns null, the engine failed to render it.
                if (actor->Get3D() == nullptr) {
                    
                    // Check cooldown (don't spam a broken actor every frame)
                    auto it = fixCooldowns.find(actor->GetFormID());
                    if (it != fixCooldowns.end()) {
                        if (now - it->second < 10s) {
                            continue;
                        }
                    }

                    SKSE::log::info("Ghost NPC Detected: {} ({:08X}). Performing Refresh...", actor->GetName(), actor->GetFormID());
                    
                    // Force refresh by clearing disabled flag and updating 3D
                    actor->formFlags &= ~RE::TESForm::RecordFlags::kDisabled;
                    actor->Update3DModel();

                    fixCooldowns[actor->GetFormID()] = now;
                }
            }
        }

        // Cleanup stale cooldowns
        if (fixCooldowns.size() > 100) {
            for (auto it = fixCooldowns.begin(); it != fixCooldowns.end(); ) {
                if (now - it->second > 1min) {
                    it = fixCooldowns.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }

    void Update()
    {
        static auto lastUpdate = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();

        // Check every 2 seconds to avoid overhead
        if (now - lastUpdate > 2s) {
            FixInvisibleNPCs();
            lastUpdate = now;
        }
    }

    // Hooking into the main loop
    struct MainUpdateHook
    {
        static void thunk(RE::Main* a_this, float a_delta)
        {
            func(a_this, a_delta);
            Update();
        }
        static inline REL::Relocation<decltype(thunk)> func;
    };

    void Install()
    {
        // Hooking Main::Update (Address for 1.6.1170)
        // ID for Main::Update is 35551
        REL::Relocation<std::uintptr_t> target{ REL::ID(35551), 0x11F }; 
        
        SKSE::AllocTrampoline(14);
        MainUpdateHook::func = SKSE::GetTrampoline().write_call<5>(target.address(), MainUpdateHook::thunk);
        
        SKSE::log::info("Main::Update hook installed.");
    }
}
