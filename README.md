# Cyber Siege -- Unreal 5.8, first-person, Aeon native

The Aeon first-party FPS. Everything here is code; the project ships with zero
binary assets and builds its own test arena from engine primitives. Art comes in
later (MetaHuman for the operative, Fab/contract for the exosuit and rifle).

## What's in the box
- `Source/CyberSiege/` -- C++: player (first-person, Enhanced Input built in code),
  projectile, enemy, wave game mode, Canvas HUD, and **AeonSubsystem** -- the
  reference platform SDK (achievements, leaderboard, presence against the real API)
- `Config/` -- render config tuned for RX 9060 XT on Vulkan (see Docs/AEON_RENDER_TARGET.md)
- `Tools/BootstrapCyberArena.py` -- builds the arena map in the editor
- No `Content/` -- the script creates `/Game/Maps/CyberArena` and `/Game/Materials/*`

## Build (Windows, first time)
1. Epic Games Launcher -> install **Unreal Engine 5.8**. Tick **Linux** under Target Platforms.
2. Install the **Linux cross-compile toolchain** for 5.8 (linked from Epic's Linux docs).
   Verify: `echo %LINUX_MULTIARCH_ROOT%` prints a path.
3. Right-click `CyberSiege.uproject` -> **Generate Visual Studio project files**.
4. Open `CyberSiege.sln`, build **Development Editor / Win64**. (Needs VS 2022 with C++.)
5. Double-click `CyberSiege.uproject` -> editor opens, compiles shaders (first time is slow).

## Build the arena
Editor -> **File > Execute Python Script...** -> `Tools/BootstrapCyberArena.py`.
Creates the map and materials, saves them. Press **Play** (PIE) -- you should be in the
arena, first person, RT to fire, waves start after "SYSTEMS ONLINE".

## Package for Aeon (Linux)
**Platforms > Linux > Package Project**, pick a folder. Output: `Linux/CyberSiege.sh` +
`Linux/CyberSiege/Binaries/Linux/CyberSiege`. Then:

    scp -r Linux wiz@Aeon:/home/wiz/aeon/games/cyber-siege-ue/
    # on the console:
    cat > ~/aeon/games/cyber-siege-ue/manifest.json << 'M'
    { "id": "CYBER-SIEGE-STD", "title": "Cyber Siege", "version": "0.2.0",
      "exec": "cd /home/wiz/aeon/games/cyber-siege-ue/Linux && ./CyberSiege.sh" }
    M
    pkill aeon-compositor

Vault -> Cyber Siege -> Play. The HUD's top-right shows **AEON LINK ONLINE** when the
game found the console's session; First Blood unlocks on your first kill and your score
posts to the leaderboard when you die.

## Controls
| Action | Pad | Keyboard/mouse |
|---|---|---|
| Move | Left stick | WASD |
| Look | Right stick | Mouse |
| Fire (auto) | RT | LMB |
| Reload | X | R |
| Jump | A | Space |
| Sprint | Click LS | Left Shift |
| Dodge | B | Left Alt |
| Restart (when dead) | A / RT | Space / LMB |

## Aeon SDK notes (AeonSubsystem)
- Session: `AEON_TOKEN` env var, else `~/.config/aeon/auth.conf` `token=` (the console writes it)
- API: `AEON_API` env var, else `https://aeon-api-production.up.railway.app`
- Routes used: `POST /store/games/{sku}/achievements/{key}/unlock`,
  `POST /store/games/{sku}/leaderboard`, `PUT /presence`
- Achievement key `first_blood` must match the backend's key for this SKU -- verify

## Not in v0.2
Art. Audio. Enemy variety. Pause menu (the Aeon overlay handles pause at the platform
level for now). Those come after the loop is proven on the console.
