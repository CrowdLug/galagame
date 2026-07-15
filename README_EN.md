# NewGame - Save Taffy Nya

[中文说明](README.md) | GitHub Repository: `https://github.com/CrowdLug/galagame`

NewGame is a 2D top-down shooting survival game built with **C++20, Qt 6 Widgets, Qt Multimedia, and CMake**. The player controls a character in a fixed combat arena, shoots enemies, avoids traps, collects healing items, and tries to save Taffy Nya or survive as long as possible in endless mode.

## Project Overview

This project uses the Qt Graphics View Framework to implement the game scene, player, enemies, bullets, pickups, traps, HUD, and visual feedback. The codebase is divided into focused modules for scene management, combat simulation, enemy spawning, collision detection, resource loading, weapon logic, and feedback effects.

The project is designed as a practical C++/Qt game development case study. It demonstrates object-oriented design, real-time update loops, event-driven UI programming, JSON-based configuration, and deployable Windows desktop builds.

## Gameplay Features

- **Normal Mode**: the player enters a fixed stage, fights enemy waves within a 60-second combat session, and then reaches the result screen.
- **Endless Mode**: enemies spawn continuously in waves. Enemy pressure increases over time, and the goal is to survive longer and defeat more enemies.
- **Real-time shooting**: the player fires toward the mouse position. Bullets collide with enemies and apply damage.
- **Enemy AI**: melee enemies chase and attack the player, while ranged enemies keep distance and shoot projectiles.
- **Pickups and recovery**: defeated enemies may drop healing items. The player recovers health after collecting them.
- **Trap system**: traps spawn during combat. Touching a trap damages the player and creates a temporary control penalty.
- **Combat feedback**: hit flashes, kill feedback, screen shake, background music, and event text improve the game feel.
- **Data-driven balancing**: weapons, enemies, and stage waves are configured through JSON files for easier tuning and extension.

## Controls

| Action | Input |
| --- | --- |
| Move character | `W` `A` `S` `D` |
| Shoot | Click or hold the left mouse button |
| Rifle | `1` |
| Pistol | `2` |
| Shotgun | `3` |
| Toggle screen shake | `V` |
| Pause / resume enemy spawning | `P` |
| Select menu option | Mouse click |

### Weapon Loadout

| Slot | Weapon | Role | Configuration |
| --- | --- | --- | --- |
| `1` | Rifle | High fire rate and stable sustained damage | `data/weapons/rifle.json` |
| `2` | Pistol | Accurate shots with higher per-shot damage | `data/weapons/pistol.json` |
| `3` | Shotgun | Seven pellets per shot and strong close-range burst | `data/weapons/shotgun.json` |

All three weapons share the current ammo pool. Fire rate, damage, bullet speed, spread, recoil, ammo cost, and pellet count are controlled through JSON.

## Technology Stack

- **Language**: C++20
- **UI framework**: Qt 6 Widgets
- **Audio / media**: Qt 6 Multimedia
- **Build system**: CMake 3.21+
- **Recommended compiler**: MinGW 64-bit or MSVC 2022 x64
- **Recommended IDE**: Qt Creator or Visual Studio Code with CMake Tools

## Environment Requirements

### Required

- Windows 10 / Windows 11
- Qt 6.x, recommended path: `C:\Qt\6.11.0\mingw_64`
- CMake 3.21 or later
- MinGW 64-bit, recommended path: `C:\mingw64`

### Optional

- Visual Studio 2022 or Build Tools
- Ninja
- Qt Creator

If Qt or MinGW is installed in a different location, update the paths in:

- `CMakePresets.json`
- `build_mingw_release.bat`
- `run_game.bat`

## Build and Run

### Option 1: Build with the provided script

Run the following file from the project root:

```bat
build_mingw_release.bat
```

After the build completes, start the game with:

```bat
run_game.bat
```

### Option 2: Build with CMake

```bat
cmake --preset mingw-release
cmake --build --preset mingw-release -j 2
```

Build output:

```text
build/mingw-release/NewGame.exe
```

To deploy Qt runtime dependencies:

```bat
windeployqt6 --release --compiler-runtime --dir build/mingw-release build/mingw-release/NewGame.exe
```

## Project Structure

```text
newgame/
├── assets/                  # Images, UI background, character sprites, music
│   ├── bgm/                 # Background music
│   └── ui/                  # Menu assets
├── data/                    # JSON configuration files
│   ├── enemies/             # Enemy parameters
│   ├── stages/              # Stage wave configuration
│   └── weapons/             # Weapon parameters
├── include/                 # Header files
├── src/                     # C++ source files
├── build_mingw_release.bat  # Release build script
├── run_game.bat             # Game launcher script
├── CMakeLists.txt           # CMake project definition
└── CMakePresets.json        # CMake preset configuration
```

## Main Modules

- `GameManager`: controls game state transitions between menu, combat, and result screens.
- `SceneManager`: creates and switches Qt graphics scenes.
- `CombatScene`: receives keyboard/mouse input, updates HUD text, and displays combat objects.
- `CombatLoop`: runs the combat simulation, enemy spawning, bullet updates, trap updates, and win/loss checks.
- `CollisionManager`: handles collisions among player, enemies, bullets, pickups, and traps.
- `EnemyDirector` / `LightDirector`: manage enemy spawning pace and combat pressure.
- `WeaponSystem`: handles weapon firing, bullet generation, and weapon JSON loading.
- `ResourceManager`: loads image and audio resources.

## Data Configuration

The game uses JSON files to separate gameplay tuning from core logic:

- `data/weapons/rifle.json`: high-rate rifle configuration.
- `data/weapons/pistol.json`: accurate pistol configuration with higher per-shot damage.
- `data/weapons/shotgun.json`: multi-pellet shotgun configuration with wide spread.
- `data/enemies/melee.json`: melee enemy detection radius, speed, attack range, damage, and retreat behavior.
- `data/enemies/ranged.json`: ranged enemy range, preferred distance, attack interval, and damage.
- `data/stages/stage01.json`: timed enemy wave events and enemy counts.

This structure makes the game easier to balance and extend without rewriting core C++ logic.

## Highlights

- Real-time 2D combat built on Qt Graphics View.
- Modular C++ design with clear responsibility boundaries.
- JSON-driven configuration for weapons, enemies, and stages.
- Normal and endless game modes with complete menu-combat-result flow.
- Visual and audio feedback, including screen shake, hit flash, and background music.
- Windows build and deployment workflow with CMake presets and `windeployqt6`.

## Future Improvements

- Add laser or area-of-effect weapons, separate ammo pools, and reload mechanics.
- Add boss encounters and special enemy types.
- Add an upgrade selection system for endless mode.
- Add save data and high-score records.
- Improve UI text encoding and multilingual display support.
- Add automated smoke tests or unit tests for core gameplay systems.

## Purpose

This project is intended for coursework and game development practice. It showcases C++ object-oriented programming, Qt desktop development, CMake project management, JSON-driven game balancing, and a complete basic 2D game loop.
