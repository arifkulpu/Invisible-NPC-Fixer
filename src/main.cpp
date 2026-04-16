#include <SKSE/SKSE.h>
#include <spdlog/sinks/basic_file_sink.h>
#include "VisibilityFixer.h"

using namespace std::string_literals;

namespace
{
    void InitializeLog()
    {
        auto path = SKSE::log::log_directory();
        if (!path) {
            return;
        }

        *path /= SKSE::PluginDeclaration::GetSingleton()->GetName();
        *path += ".log";

        auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
        auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

        log->set_level(spdlog::level::info);
        log->flush_on(spdlog::level::info);

        spdlog::set_default_logger(std::move(log));
        spdlog::set_pattern("%g(%#): [%^%l%$] %v"s);
    }

    void OnMessage(SKSE::MessagingInterface::Message* a_msg)
    {
        switch (a_msg->type) {
        case SKSE::MessagingInterface::kDataLoaded:
            VisibilityFixer::Install();
            SKSE::log::info("VisibilityFixer installed after DataLoaded.");
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

    SKSE::log::info("NpcGhostFix loaded.");
    return true;
}
