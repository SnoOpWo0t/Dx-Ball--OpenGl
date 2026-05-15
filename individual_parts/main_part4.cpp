#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#include <direct.h>
#include <io.h>
#else
#include <sys/stat.h>
#endif

#include <GL/freeglut.h>
#include <algorithm>
#include <cmath>
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

using namespace std;
#define MAX_BRICKS 60
#define MAX_DROPS 100
#define MAX_SCORES 10

// game pages
const int PAGE_MENU = 0;
const int PAGE_PLAY = 1;
const int PAGE_PAUSE = 2;
const int PAGE_SCORE = 3;
const int PAGE_HELP = 4;
const int PAGE_WIN = 5;
const int PAGE_OVER = 6;
const int PAGE_DIFFICULTY = 7;
const int PAGE_MAP_CATEGORY = 8;
const int PAGE_MAP_SELECT = 9;
const int PAGE_WEATHER = 10;
const int PAGE_SETTINGS = 11;

// difficulty levels
const int DIFFICULTY_EASY = 0;
const int DIFFICULTY_NORMAL = 1;
const int DIFFICULTY_HARD = 2;
const int DIFFICULTY_HARDEST = 3;

// map categories
const int MAP_CAT_CLASSIC = 0;
const int MAP_CAT_CHECKERBOARD = 1;
const int MAP_CAT_CIRCLE = 2;
const int MAP_CAT_PYRAMID = 3;
const int MAP_CAT_SPECIAL = 4;

// weather types
const int WEATHER_SUNNY = 0;
const int WEATHER_RAINY = 1;
const int WEATHER_STORMY = 2;
const int WEATHER_NIGHT = 3;

// themes (backgrounds)
const int THEME_NATURE = 0;
const int THEME_CITY = 1;
const int THEME_SPACE = 2;
const int THEME_NEON = 3;

// drop items
// Feature: Power-up drops (added by  Kawser)
//  Kawser: implemented drop types, Drop struct and spawn/apply mechanics
const int DROP_NONE = 0;
const int DROP_LIFE = 1;
const int DROP_SPEED = 2;
const int DROP_WIDE = 3;
const int DROP_BOMB = 4;
const int DROP_SHIELD = 5;
const int DROP_ROW_CLEAR = 6;
const int DROP_FIREBALL = 7;
const int DROP_MULTIBALL = 8;
const int DROP_KEY = 9;

struct Brick {
    float x;
    float y;
    float w;
    float h;
    float r;
    float g;
    float b;
    int points;
    int alive;
    int type; // 0 normal, 1 steel, 2 explosive, 3 moving, 4 invisible, 5 ice, 6 fire, 7 locked, 8 regen, 9 teleport
    int hp;
    float regenTimer; // for regen bricks
    float moveDir;    // for moving bricks (1.0 or -1.0)
    int isLocked;     // for locked bricks
    int linkedBrick;  // for teleport bricks
};

struct Drop {
    float x;
    float y;
    float size;
    float vy;
    int type;
    int active;
};

// map and difficulty
int selectedDifficulty = DIFFICULTY_NORMAL;
int selectedMap = 1;
int difficultyIndex = DIFFICULTY_NORMAL;
int mapIndex = 1;
int selectedMapCategory = MAP_CAT_CLASSIC;
int mapCategoryIndex = MAP_CAT_CLASSIC;
int selectedWeather = WEATHER_SUNNY;
int weatherIndex = WEATHER_SUNNY;
int selectedTheme = THEME_NATURE;
int themeIndex = THEME_NATURE;
int selectedMapVariant = 0;

float mouseX = -1000.0f;
float mouseY = -1000.0f;
int mouseVisible = 0;

// weather and lightning
float lightningTimer = 0.0f;
float lightningDuration = 0.15f;
float stormIntensity = 0.0f;
int showLightning = 0;
float weatherTimer = 0.0f;

struct ScoreLine {
    int score;
    float timeSec;
    int mapCategory;
    int difficulty;
};

int winW = 1000;
int winH = 700;

int currentPage = PAGE_MENU;
int canResume = 0;
int menuIndex = 0;

float paddleX = 500.0f;
float paddleY = 48.0f;
float paddleW = 130.0f;
float paddleH = 18.0f;
float paddleSpeed = 560.0f;

float ballX = 500.0f;
float ballY = 90.0f;
float ballR = 9.0f;
float ballVX = 180.0f;
float ballVY = 300.0f;
int ballAttached = 1;

float baseBallSpeed = 340.0f;
float speedRamp = 1.0f;
float speedRampNext = 12.0f;
float speedRampStep = 0.06f;
float speedRampMax = 2.2f;

float speedBoostTimer = 0.0f;
float speedBoostMul = 1.30f;
float speedBoostDur = 10.0f;

float wideTimer = 0.0f;
float wideDur = 12.0f;
float paddleNormalW = 130.0f;
float paddleWideW = 190.0f;

float shieldTimer = 0.0f;
float shieldDur = 8.0f;

int moveLeft = 0;
int moveRight = 0;

int lives = 3;
int score = 0;
float playTimeSec = 0.0f;
int scoreSaved = 0;
int hasKey = 0;
float keyAssistCooldown = 0.0f;

// Combo system
int comboCount = 0;
float comboTimer = 0.0f;
float comboDecay = 2.0f; // seconds to reset combo

// Fireball power-up
float fireballTimer = 0.0f;
float fireballDur = 8.0f;
float iceBallSlowTimer = 0.0f;
float iceBallSlowDur = 6.0f;
float fireBallSpeedTimer = 0.0f;
float fireBallSpeedDur = 6.0f;

// Multi-ball support (extra balls besides main ball)
const int MAX_EXTRA_BALLS = 5;
int extraBallActive[MAX_EXTRA_BALLS];
float extraBallX[MAX_EXTRA_BALLS];
float extraBallY[MAX_EXTRA_BALLS];
float extraBallVX[MAX_EXTRA_BALLS];
float extraBallVY[MAX_EXTRA_BALLS];
float extraBallR = 7.0f;
float extraBallR_val[MAX_EXTRA_BALLS];  // 0.0-1.0 color component
float extraBallG_val[MAX_EXTRA_BALLS];
float extraBallB_val[MAX_EXTRA_BALLS];
int extraBallMultiplierActive = 0;
float extraBallScoreMul = 1.5f;

// Juice: floating texts and simple particles
struct FloatText { float x,y; float vy; float life; char txt[48]; float r,g,b; };
FloatText ftexts[32];

struct Particle { float x,y; float vx,vy; float life; float r,g,b; };
Particle parts[128];
float shakeTimer = 0.0f;
float shakeIntensity = 0.0f;

int brickCount = 0;
Brick bricks[MAX_BRICKS];

int dropCount = 0;
Drop drops[MAX_DROPS];

ScoreLine highscores[MAX_SCORES];
int highCount = 0;

int prevTick = 0;

const char* scorePath = ".dist/highscores.txt";
int musicVolumePercent = 75;

#ifdef _WIN32
const int MUSIC_NONE = -1;
const int MUSIC_MENU = 0;
const int MUSIC_GAME = 1;
const int MUSIC_RESULT = 2;
const int MUSIC_SCORE = 3;

int currentMusicTrack = MUSIC_NONE;
int mciMusicActive = 0;
int lastMusicStopTime = 0;

int mciVolumeFromPercent(int percent) {
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    return (percent * 1000) / 100;
}

void applyMusicVolume() {
    char cmd[96];
    snprintf(cmd, sizeof(cmd), "set dxball_bgm audio all volume to %d", mciVolumeFromPercent(musicVolumePercent));
    mciSendStringA(cmd, NULL, 0, NULL);
}

void logMusic(const char* msg) {
    FILE* f = fopen("music_debug.log", "a");
    if (f) {
        fprintf(f, "%s\n", msg);
        fflush(f);
        fclose(f);
    }
}

const char* menuMusicPaths[] = {
    "Game Music/08. In The Eyes(Menu Bgm).mp3",
    ".dist/menu_music.wav",
    "menu_music.wav",
    ".dist/music.wav",
    "music.wav"
};

const char* gameMusicPaths[] = {
    "Game Music/10. Ultimate Battle (in game music).wav",
    "Game Music/10. Ultimate Battle (in game music).mp3",
    ".dist/game_music.wav",
    "game_music.wav",
    ".dist/music.wav",
    "music.wav"
};

const char* resultMusicPaths[] = {
    "Game Music/Rigor Mormist (gameover music).wav",
    "Game Music/Rigor Mormist (gameover music).mp3",
    ".dist/win_music.wav",
    "win_music.wav",
    ".dist/game_music.wav",
    "game_music.wav",
    ".dist/music.wav",
    "music.wav"
};

const char* scoreMusicPaths[] = {
    "Game Music/10. High Score.mp3",
    ".dist/score_music.wav",
    "score_music.wav"
};
#endif

#ifdef _WIN32
int isWavPath(const char* path) {
    const char* ext = strrchr(path, '.');
    if (!ext) {
        return 0;
    }
    if (ext[0] != '.') {
        return 0;
    }
    if ((ext[1] == 'w' || ext[1] == 'W') &&
        (ext[2] == 'a' || ext[2] == 'A') &&
        (ext[3] == 'v' || ext[3] == 'V') &&
        ext[4] == '\0') {
        return 1;
    }
    return 0;
}

void buildMusicPath(const char* relPath, char* outPath, int maxLen) {
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, sizeof(exePath));
    char* lastSlash = strrchr(exePath, '\\');
    if (lastSlash) {
        *lastSlash = '\0';
    }
    
    // Convert forward slashes to backslashes
    char normalizedPath[MAX_PATH];
    strncpy(normalizedPath, relPath, sizeof(normalizedPath) - 1);
    normalizedPath[sizeof(normalizedPath) - 1] = '\0';
    for (int i = 0; normalizedPath[i]; i++) {
        if (normalizedPath[i] == '/') {
            normalizedPath[i] = '\\';
        }
    }
    
    snprintf(outPath, maxLen, "%s\\%s", exePath, normalizedPath);
}

int startMciMusic(const char* path) {
    char cmd[1024];
    char errMsg[256];
    MCIERROR err;
    
    // Check if file exists
    char logBuf[512];
    if (_access(path, 0) != 0) {
        snprintf(logBuf, sizeof(logBuf), "[MUSIC] FILE NOT FOUND: %s", path);
        logMusic(logBuf);
        return 0;
    }
    snprintf(logBuf, sizeof(logBuf), "[MUSIC] File exists: %s", path);
    logMusic(logBuf);
    
    mciSendStringA("close dxball_bgm", NULL, 0, NULL);
    Sleep(50);

    // Try without type first (often works better)
    snprintf(cmd, sizeof(cmd), "open \"%s\" alias dxball_bgm", path);
    err = mciSendStringA(cmd, NULL, 0, NULL);
    if (err != 0) {
        mciGetErrorStringA(err, errMsg, sizeof(errMsg));
        snprintf(logBuf, sizeof(logBuf), "[MUSIC] MCI open failed: %s", errMsg);
        logMusic(logBuf);
        return 0;
    }

    Sleep(50);
    err = mciSendStringA("play dxball_bgm repeat", NULL, 0, NULL);
    if (err != 0) {
        mciGetErrorStringA(err, errMsg, sizeof(errMsg));
        snprintf(logBuf, sizeof(logBuf), "[MUSIC] MCI play failed: %s", errMsg);
        logMusic(logBuf);
        mciSendStringA("close dxball_bgm", NULL, 0, NULL);
        return 0;
    }

    Sleep(30);
    applyMusicVolume();

    mciMusicActive = 1;
    lastMusicStopTime = glutGet(GLUT_ELAPSED_TIME);
    logMusic("[MUSIC] MCI Music started successfully");
    return 1;
}

void stopMusic() {
    PlaySoundA(NULL, NULL, 0);
    if (mciMusicActive) {
        mciSendStringA("stop dxball_bgm", NULL, 0, NULL);
        Sleep(30);
        mciSendStringA("close dxball_bgm", NULL, 0, NULL);
        mciMusicActive = 0;
        lastMusicStopTime = glutGet(GLUT_ELAPSED_TIME);
    }
}

int startMusicFromPaths(const char** paths, int count) {
    char logBuf[512];
    for (int i = 0; i < count; i++) {
        char fullPath[MAX_PATH];
        buildMusicPath(paths[i], fullPath, sizeof(fullPath));
        snprintf(logBuf, sizeof(logBuf), "[MUSIC] Trying path %d: %s", i, fullPath);
        logMusic(logBuf);

        // Try MCI first, then PlaySound for wav fallback.
        if (startMciMusic(fullPath)) {
            snprintf(logBuf, sizeof(logBuf), "[MUSIC] SUCCESS with MCI: %s", fullPath);
            logMusic(logBuf);
            return 1;
        }

        if (isWavPath(fullPath) && PlaySoundA(fullPath, NULL, SND_ASYNC | SND_LOOP | SND_FILENAME | SND_NODEFAULT)) {
            snprintf(logBuf, sizeof(logBuf), "[MUSIC] SUCCESS with PlaySound: %s", fullPath);
            logMusic(logBuf);
            mciMusicActive = 0;
            return 1;
        }
    }
    logMusic("[MUSIC] FAILED - all paths exhausted");
    return 0;
}

void startMusicTrack(int track) {
    if (track == currentMusicTrack) {
        return;
    }

    stopMusic();
    Sleep(80);

    int ok = 0;
    if (track == MUSIC_MENU) {
        ok = startMusicFromPaths(menuMusicPaths, (int)(sizeof(menuMusicPaths) / sizeof(menuMusicPaths[0])));
    } else if (track == MUSIC_GAME) {
        ok = startMusicFromPaths(gameMusicPaths, (int)(sizeof(gameMusicPaths) / sizeof(gameMusicPaths[0])));
    } else if (track == MUSIC_RESULT) {
        ok = startMusicFromPaths(resultMusicPaths, (int)(sizeof(resultMusicPaths) / sizeof(resultMusicPaths[0])));
    } else if (track == MUSIC_SCORE) {
        ok = startMusicFromPaths(scoreMusicPaths, (int)(sizeof(scoreMusicPaths) / sizeof(scoreMusicPaths[0])));
    }

    if (ok) {
        currentMusicTrack = track;
    } else {
        currentMusicTrack = MUSIC_NONE;
    }
}

void syncMusicForPage() {
    char logBuf[256];
    snprintf(logBuf, sizeof(logBuf), "[MUSIC] syncMusicForPage - currentPage: %d", currentPage);
    logMusic(logBuf);
    
    if (currentPage == PAGE_PLAY) {
        logMusic("[MUSIC] Syncing to MUSIC_GAME");
        startMusicTrack(MUSIC_GAME);
    } else if (currentPage == PAGE_WIN || currentPage == PAGE_OVER) {
        if (currentPage == PAGE_OVER) {
            logMusic("[MUSIC] PAGE_OVER detected - starting MUSIC_RESULT");
        }
        startMusicTrack(MUSIC_RESULT);
    } else if (currentPage == PAGE_SCORE) {
        logMusic("[MUSIC] Syncing to MUSIC_SCORE");
        startMusicTrack(MUSIC_SCORE);
    } else {
        logMusic("[MUSIC] Syncing to MUSIC_MENU");
        startMusicTrack(MUSIC_MENU);
    }
}
#endif

#ifndef _WIN32
void applyMusicVolume() {
}
#endif

void quitGame() {
#ifdef _WIN32
    stopMusic();
#endif
    exit(0);
}

