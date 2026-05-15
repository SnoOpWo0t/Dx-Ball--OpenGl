# DX Ball (OpenGL / FreeGLUT)

DX Ball is a modern, extended Breakout-style game implemented in one portable C++ source file. It uses FreeGLUT+OpenGL for rendering and simple Windows multimedia APIs for audio.

Key features
------------
- 🎮 Gameplay: paddle & ball physics, responsive collisions, progressive speed ramp
- 🧱 10 Brick types: Normal, Steel, Explosive, Moving, Invisible, Ice, Fire, Locked, Regenerating, Teleport
- 🎁 9 Drop/Powerups: Extra Life, Speed Boost, Wide Paddle, Shield, Row Clear, Fireball, Multi-ball, Key, Bomb
- ✨ Visuals: combo floating text, particle effects, screen shake, theme+weather backgrounds (nature, city, space, neon)
- 🕹 Menus: multi-page menus (difficulty, maps, theme, weather, settings, help)
- 💾 Persistence: per-map/difficulty highscores saved in `.dist/highscores.txt`
- 🔊 Audio: background music and SFX (Windows MCI/winmm; WAV preferred for reliability)
- 🧪 Debug: optional music debug log present for diagnosis

Controls
--------
- Move paddle: `A` / `D` or Left/Right arrows or mouse
- Launch ball: `Space` or left mouse click
- Pause / Resume: `P`
- Return to menu / Quit: `Esc` / `Q`
- Use Key to unlock locked bricks: `K`
- Menu navigation: `W` / `S`, `Enter` to select

Project structure
-----------------
The important files and folders are:

 - `main.cpp` — main game source (single-file game logic and rendering)
 - `Dx Ball.exe` — compiled executable (if you built it)
 - `Game Music/` — music and SFX assets used by the game
 - `individual_parts/` — example parts / smaller entry points (`main_part1.cpp`, `main_part4.cpp`)
 - `.dist/` — contains generated files at runtime (highscores, optional fallback music files)

Build & Install
---------------
Clone from GitHub:

```powershell
git clone <your-repo-url>
cd "Dx Ball -OpenGl"
```

Prerequisites:

- A C++17-capable compiler (MinGW/g++, or Visual Studio)
- FreeGLUT development headers and libraries
- OpenGL (system-provided)
- On Windows: `winmm` (part of OS; linked automatically)

Option A — Quick manual build (MinGW / g++):

```powershell
g++ main.cpp -std=c++17 -lfreeglut -lopengl32 -lglu32 -lwinmm -o "Dx Ball.exe"
./"Dx Ball.exe"
```

Option B — Build with CMake (recommended for multi-config):

```powershell
cmake -S . -B build
cmake --build build --config Release
# Executable path:
# - MinGW/Ninja: build\dx_ball.exe
# - Visual Studio: build\Release\dx_ball.exe
```

Option C — Run the individual example parts:

```powershell
cd individual_parts
# Build & run main_part1
g++ main_part1.cpp -std=c++17 -lfreeglut -lopengl32 -lglu32 -lwinmm -o part1.exe
./part1.exe

# Build & run main_part4
g++ main_part4.cpp -std=c++17 -lfreeglut -lopengl32 -lglu32 -lwinmm -o part4.exe
./part4.exe
```

Assets & Music
--------------
- Put WAV files in `Game Music/` for best reliability. The game looks for specific filenames (see source) and falls back to `.dist/` locations if present.
- Preferred: `Game Music/Rigor Mormist (gameover music).wav` for game-over.
- If music doesn't play, check `music_debug.log` for MCI errors (on Windows).

Gameplay notes
--------------
- Highscore file is saved to `.dist/highscores.txt`. Ensure `.dist/` exists and is writable.
- Locked bricks require the Key drop to unlock. Keys are rare — use `K` to consume a key when prompted.
- Multi-ball and powerups interact with scoring; check HUD for active timers and multipliers.

Troubleshooting
---------------
- Linker errors for FreeGLUT: install dev packages (MinGW-w64 / pacman/mingw, or vcpkg) and ensure library paths are available.
- If `Dx Ball.exe` is locked while building, close any running instance.
- If music is missing or corrupted, replace MP3 with WAV files in `Game Music/`.

Contributing
------------
- Submit small PRs that keep features isolated. Major refactors should keep the `main.cpp` behaviour identical unless documented.

License & Credits
-----------------
This project is provided as-is. Credit the original author when re-using assets or logic.

Enjoy! 🎉


