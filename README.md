# DX Ball (OpenGL / FreeGLUT)

A full-featured DX Ball style arcade game built in C++ with OpenGL and FreeGLUT.

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
