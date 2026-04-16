#include <SKSE/SKSE.h>
#include "VisibilityFixer.h"

#include <RE/A/Actor.h>
#include <RE/P/ProcessLists.h>
#include <RE/M/Main.h>
#include <RE/N/NiNode.h>
#include <RE/T/TESObjectREFR.h>
#include <RE/P/PlayerCharacter.h>
#include <REL/Relocation.h>
#include <SKSE/Trampoline.h>
#include <SKSE/SKSE.h>

#include <unordered_map>

using namespace std::string_literals;

namespace VisibilityFixer
{
    // Cooldown map: FormID -> Last Fix Time (in custom timer units)
    std::unordered_map<RE::FormID, float> fixCooldowns;
    
    static float updateTimer = 0.0f;
    static float cleanupTimer = 0.0f;
    static float globalTime = 0.0f;

    void FixInvisibleNPCs()
    {
        auto processLists = RE::ProcessLists::GetSingleton();
        auto player = RE::PlayerCharacter::GetSingleton();
        if (!processLists || !player) return;

        // High actor handles are the ones usually around the player and being updated
        for (auto& handle : processLists->highActorHandles) {
            auto actor = handle.get();
            // We only care about actors whose 3D *should* be there (Is3DLoaded check)
            if (!actor || !actor->Is3DLoaded()) continue;

            if (actor->IsPlayerRef()) continue;

            // Check if actor is not disabled, not dead, and not deleted
            if (actor->IsDisabled() || actor->IsDead() || actor->IsDeleted() || ((actor->formFlags & RE::TESForm::RecordFlags::kDeleted) != 0)) {
                continue;
            }

            // Distance check (approx 4096 units)
            auto p1 = player->GetPosition();
            auto p2 = actor->GetPosition();
            float distSq = (p1.x - p2.x)*(p1.x - p2.x) + (p1.y - p2.y)*(p1.y - p2.y) + (p1.z - p2.z)*(p1.z - p2.z);
            if (distSq > (4096.0f * 4096.0f)) {
                continue;
            }

            // CRITICAL CHECK: If Get3D() returns null, the engine failed to render it.
            if (actor->Get3D() == nullptr) {
                
                // Check cooldown (don't spam a broken actor every frame)
                auto it = fixCooldowns.find(actor->GetFormID());
                if (it != fixCooldowns.end()) {
                    if (globalTime - it->second < 10.0f) {
                        continue;
                    }
                }

                SKSE::log::info("Ghost NPC Detected: {} ({:08X}). Queueing Refresh Task...", actor->GetName(), actor->GetFormID());
                
                // Queue task so we don't crash main thread mid-render
                auto taskInterface = SKSE::GetTaskInterface();
                if (taskInterface) {
                    RE::FormID actorFormID = actor->GetFormID();
                    taskInterface->AddTask([actorFormID]() {
                        auto actorPtr = RE::TESForm::LookupByID<RE::Actor>(actorFormID);
                        if (actorPtr && !actorPtr->IsDeleted()) {
                            actorPtr->formFlags &= ~RE::TESForm::RecordFlags::kDisabled;
                            actorPtr->Update3DModel();
                        }
                    });
                }

                fixCooldowns[actor->GetFormID()] = globalTime;
            }
        }

        // Cleanup stale cooldowns
        if (fixCooldowns.size() > 100) {
            for (auto it = fixCooldowns.begin(); it != fixCooldowns.end(); ) {
                if (globalTime - it->second > 60.0f) { // 1 min timer
                    it = fixCooldowns.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }

    void Update(float a_delta)
    {
        updateTimer += a_delta;
        cleanupTimer += a_delta;
        globalTime += a_delta;

        // Check every 2 seconds to avoid overhead
        if (updateTimer > 2.0f) {
            FixInvisibleNPCs();
            updateTimer = 0.0f;
        }
    }

    // Hooking into the main loop
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
        // Address for Main::Update in SSE/AE is statically ID 35551
        REL::Relocation<std::uintptr_t> target{ REL::ID(35551), 0x11F }; 
        
        SKSE::AllocTrampoline(14);
        MainUpdateHook::func = SKSE::GetTrampoline().write_call<5>(target.address(), MainUpdateHook::thunk);
        
        SKSE::log::info("Main::Update hook installed.");
    }
}