int countLockedBricksAlive() {
    int count = 0;
    for (int i = 0; i < brickCount; i++) {
        if (bricks[i].alive && bricks[i].type == 7 && bricks[i].isLocked) {
            count++;
        }
    }
    return count;
}

void playSfx(int kind, int minGapMs = 45) {
#ifdef _WIN32
    static int lastSfxTick = 0;
    int now = glutGet(GLUT_ELAPSED_TIME);
    if (now - lastSfxTick < minGapMs) {
        return;
    }
    lastSfxTick = now;

    UINT beepType = MB_OK;
    if (kind == 1) {
        beepType = MB_ICONASTERISK;
    } else if (kind == 2) {
        beepType = MB_ICONHAND;
    } else if (kind == 3) {
        beepType = MB_ICONEXCLAMATION;
    }
    MessageBeep(beepType);
#else
    (void)kind;
    (void)minGapMs;
#endif
}

void playSfxFile(const char* filePath) {
#ifdef _WIN32
    static int lastSfxTime = 0;
    int now = glutGet(GLUT_ELAPSED_TIME);
    if (now - lastSfxTime < 50) {
        return;
    }
    lastSfxTime = now;

    char cmd[1024];
    mciSendStringA("close dxball_sfx", NULL, 0, NULL);
    Sleep(30);
    
    snprintf(cmd, sizeof(cmd), "open \"%s\" type mpegvideo alias dxball_sfx", filePath);
    if (mciSendStringA(cmd, NULL, 0, NULL) != 0) {
        snprintf(cmd, sizeof(cmd), "open \"%s\" alias dxball_sfx", filePath);
        if (mciSendStringA(cmd, NULL, 0, NULL) != 0) {
            return;
        }
    }
    
    Sleep(20);
    mciSendStringA("play dxball_sfx", NULL, 0, NULL);
#else
    (void)filePath;
#endif
}

void spawnForcedDrop(float x, float y, int type) {
    if (dropCount >= MAX_DROPS) {
        return;
    }

    Drop d;
    d.x = x;
    d.y = y;
    d.size = 18.0f;
    d.vy = 150.0f;
    d.type = type;
    d.active = 1;
    drops[dropCount++] = d;
}
void addFloatText(float x, float y, const char* txt, float r = 1.0f, float g = 1.0f, float bcol = 1.0f);

int unlockRandomLockedBrick() {
    if (hasKey <= 0) {
        return 0;
    }

    int idxs[MAX_BRICKS];
    int cnt = 0;
    for (int i = 0; i < brickCount; i++) {
        if (bricks[i].alive && bricks[i].type == 7 && bricks[i].isLocked) {
            idxs[cnt++] = i;
        }
    }

    if (cnt <= 0) {
        return 0;
    }

    int pick = idxs[rand() % cnt];
    bricks[pick].isLocked = 0;
    hasKey--;
    addFloatText(bricks[pick].x + bricks[pick].w * 0.5f, bricks[pick].y + bricks[pick].h * 0.5f, "UNLOCK", 1.0f, 0.82f, 0.24f);
    playSfx(1, 80);
    return 1;
}

float clampf(float v, float lo, float hi) {
    if (v < lo) {
        return lo;
    }
    if (v > hi) {
        return hi;
    }
    return v;
}

float deg2rad(float deg) {
    return deg * 3.1415926535f / 180.0f;
}

float ballSpeedNow() {
    float s = baseBallSpeed * speedRamp;
    if (speedBoostTimer > 0.0f) {
        s = s * speedBoostMul;
    }
    if (iceBallSlowTimer > 0.0f) {
        s = s * 0.5f; // ice slows to 50%
    }
    if (fireBallSpeedTimer > 0.0f) {
        s = s * 1.8f; // fire speeds to 180%
    }
    return s;
}

// Feature: Ball physics & speed scaling (added by Akhi and Kawser)
// Akhi & Kawser: core ball speed computation and integration with power-ups

void drawText(float x, float y, const char* text, void* font = GLUT_BITMAP_HELVETICA_18) {
    glRasterPos2f(x, y);
    for (int i = 0; text[i] != '\0'; i++) {
        glutBitmapCharacter(font, text[i]);
    }
}

int textWidth(const char* text, void* font = GLUT_BITMAP_HELVETICA_18) {
    int w = 0;
    for (int i = 0; text[i] != '\0'; i++) {
        w += glutBitmapWidth(font, text[i]);
    }
    return w;
}

void drawTextCenter(float cx, float y, const char* text, void* font = GLUT_BITMAP_HELVETICA_18) {
    int w = textWidth(text, font);
    drawText(cx - w * 0.5f, y, text, font);
}

void drawRect(float x, float y, float w, float h) {
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

void drawCircle(float cx, float cy, float r) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= 32; i++) {
        float a = (2.0f * 3.1415926535f * i) / 32.0f;
        glVertex2f(cx + cosf(a) * r, cy + sinf(a) * r);
    }
    glEnd();
}

// Feature: Rendering & UI helpers (added by Kawser)
// Kawser: provided basic text/shape drawing utilities used by menus

int circleRectHit(float cx, float cy, float cr, float rx, float ry, float rw, float rh) {
    float nx = clampf(cx, rx, rx + rw);
    float ny = clampf(cy, ry, ry + rh);
    float dx = cx - nx;
    float dy = cy - ny;
    return (dx * dx + dy * dy) <= (cr * cr);
}

void sortScores() {
    for (int i = 0; i < highCount; i++) {
        for (int j = i + 1; j < highCount; j++) {
            int swapNow = 0;
            if (highscores[j].score > highscores[i].score) {
                swapNow = 1;
            } else if (highscores[j].score == highscores[i].score && highscores[j].timeSec < highscores[i].timeSec) {
                swapNow = 1;
            }
            if (swapNow) {
                ScoreLine t = highscores[i];
                highscores[i] = highscores[j];
                highscores[j] = t;
            }
        }
    }
}

void loadScores() {
    highCount = 0;
    FILE* fp = fopen(scorePath, "r");
    if (!fp) {
        return;
    }

    char line[256];
    while (highCount < MAX_SCORES && fgets(line, sizeof(line), fp)) {
        int s = 0;
        float t = 0.0f;
        int mapCategory = -1;
        int difficulty = -1;
        int parsed = sscanf(line, "%d %f %d %d", &s, &t, &mapCategory, &difficulty);
        if (parsed < 2) {
            continue;
        }
        highscores[highCount].score = s;
        highscores[highCount].timeSec = t;
        highscores[highCount].mapCategory = mapCategory;
        highscores[highCount].difficulty = difficulty;
        highCount++;
    }
    fclose(fp);
    sortScores();
}

void saveScores() {
#ifdef _WIN32
    _mkdir(".dist");
#else
    mkdir(".dist", 0755);
#endif

    FILE* fp = fopen(scorePath, "w");
    if (!fp) {
        return;
    }
    for (int i = 0; i < highCount; i++) {
        fprintf(fp, "%d %.2f %d %d\n", highscores[i].score, highscores[i].timeSec, highscores[i].mapCategory, highscores[i].difficulty);
    }
    fclose(fp);
}

// Feature: Scoring persistence (added by  Akhi)
//  Akhi: load/save highscores to disk and manage submission

void submitScore() {
    if (scoreSaved) {
        return;
    }
    scoreSaved = 1;

    if (highCount < MAX_SCORES) {
        highscores[highCount].score = score;
        highscores[highCount].timeSec = playTimeSec;
        highscores[highCount].mapCategory = selectedMapCategory;
        highscores[highCount].difficulty = selectedDifficulty;
        highCount++;
    } else {
        highscores[MAX_SCORES - 1].score = score;
        highscores[MAX_SCORES - 1].timeSec = playTimeSec;
        highscores[MAX_SCORES - 1].mapCategory = selectedMapCategory;
        highscores[MAX_SCORES - 1].difficulty = selectedDifficulty;
    }

    sortScores();
    if (highCount > MAX_SCORES) {
        highCount = MAX_SCORES;
    }
    saveScores();
}

const char* timeText(float sec) {
    static char buff[64];
    int total = (int)sec;
    int m = total / 60;
    int s = total % 60;
    sprintf(buff, "%d:%02d", m, s);
    return buff;
}

const char* difficultyText(int difficulty) {
    if (difficulty == DIFFICULTY_EASY) return "Easy";
    if (difficulty == DIFFICULTY_NORMAL) return "Normal";
    if (difficulty == DIFFICULTY_HARD) return "Hard";
    if (difficulty == DIFFICULTY_HARDEST) return "Hardest";
    return "Unknown";
}

const char* mapCategoryText(int category) {
    if (category == MAP_CAT_CLASSIC) return "Classic";
    if (category == MAP_CAT_CHECKERBOARD) return "Checkerboard";
    if (category == MAP_CAT_CIRCLE) return "Circle";
    if (category == MAP_CAT_PYRAMID) return "Pyramid";
    if (category == MAP_CAT_SPECIAL) return "Special";
    return "Unknown";
}

int getBestScoreForMap(int category, int* outDifficulty, float* outTime) {
    int bestScore = -1;
    int bestDifficulty = -1;
    float bestTime = 0.0f;
    for (int i = 0; i < highCount; i++) {
        if (highscores[i].mapCategory != category) {
            continue;
        }
        if (highscores[i].score > bestScore) {
            bestScore = highscores[i].score;
            bestDifficulty = highscores[i].difficulty;
            bestTime = highscores[i].timeSec;
        } else if (highscores[i].score == bestScore && bestScore >= 0 && highscores[i].timeSec < bestTime) {
            bestDifficulty = highscores[i].difficulty;
            bestTime = highscores[i].timeSec;
        }
    }
    if (outDifficulty) {
        *outDifficulty = bestDifficulty;
    }
    if (outTime) {
        *outTime = bestTime;
    }
    return bestScore;
}

void assignBrickType(Brick &b);

void setupBricks() {
    brickCount = 0;

    int rows = 6;
    int cols = 10;
    float gap = 8.0f;
    float bh = 28.0f;
    float margin = 56.0f;
    float startY = winH - 72.0f;
    float bw = (winW - 2.0f * margin - (cols - 1) * gap) / cols;

    float colors[6][3] = {
        {0.93f, 0.33f, 0.27f},
        {0.96f, 0.52f, 0.24f},
        {0.94f, 0.75f, 0.24f},
        {0.31f, 0.72f, 0.29f},
        {0.23f, 0.55f, 0.93f},
        {0.55f, 0.38f, 0.89f},
    };

    int mapInCategory = selectedMapVariant % 3;

    if (selectedMapCategory == MAP_CAT_CLASSIC) {
        if (mapInCategory == 1 || mapInCategory == 0) {
            for (int r = 0; r < rows; r++) {
                for (int c = 0; c < cols; c++) {
                    Brick b;
                    b.x = margin + c * (bw + gap);
                    b.y = startY - r * (bh + gap);
                    b.w = bw;
                    b.h = bh;
                    b.r = colors[r][0];
                    b.g = colors[r][1];
                    b.b = colors[r][2];
                    b.points = (rows - r) * 10;
                    b.alive = 1;
                    bricks[brickCount++] = b;
                }
            }
        } else if (mapInCategory == 2) {
            for (int r = 0; r < rows; r++) {
                for (int c = 0; c < cols; c++) {
                    if (r % 2 == 0 || c % 3 != 0) {
                        Brick b;
                        b.x = margin + c * (bw + gap);
                        b.y = startY - r * (bh + gap);
                        b.w = bw;
                        b.h = bh;
                        b.r = colors[r][0];
                        b.g = colors[r][1];
                        b.b = colors[r][2];
                        b.points = (rows - r) * 10;
                        b.alive = 1;
                        bricks[brickCount++] = b;
                    }
                }
            }
        }
    } else if (selectedMapCategory == MAP_CAT_CHECKERBOARD) {
        if (mapInCategory == 1 || mapInCategory == 0) {
            for (int r = 0; r < rows; r++) {
                for (int c = 0; c < cols; c++) {
                    if ((r + c) % 2 == 0) {
                        Brick b;
                        b.x = margin + c * (bw + gap);
                        b.y = startY - r * (bh + gap);
                        b.w = bw;
                        b.h = bh;
                        b.r = colors[r][0];
                        b.g = colors[r][1];
                        b.b = colors[r][2];
                        b.points = (rows - r) * 10;
                        b.alive = 1;
                        bricks[brickCount++] = b;
                    }
                }
            }
        } else if (mapInCategory == 2) {
            for (int r = 0; r < rows; r++) {
                for (int c = 0; c < cols; c++) {
                    if ((r + c) % 3 == 0) {
                        Brick b;
                        b.x = margin + c * (bw + gap);
                        b.y = startY - r * (bh + gap);
                        b.w = bw;
                        b.h = bh;
                        b.r = colors[r][0];
                        b.g = colors[r][1];
                        b.b = colors[r][2];
                        b.points = (rows - r) * 10;
                        b.alive = 1;
                        bricks[brickCount++] = b;
                    }
                }
            }
        }
    } else if (selectedMapCategory == MAP_CAT_CIRCLE) {
        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols; c++) {
                int dist = (c - 5) * (c - 5) + (r - 2) * (r - 2);
                int threshold = (mapInCategory == 1 || mapInCategory == 0) ? 9 : 7;
                if (dist <= threshold) {
                    Brick b;
                    b.x = margin + c * (bw + gap);
                    b.y = startY - r * (bh + gap);
                    b.w = bw;
                    b.h = bh;
                    b.r = colors[r][0];
                    b.g = colors[r][1];
                    b.b = colors[r][2];
                    b.points = (rows - r) * 10;
                    b.alive = 1;
                    bricks[brickCount++] = b;
                }
            }
        }
    } else if (selectedMapCategory == MAP_CAT_PYRAMID) {
        for (int r = 0; r < rows; r++) {
            int bricksInRow = cols - (r * 1);
            int startCol = r / 2;
            
            for (int c = 0; c < bricksInRow; c++) {
                if (startCol + c < cols) {
                    if (mapInCategory == 2) {
                        if (c % 2 == 0) {
                            Brick b;
                            b.x = margin + (startCol + c) * (bw + gap);
                            b.y = startY - r * (bh + gap);
                            b.w = bw;
                            b.h = bh;
                            b.r = colors[r][0];
                            b.g = colors[r][1];
                            b.b = colors[r][2];
                            b.points = (rows - r) * 10;
                            b.alive = 1;
                            bricks[brickCount++] = b;
                        }
                    } else {
                        Brick b;
                        b.x = margin + (startCol + c) * (bw + gap);
                        b.y = startY - r * (bh + gap);
                        b.w = bw;
                        b.h = bh;
                        b.r = colors[r][0];
                        b.g = colors[r][1];
                        b.b = colors[r][2];
                        b.points = (rows - r) * 10;
                        b.alive = 1;
                        bricks[brickCount++] = b;
                    }
                }
            }
        }
    } else if (selectedMapCategory == MAP_CAT_SPECIAL) {
        if (mapInCategory == 1 || mapInCategory == 0) {
            for (int r = 0; r < rows; r++) {
                for (int c = 0; c < cols; c++) {
                    int skip = 0;
                    if (r < 2 && (c < 2 || c >= 8)) skip = 1;
                    if (r >= 4 && c >= 3 && c <= 6) skip = 1;
                    
                    if (!skip) {
                        Brick b;
                        b.x = margin + c * (bw + gap);
                        b.y = startY - r * (bh + gap);
                        b.w = bw;
                        b.h = bh;
                        b.r = colors[r][0];
                        b.g = colors[r][1];
                        b.b = colors[r][2];
                        b.points = (rows - r) * 10;
                        b.alive = 1;
                        bricks[brickCount++] = b;
                    }
                }
            }
        } else if (mapInCategory == 2) {
            for (int r = 0; r < rows; r++) {
                for (int c = 0; c < cols; c++) {
                    if ((c + r) % 2 == 1 && (r + c * 2) % 3 == 0) {
                        Brick b;
                        b.x = margin + c * (bw + gap);
                        b.y = startY - r * (bh + gap);
                        b.w = bw;
                        b.h = bh;
                        b.r = colors[r][0];
                        b.g = colors[r][1];
                        b.b = colors[r][2];
                        b.points = (rows - r) * 10;
                        b.alive = 1;
                        bricks[brickCount++] = b;
                    }
                }
            }
        }
    }

    for (int i = 0; i < brickCount; i++) {
        assignBrickType(bricks[i]);
    }
}

