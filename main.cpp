#ifdef _WIN32
#include <windows.h>
#include <direct.h>
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

// drop items
// Feature: Power-up drops
// implemented drop types, Drop struct and spawn/apply mechanics
const int DROP_NONE = 0;
const int DROP_LIFE = 1;
const int DROP_SPEED = 2;
const int DROP_WIDE = 3;

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
};

struct Drop {
    float x;
    float y;
    float size;
    float vy;
    int type;
    int active;
};

struct ScoreLine {
    int score;
    float timeSec;
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

int moveLeft = 0;
int moveRight = 0;

int lives = 3;
int score = 0;
float playTimeSec = 0.0f;
int scoreSaved = 0;

int brickCount = 0;
Brick bricks[MAX_BRICKS];

int dropCount = 0;
Drop drops[MAX_DROPS];

ScoreLine highscores[MAX_SCORES];
int highCount = 0;

int prevTick = 0;

const char* scorePath = ".dist/highscores.txt";

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
    return s;
}
