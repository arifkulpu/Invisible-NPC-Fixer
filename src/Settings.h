#pragma once

#include "PCH.h"
#include <string>
#include <Windows.h>

class Settings {
public:
    static Settings& GetSingleton() {
        static Settings instance;
        return instance;
    }

    void Load() {
        constexpr auto path = L"Data/SKSE/Plugins/NpcGhostFix.ini";

        // --- [General] ---
        scanInterval  = GetPrivateProfileIntW(L"General", L"fScanInterval",  1, path);
        cooldown      = GetPrivateProfileIntW(L"General", L"fCooldown",      60, path);
        startupDelay  = GetPrivateProfileIntW(L"General", L"fStartupDelay",  5, path);
        fixNakedness  = GetPrivateProfileIntW(L"General", L"bFixNakedness",  1, path) != 0;

        // --- [Debug] ---
        logFixes       = GetPrivateProfileIntW(L"Debug", L"bLogFixes",       1, path) != 0;
        verboseLogging = GetPrivateProfileIntW(L"Debug", L"bVerboseLogging", 0, path) != 0;

        // --- [Compatibility] ---
        // Seks / animasyon modları aktifken tüm düzeltmeyi askıya al
        disableWithOStim   = GetPrivateProfileIntW(L"Compatibility", L"bDisableWithOStim",   1, path) != 0;
        disableWithSexLab  = GetPrivateProfileIntW(L"Compatibility", L"bDisableWithSexLab",  1, path) != 0;
        disableWithFurnitureMod = GetPrivateProfileIntW(L"Compatibility", L"bDisableWithFurnitureMod", 1, path) != 0;

        // Takipçi yöneticisi uyumluluğu — bu modların aktörlerini düzeltme listesinden çıkar
        skipNFFFollowers   = GetPrivateProfileIntW(L"Compatibility", L"bSkipNFFFollowers",   1, path) != 0;
        skipAFTFollowers   = GetPrivateProfileIntW(L"Compatibility", L"bSkipAFTFollowers",   1, path) != 0;
        skipEFFFollowers   = GetPrivateProfileIntW(L"Compatibility", L"bSkipEFFFollowers",   1, path) != 0;

        // --- Logla ---
        logger::info("Ayarlar yuklendi:");
        logger::info(" -> Tarama Araligi       : {}s", scanInterval);
        logger::info(" -> Cooldown             : {}s", cooldown);
        logger::info(" -> Baslangic Gecikmesi  : {}s", startupDelay);
        logger::info(" -> Ciplaklik Duzeltmesi : {}", fixNakedness      ? "Acik" : "Kapali");
        logger::info(" -> Log Duzeltmeleri     : {}", logFixes           ? "Acik" : "Kapali");
        logger::info(" -> Detayli Log          : {}", verboseLogging     ? "Acik" : "Kapali");
        logger::info(" -> OStim ile Durdur     : {}", disableWithOStim   ? "Acik" : "Kapali");
        logger::info(" -> SexLab ile Durdur    : {}", disableWithSexLab  ? "Acik" : "Kapali");
        logger::info(" -> Mobilya Mod Durdur   : {}", disableWithFurnitureMod ? "Acik" : "Kapali");
        logger::info(" -> NFF Takipcileri Atla : {}", skipNFFFollowers   ? "Acik" : "Kapali");
        logger::info(" -> AFT Takipcileri Atla : {}", skipAFTFollowers   ? "Acik" : "Kapali");
        logger::info(" -> EFF Takipcileri Atla : {}", skipEFFFollowers   ? "Acik" : "Kapali");
    }

    // [General]
    std::uint32_t scanInterval{ 1 };
    std::uint32_t cooldown{ 60 };
    std::uint32_t startupDelay{ 5 };
    bool fixNakedness{ true };

    // [Debug]
    bool logFixes{ true };
    bool verboseLogging{ false };

    // [Compatibility] — animasyon / seks modları
    bool disableWithOStim{ true };
    bool disableWithSexLab{ true };
    bool disableWithFurnitureMod{ true };

    // [Compatibility] — takipçi yöneticisi modları
    bool skipNFFFollowers{ true };
    bool skipAFTFollowers{ true };
    bool skipEFFFollowers{ true };

private:
    Settings() = default;
};