void assignBrickType(Brick &b) {
    int r = rand() % 100;
    b.regenTimer = 0.0f;
    b.moveDir = 1.0f;
    b.isLocked = 0;
    b.linkedBrick = -1;
    
    if (r < 6) {
        b.type = 2; // explosive
        b.hp = 1;
        b.r = 0.85f; b.g = 0.28f; b.b = 0.18f;
    } else if (r < 15) {
        b.type = 1; // steel (2 hits)
        b.hp = 2;
        b.r = 0.6f; b.g = 0.6f; b.b = 0.65f;
    } else if (r < 20) {
        b.type = 3; // moving
        b.hp = 1;
        b.r = 0.7f; b.g = 0.95f; b.b = 0.4f;
    } else if (r < 24) {
        b.type = 4; // invisible
        b.hp = 1;
        b.r = 0.3f; b.g = 0.3f; b.b = 0.4f; // faint until hit
    } else if (r < 28) {
        b.type = 5; // ice
        b.hp = 1;
        b.r = 0.3f; b.g = 0.8f; b.b = 0.95f;
    } else if (r < 32) {
        b.type = 6; // fire
        b.hp = 1;
        b.r = 1.0f; b.g = 0.4f; b.b = 0.1f;
    } else if (r < 36) {
        b.type = 7; // locked
        b.hp = 1;
        b.isLocked = 1;
        b.r = 0.5f; b.g = 0.5f; b.b = 0.8f;
    } else if (r < 40) {
        b.type = 8; // regen
        b.hp = 1;
        b.regenTimer = 0.0f;
        b.r = 0.95f; b.g = 0.6f; b.b = 0.2f;
    } else if (r < 44) {
        b.type = 9; // teleport
        b.hp = 1;
        b.r = 0.75f; b.g = 0.35f; b.b = 0.95f;
    } else {
        b.type = 0; // normal
        b.hp = 1;
    }
}

void clearTopRows(int rowsToClear) {
    while (rowsToClear > 0) {
        float topY = -100000.0f;
        for (int i = 0; i < brickCount; i++) {
            if (bricks[i].alive && bricks[i].y > topY) {
                topY = bricks[i].y;
            }
        }
        if (topY < -99999.0f) {
            return;
        }

        for (int i = 0; i < brickCount; i++) {
            if (bricks[i].alive && fabs(bricks[i].y - topY) < 0.5f) {
                bricks[i].alive = 0;
                score += bricks[i].points;
            }
        }

        rowsToClear--;
    }
}

void assignBrickType(Brick &b);

void scaleBallVector() {
    if (ballAttached) {
        return;
    }

    float len = sqrtf(ballVX * ballVX + ballVY * ballVY);
    float target = ballSpeedNow();
    if (len < 0.001f) {
        ballVX = target * 0.55f;
        ballVY = target * 0.85f;
        return;
    }
    ballVX = (ballVX / len) * target;
    ballVY = (ballVY / len) * target;
}

// Feature: Ball vector normalization & speed enforcement (Kawser)
// Kawser: ensures ball maintains intended speed after collisions/power-ups

void resetBall() {
    paddleW = (wideTimer > 0.0f) ? paddleWideW : paddleNormalW;
    paddleX = winW * 0.5f;

    ballAttached = 1;
    ballX = paddleX;
    ballY = paddleY + paddleH * 0.5f + ballR + 2.0f;

    float a = deg2rad(72.0f);
    float s = ballSpeedNow();
    float sign = (rand() % 2) ? 1.0f : -1.0f;
    ballVX = cosf(a) * s * sign;
    ballVY = sinf(a) * s;
    for (int i = 0; i < MAX_EXTRA_BALLS; i++) extraBallActive[i] = 0;
}

// Feature: Ball reset and initial trajectory (added by  Kawser)
//  Kawser: places ball on paddle and sets initial velocity when attaching

void clearDrops() {
    for (int i = 0; i < dropCount; i++) {
        drops[i].active = 0;
    }
}

void newGame() {
    lives = 3;
    score = 0;
    playTimeSec = 0.0f;
    comboCount = 0;
    comboTimer = 0.0f;
    fireballTimer = 0.0f;
    iceBallSlowTimer = 0.0f;
    fireBallSpeedTimer = 0.0f;
    shakeTimer = 0.0f;
    shakeIntensity = 0.0f;
    hasKey = 0;
    keyAssistCooldown = 0.0f;
    speedRamp = 1.0f;
    speedRampNext = 12.0f;
    speedBoostTimer = 0.0f;
    wideTimer = 0.0f;
    shieldTimer = 0.0f;
    scoreSaved = 0;
    weatherTimer = 0.0f;
    lightningTimer = 0.0f;
    showLightning = 0;

    if (selectedDifficulty == DIFFICULTY_EASY) {
        baseBallSpeed = 280.0f;
        lives = 5;
    } else if (selectedDifficulty == DIFFICULTY_NORMAL) {
        baseBallSpeed = 340.0f;
        lives = 3;
    } else if (selectedDifficulty == DIFFICULTY_HARD) {
        baseBallSpeed = 420.0f;
        lives = 2;
    } else if (selectedDifficulty == DIFFICULTY_HARDEST) {
        baseBallSpeed = 500.0f;
        lives = 2;
    }

    dropCount = 0;
    for (int i = 0; i < MAX_EXTRA_BALLS; i++) extraBallActive[i] = 0;
    for (int i = 0; i < 32; i++) ftexts[i].life = 0.0f;
    for (int i = 0; i < 128; i++) parts[i].life = 0.0f;
    setupBricks();
    resetBall();

    canResume = 1;
    currentPage = PAGE_PLAY;
}

void bombExplosion(float bx, float by) {
    float explosionRadius = 100.0f;
    for (int i = 0; i < brickCount; i++) {
        if (!bricks[i].alive) {
            continue;
        }
        float dx = (bricks[i].x + bricks[i].w * 0.5f) - bx;
        float dy = (bricks[i].y + bricks[i].h * 0.5f) - by;
        float dist = sqrt(dx * dx + dy * dy);
        if (dist < explosionRadius) {
            bricks[i].alive = 0;
            score += bricks[i].points * 2;
        }
    }
}



void applyDrop(int type) {
    if (type == DROP_LIFE) {
        lives++;
        playSfx(1, 70);
    } else if (type == DROP_SPEED) {
        speedBoostTimer = speedBoostDur;
        scaleBallVector();
        playSfxFile("Game Music/11. Humm (power or item gain).mp3");
        playSfx(1, 70);
    } else if (type == DROP_WIDE) {
        wideTimer = wideDur;
        paddleW = paddleWideW;
        playSfxFile("Game Music/11. Humm (power or item gain).mp3");
        playSfx(1, 70);
    } else if (type == DROP_BOMB) {
        playSfxFile("Game Music/14. Padexplo.mp3");
        if (shieldTimer > 0.0f) {
            shieldTimer = 0.0f;
            playSfx(3, 70);
        } else {
            lives--;
            resetBall();
            playSfx(2, 70);
        }
    } else if (type == DROP_SHIELD) {
        shieldTimer = shieldDur;
        playSfxFile("Game Music/11. Humm (power or item gain).mp3");
        playSfx(1, 70);
    } else if (type == DROP_FIREBALL) {
        fireballTimer = fireballDur;
        addFloatText(ballX, ballY, "FIREBALL", 1.0f, 0.6f, 0.1f);
        playSfxFile("Game Music/11. Humm (power or item gain).mp3");
        playSfx(1, 70);
    } else if (type == DROP_MULTIBALL) {
        // spawn 2-5 extra balls with unique colors
        int spawnCount = 2 + (rand() % 4); // 2-5 balls
        extraBallMultiplierActive = 1;
        float colors[5][3] = {
            {1.0f, 0.5f, 0.2f},  // orange
            {0.3f, 0.8f, 1.0f},  // cyan
            {0.9f, 0.3f, 0.7f},  // magenta
            {0.4f, 1.0f, 0.3f},  // lime
            {1.0f, 0.9f, 0.2f}   // yellow
        };
        for (int i = 0; i < MAX_EXTRA_BALLS && i < spawnCount; i++) {
            if (!extraBallActive[i]) {
                extraBallActive[i] = 1;
                extraBallX[i] = ballX + (i+1) * 12.0f * ((i%2)?-1:1);
                extraBallY[i] = ballY + 8.0f;
                float ang = deg2rad(30.0f + i * 25.0f);
                float s = ballSpeedNow();
                extraBallVX[i] = cosf(ang) * s * ((i%2)?-1.0f:1.0f);
                extraBallVY[i] = sinf(ang) * s;
                extraBallR_val[i] = colors[i][0];
                extraBallG_val[i] = colors[i][1];
                extraBallB_val[i] = colors[i][2];
            }
        }
        char mb[64]; sprintf(mb, "MULTI x%d", spawnCount);
        addFloatText(ballX, ballY, mb, 0.9f,0.6f,0.95f);
        playSfxFile("Game Music/11. Humm (power or item gain).mp3");
        playSfx(1, 70);
    } else if (type == DROP_ROW_CLEAR) {
        clearTopRows((rand() % 2) + 1);
        playSfxFile("Game Music/11. Humm (power or item gain).mp3");
        playSfx(1, 70);
    } else if (type == DROP_KEY) {
        if (hasKey < 9) {
            hasKey++;
        }
        char kbuf[64];
        sprintf(kbuf, "KEY +1 (%d)", hasKey);
        addFloatText(ballX, ballY, kbuf, 1.0f,0.85f,0.2f);
        playSfx(1, 70);
    }
}

// Feature: Power-up application (added by Kawser)
//  Kawser: applies the effect of drops (life, speed boost, wide paddle)

void spawnDrop(float x, float y) {
    if (dropCount >= MAX_DROPS) {
        return;
    }

    int chance = rand() % 100;
    if (chance > 72) {
        return;
    }

    Drop d;
    d.x = x;
    d.y = y;
    d.size = 18.0f;
    d.vy = 150.0f;

    int lockedAlive = countLockedBricksAlive();
    int needKeys = lockedAlive - hasKey;
    int keyBias = 0;
    if (needKeys >= 3) {
        keyBias = 45;
    } else if (needKeys == 2) {
        keyBias = 30;
    } else if (needKeys == 1) {
        keyBias = 18;
    }

    if ((rand() % 100) < keyBias) {
        d.type = DROP_KEY;
        d.active = 1;
        drops[dropCount++] = d;
        return;
    }

    int roll = rand() % 100;
    if (roll < 16) {
        d.type = DROP_LIFE;
    } else if (roll < 30) {
        d.type = DROP_SPEED;
    } else if (roll < 43) {
        d.type = DROP_WIDE;
    } else if (roll < 57) {
        d.type = DROP_ROW_CLEAR;
    } else if (roll < 68) {
        d.type = DROP_SHIELD;
    } else if (roll < 76) {
        d.type = DROP_BOMB;
    } else if (roll < 86) {
        d.type = DROP_FIREBALL;
    } else if (roll < 97) {
        d.type = DROP_MULTIBALL;
    } else {
        d.type = DROP_KEY;
    }
    d.active = 1;
    drops[dropCount++] = d;
}

// Feature: Drop spawning (added by  Akhi)
//  Akhi: logic to randomly spawn drops when bricks are destroyed

void updateDrops(float dt) {
    for (int i = 0; i < dropCount; i++) {
        if (!drops[i].active) {
            continue;
        }

        drops[i].y -= drops[i].vy * dt;

        float rx = paddleX - paddleW * 0.5f;
        float ry = paddleY - paddleH * 0.5f;
        if (circleRectHit(drops[i].x, drops[i].y, drops[i].size * 0.5f, rx, ry, paddleW, paddleH)) {
            drops[i].active = 0;
            applyDrop(drops[i].type);
            continue;
        }

        if (drops[i].y + drops[i].size < 0.0f) {
            drops[i].active = 0;
        }
    }
}

void movePaddle(float dt) {
    if (moveLeft) {
        paddleX -= paddleSpeed * dt;
    }
    if (moveRight) {
        paddleX += paddleSpeed * dt;
    }
    paddleX = clampf(paddleX, paddleW * 0.5f, winW - paddleW * 0.5f);
}

void paddleBounce() {
    float rx = paddleX - paddleW * 0.5f;
    float ry = paddleY - paddleH * 0.5f;
    if (!circleRectHit(ballX, ballY, ballR, rx, ry, paddleW, paddleH)) {
        return;
    }

    if (ballVY >= 0.0f) {
        return;
    }

    ballY = ry + paddleH + ballR + 0.5f;

    float rel = (ballX - paddleX) / (paddleW * 0.5f);
    rel = clampf(rel, -1.0f, 1.0f);

    float ang = deg2rad(90.0f - rel * 65.0f);
    float spd = ballSpeedNow();
    ballVX = cosf(ang) * spd;
    ballVY = sinf(ang) * spd;
    playSfxFile("Game Music/14. Padexplo.mp3");
    playSfx(0, 35);
}

void addFloatText(float x, float y, const char* txt, float r, float g, float bcol) {
    for (int i = 0; i < 32; i++) {
        if (ftexts[i].life <= 0.0f) {
            ftexts[i].x = x;
            ftexts[i].y = y;
            ftexts[i].vy = 28.0f;
            ftexts[i].life = 1.2f;
            strncpy(ftexts[i].txt, txt, sizeof(ftexts[i].txt)-1);
            ftexts[i].txt[sizeof(ftexts[i].txt)-1] = '\0';
            ftexts[i].r = r; ftexts[i].g = g; ftexts[i].b = bcol;
            return;
        }
    }
}

void updateFloatTexts(float dt) {
    for (int i = 0; i < 32; i++) {
        if (ftexts[i].life > 0.0f) {
            ftexts[i].life -= dt;
            ftexts[i].y += ftexts[i].vy * dt;
        }
    }
}

void drawFloatTexts() {
    for (int i = 0; i < 32; i++) {
        if (ftexts[i].life > 0.0f) {
            glColor4f(ftexts[i].r, ftexts[i].g, ftexts[i].b, clampf(ftexts[i].life, 0.0f, 1.0f));
            drawTextCenter(ftexts[i].x, ftexts[i].y, ftexts[i].txt, GLUT_BITMAP_HELVETICA_12);
        }
    }
}

void spawnParticles(float x, float y, int count) {
    for (int i = 0; i < 128 && count > 0; i++) {
        if (parts[i].life <= 0.0f) {
            parts[i].life = 0.7f + ((rand()%100)/200.0f);
            parts[i].x = x + (rand()%40 - 20);
            parts[i].y = y + (rand()%40 - 20);
            parts[i].vx = (rand()%200 - 100) * 0.6f;
            parts[i].vy = (rand()%200 - 100) * 0.6f;
            parts[i].r = 1.0f; parts[i].g = 0.6f; parts[i].b = 0.2f;
            count--;
        }
    }
}

