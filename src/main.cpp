#include <SKSE/SKSE.h>
#include <spdlog/sinks/basic_file_sink.h>
#include "VisibilityFixer.h"
#include "Settings.h"

using namespace std::string_literals;

namespace
{
    void InitializeLog()
    {
        auto path = SKSE::log::log_directory();
        if (!path) return;

        *path /= SKSE::PluginDeclaration::GetSingleton()->GetName();
        *path += ".log";

        auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
        auto log  = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

        log->set_level(spdlog::level::trace);   // trace lets debug() calls through when enabled
        log->flush_on(spdlog::level::info);

        spdlog::set_default_logger(std::move(log));
        spdlog::set_pattern("%g(%#): [%^%l%$] %v"s);
    }

    void OnMessage(SKSE::MessagingInterface::Message* a_msg)
    {
        switch (a_msg->type) {
        case SKSE::MessagingInterface::kDataLoaded:
            // Install() also loads settings and detects mods after all ESPs are loaded
            VisibilityFixer::Install();
            SKSE::log::info("NpcGhostFix: VisibilityFixer installed (DataLoaded).");
            break;
        }
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    InitializeLog();
    SKSE::log::info("NpcGhostFix loading...");

    SKSE::Init(skse);

    auto messaging = SKSE::GetMessagingInterface();
    if (!messaging || !messaging->RegisterListener(OnMessage)) {
        return false;
    }

    SKSE::log::info("NpcGhostFix loaded successfully.");
    return true;
}