## Overview

This project includes core brick-breaker gameplay plus extended systems such as:

- Multi-page menu flow (main menu, difficulty, map category, weather, help, score)
- Multiple brick and drop types
- Combo and particle effects
- Per-map and per-difficulty score tracking
- Background music support on Windows (winmm)

## Key Features

### Gameplay

- Paddle and ball mechanics with bounce physics
- Score, lives, timer HUD
- Progressive speed ramp
- Pause, resume, win, and game over states

### Difficulty and Maps

- 4 difficulty levels: Easy, Normal, Hard, Hardest
- 5 map categories with randomized variants
- Hardest mode includes falling-brick pressure

### Brick Types

- Normal
- Steel
- Explosive
- Moving
- Invisible
- Ice
- Fire
- Locked
- Regenerating
- Teleport

### Drops and Powerups

- Extra Life
- Speed Boost
- Wide Paddle
- Shield
- Row Clear
- Fireball
- Multi-ball
- Key (for locked bricks)
- Bomb (negative drop)

### Visual Systems

- Combo feedback text
- Floating score text
- Particle effects and shake effects
- Theme/weather presentation (nature, city, space, neon)

### Audio and Scores

- Background music switching by page/state
- In-menu Game Settings page to control music volume
- Persistent high scores in `.dist/highscores.txt`

## Controls

### In Game

- Move paddle: A / D
- Move paddle: Left Arrow / Right Arrow
- Move paddle: Mouse (horizontal)
- Launch ball: Space or Left Mouse Click
- Pause: P
- Return to menu: Esc
- Use key (unlock locked brick): K

### Pause

- Resume: P or R
- Return to menu: Esc

### Menus

- Navigate: W / S or Up / Down
- Select: Enter or Space or Left Mouse Click
- Back to menu (sub-pages): Esc
- Quit from main menu: Esc or Q
- Open Game Settings from main menu to set volume: Mute, 25%, 50%, 75%, 100%

### End Screens (Win / Game Over)

- New game flow: N
- Back to menu: M or Esc

### Global

- Quit game: Q

## Project Structure

```text
.
|-- CMakeLists.txt
|-- main.cpp
|-- README.md
|-- Game Music/
|-- .dist/
`-- build/
```

## Requirements

- C++ compiler with C++17 support
- OpenGL
- FreeGLUT/GLUT development package
- CMake 3.16+ (if building with CMake)
- Windows: winmm library (linked automatically in this project)

## Build and Run

### Option 1: CMake (Recommended)

```powershell
cmake -S . -B build
cmake --build build --config Release
```

Run output executable:

- Single-config generators (MinGW/Ninja): `build\dx_ball.exe`
- Multi-config generators (Visual Studio): `build\Release\dx_ball.exe`

### Option 2: MinGW g++ (Manual)

```powershell
g++ main.cpp -std=c++17 -lfreeglut -lopengl32 -lglu32 -lwinmm -o "Dx Ball.exe"
"./Dx Ball.exe"
```

## Music Setup (Windows)

For reliable in-game playback, use WAV files.

Primary game-over file now expected at:

- `Game Music/Rigor Mormist (gameover music).wav`

Other optional fallback files:

- `.dist/menu_music.wav`
- `.dist/game_music.wav`
- `.dist/win_music.wav`
- `menu_music.wav`
- `game_music.wav`
- `win_music.wav`
- `.dist/music.wav`
- `music.wav`

Notes:

- MP3 may fail on some Windows setups when accessed via MCI.
- WAV is recommended for stable playback inside the game.

## Troubleshooting

### Build Errors (GLUT/OpenGL)

- Ensure FreeGLUT and OpenGL development libraries are installed.
- Verify compiler can find headers and libraries.
- On MinGW, make sure `-lfreeglut -lopengl32 -lglu32` are present.

### Cannot Rebuild Executable

- If linker reports permission denied on `Dx Ball.exe`, close any running game process and build again.

### Music Not Playing

- Confirm file exists exactly at `Game Music/Rigor Mormist (gameover music).wav`.
- Prefer WAV over MP3 for in-game playback reliability.
- Check `music_debug.log` for path and playback diagnostics.

### High Score File Missing

- The game writes to `.dist/highscores.txt`.
- Ensure `.dist` exists and is writable.

## Notes for Contributors

- Main gameplay and rendering logic is in `main.cpp`.
- Keep controls and music paths synchronized with this README when changing code.
- Prefer small, focused changes and test game states: menu, play, pause, win, and game over.
