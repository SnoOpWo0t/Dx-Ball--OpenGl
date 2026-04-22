This project is a complete playable DX Ball implementation in C++ using OpenGL (FreeGLUT).

## Features Implemented

- Playable DX Ball core mechanics
- Paddle control with keyboard and mouse
- Brick breaking, score system, 3 lives
- Timer + score + lives HUD
- Ball speed increases gradually over time
- Three drop perks:
  - Extra life (`+1`)
  - Speed up (`S`)
  - Wider paddle (`W`)
- Menu page:
  - Start New Game
  - Resume
  - High Scores
  - Help
  - Exit
- Pause/resume and exit support during gameplay
- Win and Game Over screens
- Persistent high score storage in `.dist/highscores.txt`

## Controls

- Move paddle: `Left/Right` or `A/D`
- Move paddle (mouse): move mouse horizontally
- Launch ball: `Space` or left mouse click
- Pause/resume: `P` (or `R` to resume)
- Back to menu from gameplay: `Esc`
- Exit from anywhere: `Q`
- Menu navigation: `W/S`, `Up/Down`, `Enter`, or mouse click

## Build (CMake)

```powershell
cmake -S . -B build
cmake --build build --config Release
```

Run executable from `build` output.

## Windows (manual MinGW/freeglut example)

If your setup already has FreeGLUT and OpenGL libs available:

```powershell
g++ main.cpp -std=c++17 -lfreeglut -lopengl32 -lglu32 -o dx_ball.exe
./dx_ball.exe
```
