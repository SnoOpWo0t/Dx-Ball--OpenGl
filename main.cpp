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


//core ball speed computation and integration with power-ups

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

// Feature: Rendering & UI helpers
// Provided basic text/shape drawing utilities used by menus

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

    while (highCount < MAX_SCORES) {
        int s = 0;
        float t = 0.0f;
        if (fscanf(fp, "%d %f", &s, &t) != 2) {
            break;
        }
        highscores[highCount].score = s;
        highscores[highCount].timeSec = t;
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
        fprintf(fp, "%d %.2f\n", highscores[i].score, highscores[i].timeSec);
    }
    fclose(fp);
}