void updateParticles(float dt) {
    for (int i = 0; i < 128; i++) {
        if (parts[i].life > 0.0f) {
            parts[i].life -= dt;
            parts[i].x += parts[i].vx * dt;
            parts[i].y += parts[i].vy * dt;
        }
    }
}

void drawParticles() {
    for (int i = 0; i < 128; i++) {
        if (parts[i].life > 0.0f) {
            glColor4f(parts[i].r, parts[i].g, parts[i].b, clampf(parts[i].life,0.0f,1.0f));
            drawCircle(parts[i].x, parts[i].y, 3.0f);
        }
    }
}

int hitBrickForBall(float *bxp, float *byp, float *bvx, float *bvy, Brick* b, float usedBallR) {
    if (!b->alive) return 0;
    if (!circleRectHit(*bxp, *byp, usedBallR, b->x, b->y, b->w, b->h)) return 0;

    float nx = clampf(*bxp, b->x, b->x + b->w);
    float ny = clampf(*byp, b->y, b->y + b->h);
    float dx = *bxp - nx;
    float dy = *byp - ny;

    // Invisible brick becomes visible on hit
    if (b->type == 4) {
        b->r = 0.8f; b->g = 0.95f; b->b = 0.3f; // make it visible
    }

    // If not fireball, reflect
    int fireActive = (fireballTimer > 0.0f) ? 1 : 0;
    if (!fireActive) {
        if (fabs(dx) > fabs(dy)) {
            *bvx = -*bvx;
        } else {
            *bvy = -*bvy;
        }
    }

    // Check if locked and no key
    if (b->type == 7 && b->isLocked && !hasKey) {
        addFloatText(b->x + b->w * 0.5f, b->y + b->h * 0.5f, "LOCKED", 0.7f,0.7f,0.95f);
        if (keyAssistCooldown <= 0.0f && (rand() % 100) < 65) {
            spawnForcedDrop(b->x + b->w * 0.5f, b->y + b->h * 0.5f, DROP_KEY);
            keyAssistCooldown = 1.2f;
        }
        playSfx(2, 120);
        return 1; // hit but don't damage
    }

    // Apply ice/fire effects
    if (b->type == 5) { // ice
        iceBallSlowTimer = iceBallSlowDur;
        scaleBallVector();
        addFloatText(b->x + b->w * 0.5f, b->y + b->h * 0.5f, "SLOW", 0.3f,0.8f,0.95f);
    }
    if (b->type == 6) { // fire
        fireBallSpeedTimer = fireBallSpeedDur;
        scaleBallVector();
        addFloatText(b->x + b->w * 0.5f, b->y + b->h * 0.5f, "FAST", 1.0f,0.4f,0.1f);
    }

    // Damage brick
    b->hp -= 1;
    if (b->hp <= 0) {
        // Regen brick: set timer instead of dying
        if (b->type == 8) {
            b->hp = 1;
            b->regenTimer = 4.0f; // respawn after 4 seconds
            b->alive = 0;
        } else {
            b->alive = 0;
        }

        // Explosive brick
        if (b->type == 2) {
            bombExplosion(b->x + b->w * 0.5f, b->y + b->h * 0.5f);
            spawnParticles(b->x + b->w * 0.5f, b->y + b->h * 0.5f, 16);
            shakeTimer = 0.5f; shakeIntensity = 8.0f;
        }

        // Locked brick - consume key
        if (b->type == 7 && b->isLocked && hasKey > 0) {
            hasKey--;
            b->isLocked = 0;
            addFloatText(b->x + b->w * 0.5f, b->y + b->h * 0.5f, "UNLOCKED", 1.0f,0.8f,0.2f);
        }

        // scoring with combo multipliers
        comboCount++;
        comboTimer = comboDecay;

        int bonusMul = 1;
        if (comboCount >= 2 && comboCount < 5) bonusMul = 2;
        if (comboCount >= 10) {
            score += 200; // big bonus
            shakeTimer = 0.8f; shakeIntensity = 12.0f;
        }

        // apply multiball score multiplier
        int finalScore = (int)(b->points * bonusMul * (extraBallMultiplierActive ? extraBallScoreMul : 1.0f));
        score += finalScore;
        char tbuf[64]; sprintf(tbuf, "+%d", finalScore);
        addFloatText(b->x + b->w * 0.5f, b->y + b->h * 0.5f, tbuf, 1.0f, 0.95f, 0.3f);

        // Play explosion sound if fireball active
        if (fireActive) {
            playSfxFile("Game Music/26. Xplosht1.mp3");
        }

        spawnDrop(b->x + b->w * 0.5f, b->y + b->h * 0.5f);
        playSfx(0, 40);
    } else {
        // still alive (steel, etc), smaller score
        score += (b->points/2);
        addFloatText(b->x + b->w * 0.5f, b->y + b->h * 0.5f, "HIT", 0.9f,0.9f,0.95f);
        comboCount++;
        comboTimer = comboDecay;
        playSfx(0, 50);
    }

    // reward fireball at 5 combo
    if (comboCount >= 5 && fireballTimer <= 0.0f) {
        fireballTimer = fireballDur;
        addFloatText(*bxp, *byp, "FIREBALL!", 1.0f,0.5f,0.1f);
    }

    return 1;
}

int hitBrick(Brick* b) {
    return hitBrickForBall(&ballX, &ballY, &ballVX, &ballVY, b, ballR);
}

void updateTimers(float dt) {
    if (keyAssistCooldown > 0.0f) {
        keyAssistCooldown -= dt;
        if (keyAssistCooldown < 0.0f) {
            keyAssistCooldown = 0.0f;
        }
    }

    if (wideTimer > 0.0f) {
        wideTimer -= dt;
        if (wideTimer <= 0.0f) {
            wideTimer = 0.0f;
            paddleW = paddleNormalW;
            paddleX = clampf(paddleX, paddleW * 0.5f, winW - paddleW * 0.5f);
        }
    }

    if (speedBoostTimer > 0.0f) {
        speedBoostTimer -= dt;
        if (speedBoostTimer <= 0.0f) {
            speedBoostTimer = 0.0f;
            scaleBallVector();
        }
    }
    
    if (shieldTimer > 0.0f) {
        shieldTimer -= dt;
        if (shieldTimer <= 0.0f) {
            shieldTimer = 0.0f;
        }
    }

    if (fireballTimer > 0.0f) {
        fireballTimer -= dt;
        if (fireballTimer <= 0.0f) fireballTimer = 0.0f;
    }

    if (iceBallSlowTimer > 0.0f) {
        iceBallSlowTimer -= dt;
        if (iceBallSlowTimer <= 0.0f) {
            iceBallSlowTimer = 0.0f;
            scaleBallVector();
        }
    }

    if (fireBallSpeedTimer > 0.0f) {
        fireBallSpeedTimer -= dt;
        if (fireBallSpeedTimer <= 0.0f) {
            fireBallSpeedTimer = 0.0f;
            scaleBallVector();
        }
    }
}

int allBricksBroken() {
    for (int i = 0; i < brickCount; i++) {
        if (bricks[i].alive) {
            return 0;
        }
    }
    return 1;
}

void updateGame(float dt) {
    playTimeSec += dt;

    // Update weather effects
    weatherTimer += dt;
    if (selectedWeather == WEATHER_STORMY) {
        if (weatherTimer > 2.0f) {
            lightningTimer = lightningDuration;
            showLightning = 1;
            weatherTimer = 0.0f;
        }
        if (lightningTimer > 0.0f) {
            lightningTimer -= dt;
            if (lightningTimer <= 0.0f) {
                showLightning = 0;
            }
        }
    }

    while (playTimeSec >= speedRampNext) {
        speedRampNext += 12.0f;
        speedRamp += speedRampStep;
        if (speedRamp > speedRampMax) {
            speedRamp = speedRampMax;
        }
        scaleBallVector();
    }

    updateTimers(dt);
    movePaddle(dt);

    // special brick behaviors: moving and regen
    for (int i = 0; i < brickCount; i++) {
        if (bricks[i].type == 3 && bricks[i].alive) {
            bricks[i].x += bricks[i].moveDir * 70.0f * dt;
            if (bricks[i].x < 20.0f) {
                bricks[i].x = 20.0f;
                bricks[i].moveDir = 1.0f;
            }
            if (bricks[i].x + bricks[i].w > winW - 20.0f) {
                bricks[i].x = winW - 20.0f - bricks[i].w;
                bricks[i].moveDir = -1.0f;
            }
        }

        if (bricks[i].type == 8 && !bricks[i].alive && bricks[i].regenTimer > 0.0f) {
            bricks[i].regenTimer -= dt;
            if (bricks[i].regenTimer <= 0.0f) {
                bricks[i].alive = 1;
                bricks[i].hp = 1;
                bricks[i].regenTimer = 0.0f;
                addFloatText(bricks[i].x + bricks[i].w * 0.5f, bricks[i].y + bricks[i].h * 0.5f, "REGEN", 0.95f,0.7f,0.2f);
            }
        }
    }

    // update combo decay
    if (comboTimer > 0.0f) {
        comboTimer -= dt;
        if (comboTimer <= 0.0f) {
            comboTimer = 0.0f;
            comboCount = 0;
        }
    }

    // update particles and floating texts
    updateParticles(dt);
    updateFloatTexts(dt);
    if (shakeTimer > 0.0f) {
        shakeTimer -= dt;
        if (shakeTimer <= 0.0f) { shakeTimer = 0.0f; shakeIntensity = 0.0f; }
    }

    if (selectedDifficulty == DIFFICULTY_HARDEST) {
        float fallSpeed = 60.0f;
        for (int i = 0; i < brickCount; i++) {
            if (bricks[i].alive) {
                bricks[i].y -= fallSpeed * dt;
            }
        }

        float paddleTop = paddleY + paddleH * 0.5f;
        float paddleBottom = paddleY - paddleH * 0.5f;
        for (int i = 0; i < brickCount; i++) {
            if (!bricks[i].alive) {
                continue;
            }
            float brickBottom = bricks[i].y;
            float brickTop = bricks[i].y + bricks[i].h;
            if (brickTop >= paddleBottom && brickBottom <= paddleTop) {
#ifdef _WIN32
                logMusic("[GAME] Ball fell - triggering PAGE_OVER");
#endif
                currentPage = PAGE_OVER;
                canResume = 0;
                submitScore();
                return;
            }
        }
    }

    if (ballAttached) {
        ballX = paddleX;
        ballY = paddleY + paddleH * 0.5f + ballR + 2.0f;
        updateDrops(dt);
        return;
    }

    ballX += ballVX * dt;
    ballY += ballVY * dt;

    if (ballX - ballR < 0.0f) {
        ballX = ballR;
        ballVX = fabs(ballVX);
    } else if (ballX + ballR > winW) {
        ballX = winW - ballR;
        ballVX = -fabs(ballVX);
    }

    if (ballY + ballR > winH) {
        ballY = winH - ballR;
        ballVY = -fabs(ballVY);
    }

    paddleBounce();

    // primary ball brick collisions
    for (int i = 0; i < brickCount; i++) {
        if (hitBrickForBall(&ballX, &ballY, &ballVX, &ballVY, &bricks[i], ballR)) {
            // teleport brick sends ball to another random alive brick position
            if (bricks[i].type == 9) {
                int tries = 12;
                while (tries-- > 0) {
                    int t = rand() % brickCount;
                    if (bricks[t].alive && t != i) {
                        ballX = bricks[t].x + bricks[t].w * 0.5f;
                        ballY = bricks[t].y + bricks[t].h * 0.5f;
                        addFloatText(ballX, ballY, "TELEPORT", 0.8f,0.5f,1.0f);
                        break;
                    }
                }
            }
            ballX += ballVX * dt * 0.2f;
            ballY += ballVY * dt * 0.2f;
            break;
        }
    }

    // update extra balls
    for (int bi = 0; bi < MAX_EXTRA_BALLS; bi++) {
        if (!extraBallActive[bi]) continue;
        extraBallX[bi] += extraBallVX[bi] * dt;
        extraBallY[bi] += extraBallVY[bi] * dt;

        if (extraBallX[bi] - extraBallR < 0.0f) {
            extraBallX[bi] = extraBallR;
            extraBallVX[bi] = fabs(extraBallVX[bi]);
        } else if (extraBallX[bi] + extraBallR > winW) {
            extraBallX[bi] = winW - extraBallR;
            extraBallVX[bi] = -fabs(extraBallVX[bi]);
        }
        if (extraBallY[bi] + extraBallR > winH) {
            extraBallY[bi] = winH - extraBallR;
            extraBallVY[bi] = -fabs(extraBallVY[bi]);
        }

        // paddle collision for extra balls
        float rx = paddleX - paddleW * 0.5f;
        float ry = paddleY - paddleH * 0.5f;
        if (circleRectHit(extraBallX[bi], extraBallY[bi], extraBallR, rx, ry, paddleW, paddleH)) {
            if (extraBallVY[bi] < 0.0f) {
                extraBallY[bi] = ry + paddleH + extraBallR + 0.5f;
                float rel = (extraBallX[bi] - paddleX) / (paddleW * 0.5f);
                rel = clampf(rel, -1.0f, 1.0f);
                float ang = deg2rad(90.0f - rel * 65.0f);
                float spd = ballSpeedNow();
                extraBallVX[bi] = cosf(ang) * spd;
                extraBallVY[bi] = sinf(ang) * spd;
            }
        }

        // brick collisions for extra balls
        for (int i = 0; i < brickCount; i++) {
            if (hitBrickForBall(&extraBallX[bi], &extraBallY[bi], &extraBallVX[bi], &extraBallVY[bi], &bricks[i], extraBallR)) {
                extraBallX[bi] += extraBallVX[bi] * dt * 0.2f;
                extraBallY[bi] += extraBallVY[bi] * dt * 0.2f;
                break;
            }
        }

        if (extraBallY[bi] + extraBallR < 0.0f) {
            extraBallActive[bi] = 0;
        }
    }

    // check if all extra balls are gone
    int anyActive = 0;
    for (int i = 0; i < MAX_EXTRA_BALLS; i++) {
        if (extraBallActive[i]) { anyActive = 1; break; }
    }
    if (!anyActive && extraBallMultiplierActive) {
        extraBallMultiplierActive = 0;
    }

    updateDrops(dt);

    if (ballY + ballR < 0.0f) {
        if (shieldTimer > 0.0f) {
            shieldTimer = 0.0f;
            ballY = 100.0f;
            ballVY = -ballVY;
        } else {
            lives--;
            clearDrops();
            if (lives <= 0) {
#ifdef _WIN32
                logMusic("[GAME] Lives exhausted - triggering PAGE_OVER");
#endif
                currentPage = PAGE_OVER;
                canResume = 0;
                submitScore();
                return;
            }
            resetBall();
        }
    }

    if (allBricksBroken()) {
        currentPage = PAGE_WIN;
        canResume = 0;
        submitScore();
    }
}

