# Invisible NPC Fixer

> An SKSE plugin for Skyrim Special Edition & Anniversary Edition that automatically fixes the "ghost NPC" bug — NPCs that are present in the world but invisible or appear naked.

**GitHub:** https://github.com/arifkulpu/Invisible-NPC-Fixer

---

## English

### What it does

- Scans NPCs near the player every few seconds and detects actors whose 3D node has failed to render (ghost NPCs).
- Fixes affected NPCs by directly resetting the NiNode visibility flag (`SetAppCulled`). This operation only touches the render graph and **never modifies the actor's inventory or outfit**.
- Skips actors that are currently inside an animation scene (OStim, OStim NG, SexLab, etc.) to avoid interfering with those frameworks.
- Skips all followers — both vanilla followers (via `CurrentFollowerFaction`) and followers managed by Nether's Follower Framework (NFF), Amazing Follower Tweaks (AFT), and Enhanced Follower Framework (EFF).
- Automatically pauses the scan whenever a menu is open (inventory, container, barter, etc.) — the engine temporarily unloads NPC 3D models during menu screens, and scanning at that moment would produce false positives.
- All behavior can be configured via `NpcGhostFix.ini` in `Data/SKSE/Plugins/`. The mod works with default values even without the INI file.

### Compatibility

| Mod | Status |
|---|---|
| OStim / OStim NG / OSex | ✅ Compatible — actors in active scenes are skipped |
| SexLab Framework | ✅ Compatible — actors in active scenes are skipped |
| Nether's Follower Framework (NFF) | ✅ Compatible — managed followers are skipped |
| Amazing Follower Tweaks (AFT) | ✅ Compatible — managed followers are skipped |
| Enhanced Follower Framework (EFF) | ✅ Compatible — managed followers are skipped |
| Vanilla followers | ✅ Protected — CurrentFollowerFaction is always checked |

### Requirements

- Skyrim Special Edition or Anniversary Edition (1.5.97 – 1.6.1170+)
- [SKSE64](https://skse.silverlock.org/)
- [Address Library for SKSE Plugins](https://www.nexusmods.com/skyrimspecialedition/mods/32444)

### Installation

1. Install the requirements above.
2. Place `NpcGhostFix.dll` into `Data/SKSE/Plugins/`.
3. Optionally place `NpcGhostFix.ini` into `Data/SKSE/Plugins/` to customize settings.

---

## Türkçe

### Ne yapar?

- Oyuncu çevresindeki NPC'leri birkaç saniyede bir tarar; 3D node'u render edilemeyen (hayalet NPC) aktörleri tespit eder.
- Etkilenen NPC'leri NiNode görünürlük flag'ini sıfırlayarak (`SetAppCulled`) düzeltir. Bu işlem yalnızca render grafiğine dokunur; **aktörün envanterini veya kıyafetini asla değiştirmez.**
- OStim, OStim NG, SexLab gibi animasyon sahnelerindeki aktörleri atlar; bu framework'lerle çakışma yaşanmaz.
- Tüm takipçileri korur: vanilla takipçiler (`CurrentFollowerFaction`) ve Nether's Follower Framework (NFF), Amazing Follower Tweaks (AFT), Enhanced Follower Framework (EFF) tarafından yönetilen takipçiler.
- Herhangi bir menü açıkken (envanter, sandık, alışveriş vb.) taramayı otomatik olarak duraklatır. Bu menüler sırasında motor NPC 3D modellerini geçici olarak bellekten kaldırdığından, o anda tarama yapılması yanlış pozitif sonuçlar üretir.
- Tüm davranışlar `Data/SKSE/Plugins/NpcGhostFix.ini` üzerinden ayarlanabilir. INI dosyası olmadan mod varsayılan değerlerle çalışır.

### Uyumluluk

| Mod | Durum |
|---|---|
| OStim / OStim NG / OSex | ✅ Uyumlu — aktif sahnedeki aktörler atlanır |
| SexLab Framework | ✅ Uyumlu — aktif sahnedeki aktörler atlanır |
| Nether's Follower Framework (NFF) | ✅ Uyumlu — yönetilen takipçiler atlanır |
| Amazing Follower Tweaks (AFT) | ✅ Uyumlu — yönetilen takipçiler atlanır |
| Enhanced Follower Framework (EFF) | ✅ Uyumlu — yönetilen takipçiler atlanır |
| Vanilla takipçiler | ✅ Korumalı — CurrentFollowerFaction her zaman kontrol edilir |

### Gereksinimler

- Skyrim Special Edition veya Anniversary Edition (1.5.97 – 1.6.1170+)
- [SKSE64](https://skse.silverlock.org/)
- [SKSE Eklentileri için Adres Kütüphanesi](https://www.nexusmods.com/skyrimspecialedition/mods/32444)

### Kurulum

1. Yukarıdaki gereksinimleri kurun.
2. `NpcGhostFix.dll` dosyasını `Data/SKSE/Plugins/` klasörüne kopyalayın.
3. İsteğe bağlı: `NpcGhostFix.ini` dosyasını da aynı klasöre kopyalayarak ayarları özelleştirin.

---

## License / Lisans

Copyright (c) 2026 Arif KULPU. All Rights Reserved. — Tüm Hakları Saklıdır.  
See [LICENSE](LICENSE) for details.