void drawBackground() {
    // Base color based on theme
    if (selectedTheme == THEME_NATURE) {
        if (selectedWeather == WEATHER_SUNNY) {
            glBegin(GL_QUADS);
            glColor3f(0.53f, 0.81f, 0.92f);
            glVertex2f(0.0f, (float)winH * 0.5f);
            glVertex2f((float)winW, (float)winH * 0.5f);
            glColor3f(0.34f, 0.68f, 0.24f);
            glVertex2f((float)winW, 0.0f);
            glVertex2f(0.0f, 0.0f);
            glEnd();
            
            glColor4f(1.0f, 1.0f, 0.8f, 0.4f);
            drawCircle(winW * 0.85f, winH * 0.8f, 60.0f);
            
            // Draw trees
            for (int i = 0; i < 4; i++) {
                float tx = 100.0f + i * 250.0f;
                glColor3f(0.4f, 0.25f, 0.1f);
                drawRect(tx - 8.0f, 80.0f, 16.0f, 60.0f);
                glColor3f(0.2f, 0.6f, 0.2f);
                drawCircle(tx, 140.0f, 35.0f);
            }
            
            // Draw grass patches
            glColor3f(0.25f, 0.5f, 0.15f);
            for (int i = 0; i < 6; i++) {
                float gx = i * 180.0f;
                drawRect(gx, 20.0f, 160.0f, 25.0f);
            }
            
            // Draw clouds
            glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
            drawCircle(150.0f, 600.0f, 30.0f);
            drawCircle(200.0f, 590.0f, 40.0f);
            drawCircle(250.0f, 600.0f, 28.0f);
            drawCircle(700.0f, 620.0f, 35.0f);
            drawCircle(780.0f, 610.0f, 45.0f);
            drawCircle(850.0f, 620.0f, 32.0f);
            
        } else if (selectedWeather == WEATHER_RAINY) {
            glBegin(GL_QUADS);
            glColor3f(0.3f, 0.4f, 0.5f);
            glVertex2f(0.0f, (float)winH * 0.5f);
            glVertex2f((float)winW, (float)winH * 0.5f);
            glColor3f(0.15f, 0.35f, 0.1f);
            glVertex2f((float)winW, 0.0f);
            glVertex2f(0.0f, 0.0f);
            glEnd();
            
            // Draw trees in rain
            for (int i = 0; i < 4; i++) {
                float tx = 100.0f + i * 250.0f;
                glColor3f(0.3f, 0.2f, 0.08f);
                drawRect(tx - 8.0f, 80.0f, 16.0f, 60.0f);
                glColor3f(0.15f, 0.45f, 0.15f);
                drawCircle(tx, 140.0f, 35.0f);
            }
            
            // Rain drops
            glColor4f(0.7f, 0.8f, 0.95f, 0.4f);
            for (int i = 0; i < 40; i++) {
                float rx = (float)(rand() % winW);
                float ry = (float)(rand() % winH);
                drawRect(rx, ry, 2.0f, 8.0f);
            }
            
        } else if (selectedWeather == WEATHER_STORMY) {
            glBegin(GL_QUADS);
            glColor3f(0.15f, 0.15f, 0.2f);
            glVertex2f(0.0f, (float)winH * 0.5f);
            glVertex2f((float)winW, (float)winH * 0.5f);
            glColor3f(0.08f, 0.15f, 0.05f);
            glVertex2f((float)winW, 0.0f);
            glVertex2f(0.0f, 0.0f);
            glEnd();
            
            if (showLightning) {
                glColor4f(1.0f, 1.0f, 1.0f, 0.8f);
                drawRect(0.0f, 0.0f, (float)winW, (float)winH);
            }
            
        } else if (selectedWeather == WEATHER_NIGHT) {
            glBegin(GL_QUADS);
            glColor3f(0.02f, 0.02f, 0.08f);
            glVertex2f(0.0f, (float)winH);
            glVertex2f((float)winW, (float)winH);
            glColor3f(0.04f, 0.04f, 0.12f);
            glVertex2f((float)winW, 0.0f);
            glVertex2f(0.0f, 0.0f);
            glEnd();
            
            // Draw night stars and silhouettes
            glColor4f(0.8f, 0.9f, 1.0f, 0.6f);
            for (int i = 0; i < 30; i++) {
                float sx = (float)(rand() % winW);
                float sy = (float)(winH * 0.5f + rand() % (int)(winH * 0.5f));
                drawCircle(sx, sy, 1.5f);
            }
            
            // Dark tree silhouettes
            glColor3f(0.05f, 0.05f, 0.08f);
            for (int i = 0; i < 4; i++) {
                float tx = 100.0f + i * 250.0f;
                drawRect(tx - 8.0f, 80.0f, 16.0f, 60.0f);
                drawCircle(tx, 140.0f, 35.0f);
            }
        }
        
    } else if (selectedTheme == THEME_CITY) {
        glBegin(GL_QUADS);
        glColor3f(0.2f, 0.2f, 0.25f);
        glVertex2f(0.0f, (float)winH);
        glVertex2f((float)winW, (float)winH);
        glColor3f(0.08f, 0.1f, 0.15f);
        glVertex2f((float)winW, 0.0f);
        glVertex2f(0.0f, 0.0f);
        glEnd();
        
        // Draw buildings
        float buildHeights[] = {250.0f, 200.0f, 280.0f, 150.0f, 220.0f};
        for (int i = 0; i < 5; i++) {
            float bx = i * 200.0f;
            glColor3f(0.25f, 0.25f, 0.28f);
            drawRect(bx, 0.0f, 200.0f, buildHeights[i]);
            
            // Windows
            glColor3f(0.95f, 0.85f, 0.3f);
            for (int y = 0; y < (int)(buildHeights[i] / 30.0f); y++) {
                for (int x = 0; x < 5; x++) {
                    float wx = bx + 20.0f + x * 35.0f;
                    float wy = buildHeights[i] - 25.0f - y * 30.0f;
                    drawRect(wx, wy, 15.0f, 15.0f);
                }
            }
        }
        
        // Street
        glColor3f(0.12f, 0.12f, 0.15f);
        drawRect(0.0f, 0.0f, (float)winW, 25.0f);
        
        // Street markings
        glColor3f(0.4f, 0.4f, 0.4f);
        for (int i = 0; i < 10; i++) {
            drawRect(i * 100.0f, 10.0f, 50.0f, 4.0f);
        }
        
        if (selectedWeather == WEATHER_STORMY && showLightning) {
            glColor4f(1.0f, 1.0f, 0.8f, 0.6f);
            drawRect(0.0f, 0.0f, (float)winW, (float)winH);
        }
        
    } else if (selectedTheme == THEME_SPACE) {
        glBegin(GL_QUADS);
        glColor3f(0.01f, 0.01f, 0.05f);
        glVertex2f(0.0f, (float)winH);
        glVertex2f((float)winW, (float)winH);
        glColor3f(0.02f, 0.01f, 0.08f);
        glVertex2f((float)winW, 0.0f);
        glVertex2f(0.0f, 0.0f);
        glEnd();
        
        // Draw planets
        glColor3f(0.8f, 0.4f, 0.2f);
        drawCircle(150.0f, 550.0f, 50.0f);
        
        glColor3f(0.3f, 0.6f, 0.9f);
        drawCircle(800.0f, 480.0f, 40.0f);
        
        glColor3f(0.9f, 0.7f, 0.3f);
        drawCircle(950.0f, 600.0f, 35.0f);
        
        glColor3f(0.5f, 0.2f, 0.7f);
        drawCircle(300.0f, 150.0f, 45.0f);
        
        // Draw asteroids
        glColor3f(0.6f, 0.6f, 0.6f);
        for (int i = 0; i < 15; i++) {
            float ax = 200.0f + i * 50.0f;
            float ay = 250.0f + (float)(rand() % 200);
            float size = 8.0f + (float)(rand() % 12);
            drawCircle(ax, ay, size);
        }
        
        // Draw nebula glow
        glColor4f(1.0f, 0.3f, 0.8f, 0.15f);
        drawCircle(winW * 0.5f, winH * 0.5f, 300.0f);
        
        // Draw stars
        glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
        for (int i = 0; i < 50; i++) {
            float sx = (float)(rand() % winW);
            float sy = (float)(rand() % winH);
            drawCircle(sx, sy, 1.0f);
        }
        
    } else if (selectedTheme == THEME_NEON) {
        glBegin(GL_QUADS);
        glColor3f(0.05f, 0.0f, 0.1f);
        glVertex2f(0.0f, (float)winH);
        glVertex2f((float)winW, (float)winH);
        glColor3f(0.08f, 0.0f, 0.15f);
        glVertex2f((float)winW, 0.0f);
        glVertex2f(0.0f, 0.0f);
        glEnd();
        
        glLineWidth(2.0f);
        glColor4f(0.0f, 1.0f, 1.0f, 0.2f);
        for (int i = 0; i < 8; i++) {
            float y = 40.0f + i * 80.0f;
            glBegin(GL_LINE_STRIP);
            glVertex2f(0.0f, y);
            glVertex2f((float)winW, y);
            glEnd();
        }
        
        for (int i = 0; i < 12; i++) {
            float x = 80.0f + i * 75.0f;
            glBegin(GL_LINE_STRIP);
            glVertex2f(x, 0.0f);
            glVertex2f(x, (float)winH);
            glEnd();
        }
        
        // Neon glowing elements
        glColor4f(0.0f, 1.0f, 1.0f, 0.4f);
        for (int i = 0; i < 5; i++) {
            drawCircle(150.0f + i * 180.0f, 350.0f, 40.0f);
        }
        
        glColor4f(1.0f, 0.0f, 1.0f, 0.3f);
        for (int i = 0; i < 5; i++) {
            drawCircle(250.0f + i * 180.0f, 250.0f, 35.0f);
        }
    }

    glColor4f(1.0f, 1.0f, 1.0f, 0.06f);
    for (int i = 0; i < 14; i++) {
        float y = 40.0f + i * 45.0f;
        drawRect(0.0f, y, (float)winW, 1.2f);
    }
}

void drawHUD() {
    char buff[128];
    glColor3f(0.95f, 0.97f, 1.0f);

    sprintf(buff, "Time: %s", timeText(playTimeSec));
    drawText(24.0f, winH - 34.0f, buff);

    sprintf(buff, "Score: %d", score);
    drawText(210.0f, winH - 34.0f, buff);

    sprintf(buff, "Lives: %d", lives);
    drawText(400.0f, winH - 34.0f, buff);

    if (wideTimer > 0.0f) {
        sprintf(buff, "Wide Paddle: %ds", (int)ceil(wideTimer));
        drawText(winW - 520.0f, winH - 34.0f, buff);
    }

    if (speedBoostTimer > 0.0f) {
        sprintf(buff, "Speed Boost: %ds", (int)ceil(speedBoostTimer));
        drawText(winW - 520.0f, winH - 58.0f, buff);
    }
    
    if (shieldTimer > 0.0f) {
        glColor3f(0.2f, 0.6f, 0.95f);
        sprintf(buff, "Shield: %ds", (int)ceil(shieldTimer));
        drawText(winW - 260.0f, winH - 34.0f, buff);
    }

    if (selectedDifficulty == DIFFICULTY_HARDEST) {
        glColor3f(1.0f, 0.35f, 0.35f);
        drawText(winW - 260.0f, winH - 82.0f, "HARDEST: BRICKS FALL", GLUT_BITMAP_HELVETICA_12);
    }

    if (comboCount > 1) {
        glColor3f(1.0f, 0.6f, 0.12f);
        char cb[64]; sprintf(cb, "COMBO x%d", comboCount);
        drawText(winW - 260.0f, winH - 58.0f, cb);
    }

    if (extraBallMultiplierActive) {
        glColor3f(0.9f, 0.5f, 0.8f);
        char mb[64]; sprintf(mb, "MULTI BALLS: x%.1f", extraBallScoreMul);
        drawText(winW - 420.0f, winH - 82.0f, mb);
    }

    if (hasKey) {
        glColor3f(1.0f, 0.85f, 0.2f);
        char kline[96];
        sprintf(kline, "KEYS: %d (hit locked brick or press K)", hasKey);
        drawText(24.0f, winH - 82.0f, kline, GLUT_BITMAP_HELVETICA_12);
    }

    if (iceBallSlowTimer > 0.0f) {
        glColor3f(0.3f, 0.8f, 0.95f);
        sprintf(buff, "Ball Slow: %ds", (int)ceil(iceBallSlowTimer));
        drawText(24.0f, winH - 58.0f, buff, GLUT_BITMAP_HELVETICA_12);
    }

    if (fireBallSpeedTimer > 0.0f) {
        glColor3f(1.0f, 0.4f, 0.1f);
        sprintf(buff, "Ball Fast: %ds", (int)ceil(fireBallSpeedTimer));
        drawText(24.0f, winH - 82.0f, buff, GLUT_BITMAP_HELVETICA_12);
    }
}

void drawBricks() {
    for (int i = 0; i < brickCount; i++) {
        if (!bricks[i].alive) {
            continue;
        }

        // Invisible bricks appear very faint until hit
        if (bricks[i].type == 4) {
            glColor4f(0.3f, 0.3f, 0.4f, 0.3f);
        } else {
            glColor3f(bricks[i].r, bricks[i].g, bricks[i].b);
        }
        drawRect(bricks[i].x, bricks[i].y, bricks[i].w, bricks[i].h);

        // Moving brick animation
        if (bricks[i].type == 3) {
            glColor4f(0.7f, 0.95f, 0.4f, 0.5f);
            drawCircle(bricks[i].x + bricks[i].w * 0.25f, bricks[i].y + bricks[i].h * 0.5f, 4.0f);
            drawCircle(bricks[i].x + bricks[i].w * 0.75f, bricks[i].y + bricks[i].h * 0.5f, 4.0f);
        }

        // Locked brick shows lock symbol
        if (bricks[i].type == 7 && bricks[i].isLocked) {
            glColor3f(1.0f, 0.9f, 0.3f);
            drawCircle(bricks[i].x + bricks[i].w * 0.5f, bricks[i].y + bricks[i].h * 0.5f, 5.0f);
        }

        glColor3f(0.08f, 0.08f, 0.1f);
        glLineWidth(2.0f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(bricks[i].x, bricks[i].y);
        glVertex2f(bricks[i].x + bricks[i].w, bricks[i].y);
        glVertex2f(bricks[i].x + bricks[i].w, bricks[i].y + bricks[i].h);
        glVertex2f(bricks[i].x, bricks[i].y + bricks[i].h);
        glEnd();
    }
}

void drawDrops() {
    for (int i = 0; i < dropCount; i++) {
        if (!drops[i].active) {
            continue;
        }

        if (drops[i].type == DROP_LIFE) {
            glColor3f(0.95f, 0.36f, 0.40f);
        } else if (drops[i].type == DROP_SPEED) {
            glColor3f(0.96f, 0.72f, 0.18f);
        } else if (drops[i].type == DROP_WIDE) {
            glColor3f(0.20f, 0.78f, 0.45f);
        } else if (drops[i].type == DROP_ROW_CLEAR) {
            glColor3f(0.76f, 0.95f, 0.30f);
        } else if (drops[i].type == DROP_FIREBALL) {
            glColor3f(1.0f, 0.48f, 0.12f);
        } else if (drops[i].type == DROP_MULTIBALL) {
            glColor3f(0.9f, 0.5f, 0.9f);
        } else if (drops[i].type == DROP_BOMB) {
            glColor3f(0.85f, 0.20f, 0.20f);
        } else if (drops[i].type == DROP_SHIELD) {
            glColor3f(0.20f, 0.60f, 0.95f);
        } else if (drops[i].type == DROP_KEY) {
            glColor3f(1.0f, 0.85f, 0.2f);
        }
        drawCircle(drops[i].x, drops[i].y, drops[i].size * 0.5f);

        glColor3f(0.08f, 0.08f, 0.1f);
        if (drops[i].type == DROP_LIFE) {
            drawTextCenter(drops[i].x, drops[i].y - 4.0f, "+1", GLUT_BITMAP_HELVETICA_12);
        } else if (drops[i].type == DROP_SPEED) {
            drawTextCenter(drops[i].x, drops[i].y - 4.0f, "S", GLUT_BITMAP_HELVETICA_12);
        } else if (drops[i].type == DROP_WIDE) {
            drawTextCenter(drops[i].x, drops[i].y - 4.0f, "W", GLUT_BITMAP_HELVETICA_12);
        } else if (drops[i].type == DROP_ROW_CLEAR) {
            drawTextCenter(drops[i].x, drops[i].y - 4.0f, "R", GLUT_BITMAP_HELVETICA_12);
        } else if (drops[i].type == DROP_FIREBALL) {
            drawTextCenter(drops[i].x, drops[i].y - 4.0f, "F", GLUT_BITMAP_HELVETICA_12);
        } else if (drops[i].type == DROP_MULTIBALL) {
            drawTextCenter(drops[i].x, drops[i].y - 4.0f, "M", GLUT_BITMAP_HELVETICA_12);
        } else if (drops[i].type == DROP_BOMB) {
            drawTextCenter(drops[i].x, drops[i].y - 4.0f, "X", GLUT_BITMAP_HELVETICA_12);
        } else if (drops[i].type == DROP_SHIELD) {
            drawTextCenter(drops[i].x, drops[i].y - 4.0f, "P", GLUT_BITMAP_HELVETICA_12);
        } else if (drops[i].type == DROP_KEY) {
            drawTextCenter(drops[i].x, drops[i].y - 4.0f, "K", GLUT_BITMAP_HELVETICA_12);
        }
    }
}

void drawPaddleBall() {
    glColor3f(0.92f, 0.96f, 1.0f);
    drawRect(paddleX - paddleW * 0.5f, paddleY - paddleH * 0.5f, paddleW, paddleH);
    
    // Draw shield glow if active
    if (shieldTimer > 0.0f) {
        glColor4f(0.2f, 0.6f, 0.95f, 0.3f);
        drawCircle(paddleX, paddleY, paddleW * 0.6f);
    }

    glColor3f(1.0f, 0.84f, 0.25f);
    // draw fireball glow if active
    if (fireballTimer > 0.0f) {
        glColor4f(1.0f, 0.45f, 0.1f, 0.85f);
        drawCircle(ballX, ballY, ballR * 1.9f);
        glColor3f(1.0f, 0.84f, 0.25f);
    }
    drawCircle(ballX, ballY, ballR);

    // draw extra balls with unique colors
    for (int i = 0; i < MAX_EXTRA_BALLS; i++) {
        if (!extraBallActive[i]) continue;
        glColor3f(extraBallR_val[i], extraBallG_val[i], extraBallB_val[i]);
        drawCircle(extraBallX[i], extraBallY[i], extraBallR);
        // glow effect
        glColor4f(extraBallR_val[i], extraBallG_val[i], extraBallB_val[i], 0.3f);
        drawCircle(extraBallX[i], extraBallY[i], extraBallR * 1.6f);
    }
}

void drawGame() {
    // screen shake offset
    if (shakeTimer > 0.0f) {
        float ox = (rand() % 100 - 50) / 50.0f * shakeIntensity;
        float oy = (rand() % 100 - 50) / 50.0f * shakeIntensity;
        glPushMatrix();
        glTranslatef(ox, oy, 0.0f);
    }

    drawBackground();
    drawBricks();
    drawDrops();
    drawPaddleBall();
    drawHUD();

    drawParticles();
    drawFloatTexts();

    if (shakeTimer > 0.0f) {
        glPopMatrix();
    }

    if (ballAttached && currentPage == PAGE_PLAY) {
        glColor3f(0.88f, 0.9f, 1.0f);
        drawTextCenter(winW * 0.5f, 108.0f, "Press SPACE or Left Click to launch", GLUT_BITMAP_HELVETICA_18);
    }
}

int pointInRect(float px, float py, float rx, float ry, float rw, float rh) {
    return px >= rx && px <= rx + rw && py >= ry && py <= ry + rh;
}

void drawModernBackdrop() {
    glBegin(GL_QUADS);
    glColor3f(0.05f, 0.06f, 0.10f);
    glVertex2f(0.0f, 0.0f);
    glVertex2f((float)winW, 0.0f);
    glColor3f(0.10f, 0.14f, 0.24f);
    glVertex2f((float)winW, (float)winH);
    glColor3f(0.04f, 0.08f, 0.15f);
    glVertex2f(0.0f, (float)winH);
    glEnd();

    glColor4f(0.20f, 0.56f, 0.98f, 0.13f);
    drawCircle(winW * 0.18f, winH * 0.78f, 180.0f);
    glColor4f(1.00f, 0.84f, 0.25f, 0.12f);
    drawCircle(winW * 0.85f, winH * 0.82f, 150.0f);
    glColor4f(0.35f, 0.90f, 0.60f, 0.10f);
    drawCircle(winW * 0.55f, winH * 0.28f, 240.0f);

    glColor4f(1.0f, 1.0f, 1.0f, 0.05f);
    for (int i = 0; i < 12; i++) {
        float y = 50.0f + i * 52.0f;
        drawRect(0.0f, y, (float)winW, 1.2f);
    }

    glColor4f(1.0f, 1.0f, 1.0f, 0.04f);
    for (int i = 0; i < 18; i++) {
        float x = 40.0f + i * 54.0f;
        drawRect(x, 0.0f, 1.0f, (float)winH);
    }
}

void drawCursorSpotlight() {
    if (!mouseVisible) {
        return;
    }

    glColor4f(0.20f, 0.56f, 0.98f, 0.12f);
    drawCircle(mouseX, mouseY, 54.0f);
    glColor4f(0.20f, 0.56f, 0.98f, 0.18f);
    drawCircle(mouseX, mouseY, 30.0f);

    glColor4f(1.0f, 1.0f, 1.0f, 0.70f);
    glLineWidth(1.5f);
    glBegin(GL_LINES);
    glVertex2f(mouseX - 12.0f, mouseY);
    glVertex2f(mouseX + 12.0f, mouseY);
    glVertex2f(mouseX, mouseY - 12.0f);
    glVertex2f(mouseX, mouseY + 12.0f);
    glEnd();
}

void drawBackButton() {
    float bx = 28.0f;
    float by = winH - 76.0f;
    float bw = 122.0f;
    float bh = 34.0f;
    int hover = pointInRect(mouseX, mouseY, bx, by, bw, bh);

    if (hover) {
        glColor3f(0.20f, 0.56f, 0.98f);
    } else {
        glColor3f(0.10f, 0.14f, 0.23f);
    }
    drawRect(bx, by, bw, bh);

    glColor3f(0.86f, 0.92f, 1.0f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(bx, by);
    glVertex2f(bx + bw, by);
    glVertex2f(bx + bw, by + bh);
    glVertex2f(bx, by + bh);
    glEnd();

    drawTextCenter(bx + bw * 0.5f, by + 11.0f, "Back", GLUT_BITMAP_HELVETICA_18);
}

void drawCardFrame(float cx, float cy, float cardW, float cardH, int selected, int hovered) {
    float x = cx - cardW * 0.5f;
    float y = cy - cardH * 0.5f;

    if (selected) {
        glColor3f(0.20f, 0.56f, 0.98f);
    } else if (hovered) {
        glColor3f(0.95f, 0.75f, 0.20f);
    } else {
        glColor3f(0.18f, 0.22f, 0.32f);
    }
    drawRect(x, y, cardW, cardH);

    glColor4f(1.0f, 1.0f, 1.0f, 0.07f);
    drawRect(x + 5.0f, y + cardH - 18.0f, cardW - 10.0f, 12.0f);

    glColor3f(0.04f, 0.05f, 0.08f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y);
    glVertex2f(x + cardW, y);
    glVertex2f(x + cardW, y + cardH);
    glVertex2f(x, y + cardH);
    glEnd();
}

void drawThemePreview(int theme, float cx, float cy, int selected, int hovered) {
    float cardW = 170.0f;
    float cardH = 160.0f;
    float x = cx - cardW * 0.5f;
    float y = cy - cardH * 0.5f;

    drawCardFrame(cx, cy, cardW, cardH, selected, hovered);

    if (theme == THEME_NATURE) {
        glColor3f(0.40f, 0.72f, 0.95f);
        drawRect(x + 8.0f, y + 62.0f, cardW - 16.0f, 90.0f);
        glColor3f(0.24f, 0.62f, 0.25f);
        drawRect(x + 8.0f, y + 8.0f, cardW - 16.0f, 58.0f);
        glColor4f(1.0f, 0.92f, 0.32f, 0.85f);
        drawCircle(x + 132.0f, y + 125.0f, 16.0f);
        glColor3f(0.15f, 0.48f, 0.18f);
        drawRect(x + 36.0f, y + 28.0f, 10.0f, 26.0f);
        drawRect(x + 54.0f, y + 28.0f, 10.0f, 36.0f);
        glColor3f(0.28f, 0.75f, 0.30f);
        drawCircle(x + 41.0f, y + 58.0f, 18.0f);
        drawCircle(x + 60.0f, y + 64.0f, 16.0f);
    } else if (theme == THEME_CITY) {
        glColor3f(0.10f, 0.12f, 0.18f);
        drawRect(x + 8.0f, y + 8.0f, cardW - 16.0f, 144.0f);
        glColor3f(0.18f, 0.20f, 0.28f);
        drawRect(x + 14.0f, y + 8.0f, 28.0f, 72.0f);
        drawRect(x + 46.0f, y + 8.0f, 24.0f, 96.0f);
        drawRect(x + 74.0f, y + 8.0f, 34.0f, 82.0f);
        drawRect(x + 112.0f, y + 8.0f, 40.0f, 58.0f);
        glColor3f(0.98f, 0.82f, 0.24f);
        drawCircle(x + 130.0f, y + 122.0f, 13.0f);
        glColor4f(0.95f, 0.95f, 1.0f, 0.25f);
        drawRect(x + 22.0f, y + 18.0f, 6.0f, 6.0f);
        drawRect(x + 52.0f, y + 26.0f, 5.0f, 5.0f);
        drawRect(x + 83.0f, y + 24.0f, 5.0f, 5.0f);
    } else if (theme == THEME_SPACE) {
        glColor3f(0.02f, 0.02f, 0.08f);
        drawRect(x + 8.0f, y + 8.0f, cardW - 16.0f, 144.0f);
        glColor3f(0.92f, 0.94f, 1.0f);
        drawCircle(x + 124.0f, y + 120.0f, 10.0f);
        glColor3f(0.32f, 0.62f, 0.98f);
        drawCircle(x + 58.0f, y + 82.0f, 22.0f);
        glColor3f(0.92f, 0.62f, 0.24f);
        drawCircle(x + 84.0f, y + 52.0f, 8.0f);
        glColor4f(1.0f, 1.0f, 1.0f, 0.8f);
        for (int i = 0; i < 8; i++) {
            drawCircle(x + 22.0f + i * 18.0f, y + 122.0f - (i % 3) * 26.0f, 1.4f);
        }
    } else {
        glColor3f(0.06f, 0.01f, 0.10f);
        drawRect(x + 8.0f, y + 8.0f, cardW - 16.0f, 144.0f);
        glColor3f(0.18f, 0.76f, 0.95f);
        drawRect(x + 10.0f, y + 24.0f, cardW - 20.0f, 2.0f);
        drawRect(x + 10.0f, y + 50.0f, cardW - 20.0f, 2.0f);
        drawRect(x + 10.0f, y + 76.0f, cardW - 20.0f, 2.0f);
        glColor3f(0.96f, 0.25f, 0.82f);
        drawCircle(x + 126.0f, y + 118.0f, 16.0f);
        glColor3f(0.96f, 0.72f, 0.18f);
        drawRect(x + 28.0f, y + 28.0f, 18.0f, 56.0f);
        drawRect(x + 56.0f, y + 28.0f, 18.0f, 74.0f);
        drawRect(x + 84.0f, y + 28.0f, 18.0f, 50.0f);
    }
}

void drawMapPreview(int mapNum, float px, float py, float scale, int isSelected);

void drawMapCategoryCard(int category, float cx, float cy, int selected, int hovered) {
    float cardW = 170.0f;
    float cardH = 160.0f;
    drawCardFrame(cx, cy, cardW, cardH, selected, hovered);
    drawMapPreview(category + 1, cx, cy + 6.0f, 1.0f, selected);

    int bestDifficulty = -1;
    float bestTime = 0.0f;
    int bestScore = getBestScoreForMap(category, &bestDifficulty, &bestTime);

    glColor3f(0.94f, 0.96f, 1.0f);
    if (bestScore >= 0) {
        char scoreLine[128];
        char diffLine[128];
        sprintf(scoreLine, "Best: %d", bestScore);
        sprintf(diffLine, "%s", difficultyText(bestDifficulty));
        drawTextCenter(cx, cy - 60.0f, scoreLine, GLUT_BITMAP_HELVETICA_12);
        drawTextCenter(cx, cy - 76.0f, diffLine, GLUT_BITMAP_HELVETICA_12);
    } else {
        drawTextCenter(cx, cy - 66.0f, "Best: --", GLUT_BITMAP_HELVETICA_12);
        drawTextCenter(cx, cy - 82.0f, "Difficulty: --", GLUT_BITMAP_HELVETICA_12);
    }
}

void drawMapPreview(int mapNum, float px, float py, float scale, int isSelected) {
    float cardW = 140.0f;
    float cardH = 140.0f;
    float previewX = px - cardW * 0.5f;
    float previewY = py - cardH * 0.5f;

    if (isSelected) {
        glColor3f(0.20f, 0.56f, 0.98f);
        glLineWidth(3.0f);
    } else {
        glColor3f(0.4f, 0.4f, 0.5f);
        glLineWidth(2.0f);
    }
    
    glBegin(GL_LINE_LOOP);
    glVertex2f(previewX, previewY);
    glVertex2f(previewX + cardW, previewY);
    glVertex2f(previewX + cardW, previewY + cardH);
    glVertex2f(previewX, previewY + cardH);
    glEnd();

    glColor4f(0.08f, 0.1f, 0.2f, 0.8f);
    drawRect(previewX, previewY, cardW, cardH);

    float brickPreviewX = previewX + 8.0f;
    float brickPreviewY = previewY + cardH - 12.0f;
    float bw = 12.0f;
    float bh = 8.0f;
    float gap = 1.0f;

    float colors[6][3] = {
        {0.93f, 0.33f, 0.27f},
        {0.96f, 0.52f, 0.24f},
        {0.94f, 0.75f, 0.24f},
        {0.31f, 0.72f, 0.29f},
        {0.23f, 0.55f, 0.93f},
        {0.55f, 0.38f, 0.89f},
    };

    if (mapNum == 1) {
        for (int r = 0; r < 4; r++) {
            for (int c = 0; c < 10; c++) {
                glColor3f(colors[r][0], colors[r][1], colors[r][2]);
                drawRect(brickPreviewX + c * (bw + gap), brickPreviewY - r * (bh + gap), bw, bh);
            }
        }
    } else if (mapNum == 2) {
        for (int r = 0; r < 4; r++) {
            for (int c = 0; c < 10; c++) {
                if ((r + c) % 2 == 0) {
                    glColor3f(colors[r][0], colors[r][1], colors[r][2]);
                    drawRect(brickPreviewX + c * (bw + gap), brickPreviewY - r * (bh + gap), bw, bh);
                }
            }
        }
    } else if (mapNum == 3) {
        for (int r = 0; r < 4; r++) {
            for (int c = 0; c < 10; c++) {
                int skip = 0;
                if (r < 1 && (c < 1 || c >= 8)) skip = 1;
                if (r >= 2 && c >= 3 && c <= 6) skip = 1;
                
                if (!skip) {
                    glColor3f(colors[r][0], colors[r][1], colors[r][2]);
                    drawRect(brickPreviewX + c * (bw + gap), brickPreviewY - r * (bh + gap), bw, bh);
                }
            }
        }
    } else if (mapNum == 4) {
        for (int r = 0; r < 4; r++) {
            int bricksInRow = 10 - (r * 1);
            int startCol = r / 2;
            
            for (int c = 0; c < bricksInRow; c++) {
                if (startCol + c < 10) {
                    glColor3f(colors[r][0], colors[r][1], colors[r][2]);
                    drawRect(brickPreviewX + (startCol + c) * (bw + gap), brickPreviewY - r * (bh + gap), bw, bh);
                }
            }
        }
    } else if (mapNum == 5) {
        for (int r = 0; r < 4; r++) {
            for (int c = 0; c < 10; c++) {
                int dist = (c - 5) * (c - 5) + (r - 1) * (r - 1);
                if (dist <= 8) {
                    glColor3f(colors[r][0], colors[r][1], colors[r][2]);
                    drawRect(brickPreviewX + c * (bw + gap), brickPreviewY - r * (bh + gap), bw, bh);
                }
            }
        }
    }
}

// Feature: Map preview visualization (added by Kawser)
// Kawser: provides visual card-based previews for each map layout

int menuSize() {
    if (currentPage == PAGE_DIFFICULTY) {
        return 4;
    }
    if (currentPage == PAGE_MAP_CATEGORY) {
        return 5;
    }
    if (currentPage == PAGE_WEATHER) {
        return 4;
    }
    if (currentPage == PAGE_SETTINGS) {
        return 6;
    }
    return canResume ? 6 : 5;
}

const char* menuText(int idx) {
    if (currentPage == PAGE_DIFFICULTY) {
        if (idx == 0) return "Easy";
        if (idx == 1) return "Normal";
        if (idx == 2) return "Hard";
        return "Hardest";
    }
    
    if (currentPage == PAGE_MAP_CATEGORY) {
        if (idx == 0) return "Classic";
        if (idx == 1) return "Checkerboard";
        if (idx == 2) return "Circle";
        if (idx == 3) return "Pyramid";
        return "Special";
    }
    
    if (currentPage == PAGE_WEATHER) {
        if (idx == 0) return "Nature";
        if (idx == 1) return "City";
        if (idx == 2) return "Space";
        return "Neon";
    }

    if (currentPage == PAGE_SETTINGS) {
        if (idx == 0) return "Volume: Mute";
        if (idx == 1) return "Volume: 25%";
        if (idx == 2) return "Volume: 50%";
        if (idx == 3) return "Volume: 75%";
        if (idx == 4) return "Volume: 100%";
        return "Back";
    }
    
    if (canResume) {
        if (idx == 0) return "Start New Game";
        if (idx == 1) return "Resume";
        if (idx == 2) return "High Scores";
        if (idx == 3) return "Help";
        if (idx == 4) return "Game Settings";
        return "Exit";
    }

    if (idx == 0) return "Start New Game";
    if (idx == 1) return "High Scores";
    if (idx == 2) return "Help";
    if (idx == 3) return "Game Settings";
    return "Exit";
}

void runMenuAction(int idx) {
    if (currentPage == PAGE_DIFFICULTY) {
        selectedDifficulty = idx;
        difficultyIndex = idx;
        menuIndex = 0;
        currentPage = PAGE_MAP_CATEGORY;
        return;
    }
    
    if (currentPage == PAGE_MAP_CATEGORY) {
        selectedMapCategory = idx;
        mapCategoryIndex = idx;
        menuIndex = 0;
        currentPage = PAGE_WEATHER;
        return;
    }
    
    if (currentPage == PAGE_WEATHER) {
        if (idx == 0) {
            selectedTheme = THEME_NATURE;
            selectedWeather = WEATHER_SUNNY;
        } else if (idx == 1) {
            selectedTheme = THEME_CITY;
            selectedWeather = WEATHER_NIGHT;
        } else if (idx == 2) {
            selectedTheme = THEME_SPACE;
            selectedWeather = WEATHER_NIGHT;
        } else {
            selectedTheme = THEME_NEON;
            selectedWeather = WEATHER_STORMY;
        }
        themeIndex = idx;
        weatherIndex = idx;
        selectedMapVariant = rand() % 3;
        selectedMap = selectedMapVariant + 1;
        newGame();
        return;
    }

    if (currentPage == PAGE_SETTINGS) {
        if (idx == 0) {
            musicVolumePercent = 0;
            applyMusicVolume();
        } else if (idx == 1) {
            musicVolumePercent = 25;
            applyMusicVolume();
        } else if (idx == 2) {
            musicVolumePercent = 50;
            applyMusicVolume();
        } else if (idx == 3) {
            musicVolumePercent = 75;
            applyMusicVolume();
        } else if (idx == 4) {
            musicVolumePercent = 100;
            applyMusicVolume();
        } else {
            currentPage = PAGE_MENU;
            menuIndex = 0;
        }
        return;
    }
    
    if (canResume) {
        if (idx == 0) {
            menuIndex = 0;
            currentPage = PAGE_DIFFICULTY;
        } else if (idx == 1) {
            currentPage = PAGE_PLAY;
        } else if (idx == 2) {
            loadScores();
            currentPage = PAGE_SCORE;
        } else if (idx == 3) {
            currentPage = PAGE_HELP;
        } else if (idx == 4) {
            currentPage = PAGE_SETTINGS;
            menuIndex = 0;
        } else {
            quitGame();
        }
        return;
    }

    if (idx == 0) {
        menuIndex = 0;
        currentPage = PAGE_DIFFICULTY;
    } else if (idx == 1) {
        loadScores();
        currentPage = PAGE_SCORE;
    } else if (idx == 2) {
        currentPage = PAGE_HELP;
    } else if (idx == 3) {
        currentPage = PAGE_SETTINGS;
        menuIndex = 0;
    } else {
        quitGame();
    }
}

void drawMenu() {
    if (currentPage == PAGE_DIFFICULTY) {
        drawModernBackdrop();
        drawBackButton();

        glColor3f(0.95f, 0.90f, 0.28f);
        drawTextCenter(winW * 0.5f, winH - 108.0f, "SELECT DIFFICULTY", GLUT_BITMAP_TIMES_ROMAN_24);

        float cardY = winH * 0.54f;
        float startX = winW * 0.5f - 315.0f;
        for (int i = 0; i < 4; i++) {
            float cardX = startX + i * 210.0f;
            int hovered = pointInRect(mouseX, mouseY, cardX - 85.0f, cardY - 80.0f, 170.0f, 160.0f);
            drawCardFrame(cardX, cardY, 170.0f, 160.0f, i == menuIndex, hovered);
            if (i == 0) glColor3f(0.28f, 0.78f, 0.42f);
            else if (i == 1) glColor3f(0.98f, 0.74f, 0.24f);
            else if (i == 2) glColor3f(0.96f, 0.35f, 0.36f);
            else glColor3f(0.78f, 0.30f, 0.98f);
            drawCircle(cardX, cardY + 22.0f, 24.0f);
            glColor3f(0.95f, 0.97f, 1.0f);
            drawTextCenter(cardX, cardY - 34.0f, menuText(i), GLUT_BITMAP_TIMES_ROMAN_24);
            if (i == 0) drawTextCenter(cardX, cardY - 64.0f, "5 lives", GLUT_BITMAP_HELVETICA_12);
            else if (i == 1) drawTextCenter(cardX, cardY - 64.0f, "Balanced run", GLUT_BITMAP_HELVETICA_12);
            else if (i == 2) drawTextCenter(cardX, cardY - 64.0f, "Fast and risky", GLUT_BITMAP_HELVETICA_12);
            else drawTextCenter(cardX, cardY - 64.0f, "Bricks fall down", GLUT_BITMAP_HELVETICA_12);
        }

        glColor3f(0.86f, 0.9f, 0.97f);
        drawTextCenter(winW * 0.5f, 88.0f, "W/S or Mouse to choose, Enter to continue", GLUT_BITMAP_HELVETICA_12);
        return;
    }

    if (currentPage == PAGE_MAP_CATEGORY) {
        drawModernBackdrop();
        drawBackButton();

        glColor3f(0.95f, 0.90f, 0.28f);
        drawTextCenter(winW * 0.5f, winH - 108.0f, "SELECT MAP CATEGORY", GLUT_BITMAP_TIMES_ROMAN_24);
        drawTextCenter(winW * 0.5f, winH - 138.0f, "Map variants are randomized after you choose a category", GLUT_BITMAP_HELVETICA_12);

        float xs[5] = {winW * 0.18f, winW * 0.40f, winW * 0.62f, winW * 0.31f, winW * 0.53f};
        float ys[5] = {winH * 0.61f, winH * 0.61f, winH * 0.61f, winH * 0.34f, winH * 0.34f};
        for (int i = 0; i < 5; i++) {
            int hovered = pointInRect(mouseX, mouseY, xs[i] - 85.0f, ys[i] - 80.0f, 170.0f, 160.0f);
            drawMapCategoryCard(i, xs[i], ys[i], i == menuIndex, hovered);
            glColor3f(0.94f, 0.96f, 1.0f);
            drawTextCenter(xs[i], ys[i] - 98.0f, menuText(i), GLUT_BITMAP_HELVETICA_18);
        }

        glColor3f(0.86f, 0.9f, 0.97f);
        drawTextCenter(winW * 0.5f, 88.0f, "W/S or Mouse to choose, Enter to continue", GLUT_BITMAP_HELVETICA_12);
        return;
    }

    if (currentPage == PAGE_WEATHER) {
        drawModernBackdrop();
        drawBackButton();

        glColor3f(0.95f, 0.90f, 0.28f);
        drawTextCenter(winW * 0.5f, winH - 108.0f, "SELECT THEME", GLUT_BITMAP_TIMES_ROMAN_24);
        drawTextCenter(winW * 0.5f, winH - 138.0f, "Theme cards preview the background style before play starts", GLUT_BITMAP_HELVETICA_12);

        float xs[4] = {winW * 0.30f, winW * 0.60f, winW * 0.30f, winW * 0.60f};
        float ys[4] = {winH * 0.61f, winH * 0.61f, winH * 0.34f, winH * 0.34f};
        for (int i = 0; i < 4; i++) {
            int hovered = pointInRect(mouseX, mouseY, xs[i] - 85.0f, ys[i] - 80.0f, 170.0f, 160.0f);
            drawThemePreview(i, xs[i], ys[i], i == menuIndex, hovered);
            glColor3f(0.94f, 0.96f, 1.0f);
            drawTextCenter(xs[i], ys[i] - 98.0f, menuText(i), GLUT_BITMAP_HELVETICA_18);
        }

        glColor3f(0.86f, 0.9f, 0.97f);
        drawTextCenter(winW * 0.5f, 88.0f, "W/S or Mouse to choose, Enter to start", GLUT_BITMAP_HELVETICA_12);
        return;
    }

    if (currentPage == PAGE_SETTINGS) {
        drawModernBackdrop();
        drawBackButton();

        glColor3f(0.95f, 0.90f, 0.28f);
        drawTextCenter(winW * 0.5f, winH - 108.0f, "GAME SETTINGS", GLUT_BITMAP_TIMES_ROMAN_24);

        char volLine[64];
        sprintf(volLine, "Current Music Volume: %d%%", musicVolumePercent);
        glColor3f(0.86f, 0.9f, 0.97f);
        drawTextCenter(winW * 0.5f, winH - 144.0f, volLine, GLUT_BITMAP_HELVETICA_18);

        float buttonY = winH * 0.58f;
        float buttonX = winW * 0.5f - 170.0f;
        for (int i = 0; i < menuSize(); i++) {
            float bx = buttonX;
            float by = buttonY;
            float bw = 340.0f;
            float bh = 42.0f;
            int hovered = pointInRect(mouseX, mouseY, bx, by, bw, bh);
            if (i == menuIndex || hovered) {
                glColor3f(0.20f, 0.56f, 0.98f);
            } else {
                glColor3f(0.10f, 0.14f, 0.22f);
            }
            drawRect(bx, by, bw, bh);
            glColor3f(0.94f, 0.96f, 1.0f);
            glLineWidth(2.0f);
            glBegin(GL_LINE_LOOP);
            glVertex2f(bx, by);
            glVertex2f(bx + bw, by);
            glVertex2f(bx + bw, by + bh);
            glVertex2f(bx, by + bh);
            glEnd();
            drawTextCenter(winW * 0.5f, by + 13.0f, menuText(i), GLUT_BITMAP_HELVETICA_18);
            buttonY -= 56.0f;
        }

        glColor3f(0.86f, 0.9f, 0.97f);
        drawTextCenter(winW * 0.5f, 88.0f, "W/S or Mouse to choose, Enter to apply", GLUT_BITMAP_HELVETICA_12);
        return;
    }

    drawModernBackdrop();
    glColor3f(0.95f, 0.90f, 0.28f);
    drawTextCenter(winW * 0.5f, winH - 112.0f, "DX BALL", GLUT_BITMAP_TIMES_ROMAN_24);
    drawTextCenter(winW * 0.5f, winH - 144.0f, "Modern arcade battle with themed backgrounds and randomized maps", GLUT_BITMAP_HELVETICA_12);

    float buttonY = winH * 0.58f;
    float buttonX = winW * 0.5f - 170.0f;
    int msize = menuSize();
    if (menuIndex < 0) {
        menuIndex = msize - 1;
    }
    if (menuIndex >= msize) {
        menuIndex = 0;
    }

    for (int i = 0; i < msize; i++) {
        float bx = buttonX;
        float by = buttonY;
        float bw = 340.0f;
        float bh = 42.0f;
        int hovered = pointInRect(mouseX, mouseY, bx, by, bw, bh);
        if (i == menuIndex || hovered) {
            glColor3f(0.20f, 0.56f, 0.98f);
        } else {
            glColor3f(0.10f, 0.14f, 0.22f);
        }
        drawRect(bx, by, bw, bh);
        glColor3f(0.94f, 0.96f, 1.0f);
        glLineWidth(2.0f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(bx, by);
        glVertex2f(bx + bw, by);
        glVertex2f(bx + bw, by + bh);
        glVertex2f(bx, by + bh);
        glEnd();
        drawTextCenter(winW * 0.5f, by + 13.0f, menuText(i), GLUT_BITMAP_HELVETICA_18);
        buttonY -= 56.0f;
    }

    glColor3f(0.86f, 0.9f, 0.97f);
    drawTextCenter(winW * 0.5f, 88.0f, "W/S or Mouse to choose, Enter to continue", GLUT_BITMAP_HELVETICA_12);
}

void drawScoresPage() {
    drawBackground();

    glColor3f(0.96f, 0.93f, 0.33f);
    drawTextCenter(winW * 0.5f, winH - 110.0f, "HIGH SCORES BY MAP", GLUT_BITMAP_TIMES_ROMAN_24);

    if (highCount == 0) {
        glColor3f(0.85f, 0.9f, 1.0f);
        drawTextCenter(winW * 0.5f, winH * 0.5f, "No records yet");
    } else {
        float startY = winH - 190.0f;
        for (int category = 0; category < 5; category++) {
            float rowY = startY - category * 72.0f;
            glColor3f(0.10f, 0.14f, 0.22f);
            drawRect(winW * 0.18f, rowY - 22.0f, winW * 0.64f, 54.0f);
            glColor3f(0.94f, 0.96f, 1.0f);
            glLineWidth(2.0f);
            glBegin(GL_LINE_LOOP);
            glVertex2f(winW * 0.18f, rowY - 22.0f);
            glVertex2f(winW * 0.82f, rowY - 22.0f);
            glVertex2f(winW * 0.82f, rowY + 32.0f);
            glVertex2f(winW * 0.18f, rowY + 32.0f);
            glEnd();

            int bestDifficulty = -1;
            float bestTime = 0.0f;
            int bestScore = getBestScoreForMap(category, &bestDifficulty, &bestTime);

            glColor3f(0.96f, 0.93f, 0.33f);
            drawText(winW * 0.21f, rowY + 2.0f, mapCategoryText(category), GLUT_BITMAP_HELVETICA_18);

            glColor3f(0.85f, 0.92f, 0.98f);
            char line[128];
            if (bestScore >= 0) {
                sprintf(line, "Best: %d   Difficulty: %s   Time: %s", bestScore, difficultyText(bestDifficulty), timeText(bestTime));
            } else {
                sprintf(line, "Best: --   Difficulty: --   Time: --");
            }
            drawText(winW * 0.46f, rowY + 2.0f, line, GLUT_BITMAP_HELVETICA_12);
        }
    }

    glColor3f(0.86f, 0.9f, 0.97f);
    drawTextCenter(winW * 0.5f, 88.0f, "Press M or ESC to go back");
}

void drawHelpPage() {
    drawBackground();

    glColor3f(0.96f, 0.93f, 0.33f);
    drawTextCenter(winW * 0.5f, winH - 110.0f, "HELP", GLUT_BITMAP_TIMES_ROMAN_24);

    glColor3f(0.86f, 0.92f, 0.99f);
    drawTextCenter(winW * 0.5f, winH - 190.0f, "Move Paddle: Left/Right or A/D keys, or Mouse movement");
    drawTextCenter(winW * 0.5f, winH - 232.0f, "Launch Ball: SPACE or Left Click");
    drawTextCenter(winW * 0.5f, winH - 274.0f, "Pause/Resume: P");
    drawTextCenter(winW * 0.5f, winH - 316.0f, "Return to Menu: ESC");
    drawTextCenter(winW * 0.5f, winH - 358.0f, "Exit Any Time: Q");
    drawTextCenter(winW * 0.5f, winH - 400.0f, "Perks: +1 Life, Speed Up (S), Wide Paddle (W), Row Clear (R)");
    drawTextCenter(winW * 0.5f, winH - 442.0f, "Bomb is bad drop. Shield can block it.");
    drawTextCenter(winW * 0.5f, winH - 484.0f, "Collect K drops. Press K to unlock one locked brick.");

    glColor3f(0.86f, 0.9f, 0.97f);
    drawTextCenter(winW * 0.5f, 88.0f, "Press M or ESC to go back");
}

void drawResultPage(int won) {
    drawGame();

    glColor4f(0.0f, 0.0f, 0.0f, 0.55f);
    drawRect(0.0f, 0.0f, (float)winW, (float)winH);

    if (won) {
        glColor3f(0.32f, 0.95f, 0.55f);
        drawTextCenter(winW * 0.5f, winH * 0.58f, "YOU WIN!", GLUT_BITMAP_TIMES_ROMAN_24);
    } else {
        glColor3f(1.0f, 0.35f, 0.35f);
        drawTextCenter(winW * 0.5f, winH * 0.58f, "GAME OVER", GLUT_BITMAP_TIMES_ROMAN_24);
    }

    char line[200];
    sprintf(line, "Score: %d   Time: %s", score, timeText(playTimeSec));
    glColor3f(0.94f, 0.97f, 1.0f);
    drawTextCenter(winW * 0.5f, winH * 0.51f, line);
    drawTextCenter(winW * 0.5f, winH * 0.45f, "Press N for new game or M for menu");
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    if (currentPage == PAGE_MENU || currentPage == PAGE_DIFFICULTY || currentPage == PAGE_MAP_CATEGORY || currentPage == PAGE_WEATHER || currentPage == PAGE_SETTINGS) {
        drawMenu();
        drawCursorSpotlight();
    } else if (currentPage == PAGE_PLAY) {
        drawGame();
    } else if (currentPage == PAGE_PAUSE) {
        drawGame();
        glColor4f(0.0f, 0.0f, 0.0f, 0.55f);
        drawRect(0.0f, 0.0f, (float)winW, (float)winH);
        glColor3f(0.95f, 0.95f, 1.0f);
        drawTextCenter(winW * 0.5f, winH * 0.52f, "PAUSED", GLUT_BITMAP_TIMES_ROMAN_24);
        drawTextCenter(winW * 0.5f, winH * 0.46f, "Press P/R to resume, ESC for menu");
    } else if (currentPage == PAGE_SCORE) {
        drawScoresPage();
    } else if (currentPage == PAGE_HELP) {
        drawHelpPage();
    } else if (currentPage == PAGE_WIN) {
        drawResultPage(1);
    } else if (currentPage == PAGE_OVER) {
        drawResultPage(0);
    }

    glutSwapBuffers();
}

void launchBall() {
    if (currentPage == PAGE_PLAY && ballAttached) {
        ballAttached = 0;
    }
}

void keyboardDown(unsigned char key, int x, int y) {
    (void)x;
    (void)y;

    if (key == 'q' || key == 'Q') {
        quitGame();
    }

    if (currentPage == PAGE_PLAY) {
        if (key == 'a' || key == 'A') {
            moveLeft = 1;
        } else if (key == 'd' || key == 'D') {
            moveRight = 1;
        } else if (key == 'k' || key == 'K') {
            if (unlockRandomLockedBrick()) {
                addFloatText(paddleX, paddleY + 36.0f, "KEY USED", 1.0f, 0.85f, 0.25f);
            } else if (hasKey <= 0) {
                addFloatText(paddleX, paddleY + 36.0f, "NO KEY", 0.95f, 0.55f, 0.35f);
            }
        } else if (key == ' ') {
            launchBall();
        } else if (key == 'p' || key == 'P') {
            currentPage = PAGE_PAUSE;
        } else if (key == 27) {
            currentPage = PAGE_MENU;
            canResume = 1;
            menuIndex = 0;
        }
        return;
    }

    if (currentPage == PAGE_PAUSE) {
        if (key == 'p' || key == 'P' || key == 'r' || key == 'R') {
            currentPage = PAGE_PLAY;
        } else if (key == 27) {
            currentPage = PAGE_MENU;
            canResume = 1;
            menuIndex = 0;
        }
        return;
    }

    if (currentPage == PAGE_MENU || currentPage == PAGE_DIFFICULTY || currentPage == PAGE_MAP_CATEGORY || currentPage == PAGE_WEATHER || currentPage == PAGE_SETTINGS) {
        int msize = menuSize();
        
        if (key == 'w' || key == 'W') {
            menuIndex--;
            if (menuIndex < 0) menuIndex = msize - 1;
        } else if (key == 's' || key == 'S') {
            menuIndex++;
            if (menuIndex >= msize) menuIndex = 0;
        } else if (key == 13 || key == ' ') {
            runMenuAction(menuIndex);
        } else if (key == 27) {
            if (currentPage == PAGE_DIFFICULTY || currentPage == PAGE_MAP_CATEGORY || currentPage == PAGE_WEATHER || currentPage == PAGE_SETTINGS) {
                currentPage = PAGE_MENU;
                menuIndex = 0;
            } else {
                quitGame();
            }
        }
        return;
    }

    if (currentPage == PAGE_SCORE || currentPage == PAGE_HELP) {
        if (key == 'm' || key == 'M' || key == 27) {
            currentPage = PAGE_MENU;
            menuIndex = 0;
        }
        return;
    }

    if (currentPage == PAGE_WIN || currentPage == PAGE_OVER) {
        if (key == 'n' || key == 'N') {
            menuIndex = 0;
            currentPage = PAGE_DIFFICULTY;
        } else if (key == 'm' || key == 'M' || key == 27) {
            currentPage = PAGE_MENU;
            menuIndex = 0;
        }
    }
}

void keyboardUp(unsigned char key, int x, int y) {
    (void)x;
    (void)y;

    if (key == 'a' || key == 'A') {
        moveLeft = 0;
    } else if (key == 'd' || key == 'D') {
        moveRight = 0;
    }
}

void specialDown(int key, int x, int y) {
    (void)x;
    (void)y;

    if (currentPage == PAGE_PLAY) {
        if (key == GLUT_KEY_LEFT) {
            moveLeft = 1;
        } else if (key == GLUT_KEY_RIGHT) {
            moveRight = 1;
        }
    } else if (currentPage == PAGE_MENU || currentPage == PAGE_DIFFICULTY || currentPage == PAGE_MAP_CATEGORY || currentPage == PAGE_WEATHER || currentPage == PAGE_SETTINGS) {
        int msize = menuSize();
        if (key == GLUT_KEY_UP) {
            menuIndex--;
            if (menuIndex < 0) menuIndex = msize - 1;
        } else if (key == GLUT_KEY_DOWN) {
            menuIndex++;
            if (menuIndex >= msize) menuIndex = 0;
        }
    }
}

void specialUp(int key, int x, int y) {
    (void)x;
    (void)y;

    if (key == GLUT_KEY_LEFT) {
        moveLeft = 0;
    } else if (key == GLUT_KEY_RIGHT) {
        moveRight = 0;
    }
}

void mouseMove(int x, int y) {
    mouseX = (float)x;
    mouseY = (float)(winH - y);
    mouseVisible = 1;

    if (currentPage == PAGE_PLAY || currentPage == PAGE_PAUSE) {
        paddleX = clampf((float)x, paddleW * 0.5f, winW - paddleW * 0.5f);
    }
}

void mouseClick(int button, int state, int x, int y) {
    if (button != GLUT_LEFT_BUTTON || state != GLUT_DOWN) {
        return;
    }

    float wx = (float)x;
    float wy = (float)(winH - y);

    if (currentPage == PAGE_DIFFICULTY || currentPage == PAGE_MAP_CATEGORY || currentPage == PAGE_WEATHER || currentPage == PAGE_SETTINGS) {
        float backX = 28.0f;
        float backY = winH - 76.0f;
        if (pointInRect(wx, wy, backX, backY, 122.0f, 34.0f)) {
            currentPage = PAGE_MENU;
            menuIndex = 0;
            return;
        }
    }

    if (currentPage == PAGE_PLAY) {
        launchBall();
        return;
    }

    if (currentPage == PAGE_MENU || currentPage == PAGE_DIFFICULTY || currentPage == PAGE_MAP_CATEGORY || currentPage == PAGE_WEATHER || currentPage == PAGE_SETTINGS) {
        int msize = menuSize();
        if (currentPage == PAGE_DIFFICULTY) {
            float cardY = winH * 0.54f;
            float startX = winW * 0.5f - 210.0f;
            for (int i = 0; i < msize; i++) {
                float cardX = startX + i * 210.0f;
                if (pointInRect(wx, wy, cardX - 85.0f, cardY - 80.0f, 170.0f, 160.0f)) {
                    menuIndex = i;
                    runMenuAction(i);
                    return;
                }
            }
        } else if (currentPage == PAGE_MAP_CATEGORY) {
            float xs[5] = {winW * 0.18f, winW * 0.40f, winW * 0.62f, winW * 0.31f, winW * 0.53f};
            float ys[5] = {winH * 0.61f, winH * 0.61f, winH * 0.61f, winH * 0.34f, winH * 0.34f};
            for (int i = 0; i < msize; i++) {
                if (pointInRect(wx, wy, xs[i] - 85.0f, ys[i] - 80.0f, 170.0f, 160.0f)) {
                    menuIndex = i;
                    runMenuAction(i);
                    return;
                }
            }
        } else if (currentPage == PAGE_WEATHER) {
            float xs[4] = {winW * 0.30f, winW * 0.60f, winW * 0.30f, winW * 0.60f};
            float ys[4] = {winH * 0.61f, winH * 0.61f, winH * 0.34f, winH * 0.34f};
            for (int i = 0; i < msize; i++) {
                if (pointInRect(wx, wy, xs[i] - 85.0f, ys[i] - 80.0f, 170.0f, 160.0f)) {
                    menuIndex = i;
                    runMenuAction(i);
                    return;
                }
            }
        } else if (currentPage == PAGE_MENU) {
            float buttonY = winH * 0.58f;
            float buttonX = winW * 0.5f - 170.0f;
            for (int i = 0; i < msize; i++) {
                float bx = buttonX;
                float by = buttonY;
                float bw = 340.0f;
                float bh = 42.0f;
                if (pointInRect(wx, wy, bx, by, bw, bh)) {
                    menuIndex = i;
                    runMenuAction(i);
                    return;
                }
                buttonY -= 56.0f;
            }
        } else if (currentPage == PAGE_SETTINGS) {
            float buttonY = winH * 0.58f;
            float buttonX = winW * 0.5f - 170.0f;
            for (int i = 0; i < msize; i++) {
                float bx = buttonX;
                float by = buttonY;
                float bw = 340.0f;
                float bh = 42.0f;
                if (pointInRect(wx, wy, bx, by, bw, bh)) {
                    menuIndex = i;
                    runMenuAction(i);
                    return;
                }
                buttonY -= 56.0f;
            }
        }
    }
}

void reshape(int w, int h) {
    if (w < 800) w = 800;
    if (h < 600) h = 600;

    winW = w;
    winH = h;

    glViewport(0, 0, winW, winH);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, (double)winW, 0.0, (double)winH);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    paddleY = 48.0f;
    paddleX = clampf(paddleX, paddleW * 0.5f, winW - paddleW * 0.5f);
}

void update(int value) {
    (void)value;

    int now = glutGet(GLUT_ELAPSED_TIME);
    float dt = (now - prevTick) / 1000.0f;
    prevTick = now;
    dt = clampf(dt, 0.0f, 0.05f);

#ifdef _WIN32
    syncMusicForPage();
#endif

    if (currentPage == PAGE_PLAY) {
        updateGame(dt);
    }

    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

void init() {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, (double)winW, 0.0, (double)winH);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

int main(int argc, char** argv) {
    srand((unsigned int)time(NULL));

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(winW, winH);
    glutInitWindowPosition(60, 30);
    glutCreateWindow("DX Ball - OpenGL C++");

    init();
    loadScores();
    setupBricks();
    resetBall();

#ifdef _WIN32
    syncMusicForPage();
#endif

    prevTick = glutGet(GLUT_ELAPSED_TIME);

    glutDisplayFunc(display);
    glutKeyboardFunc(keyboardDown);
    glutKeyboardUpFunc(keyboardUp);
    glutSpecialFunc(specialDown);
    glutSpecialUpFunc(specialUp);
    glutPassiveMotionFunc(mouseMove);
    glutMotionFunc(mouseMove);
    glutMouseFunc(mouseClick);
    glutReshapeFunc(reshape);
    glutTimerFunc(16, update, 0);

    glutMainLoop();
    return 0;
}

