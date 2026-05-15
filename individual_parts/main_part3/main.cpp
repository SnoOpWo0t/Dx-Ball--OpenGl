#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

#include <GL/freeglut.h>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <cstdio>

const int MAX_BRICKS = 60;
const int MAX_DROPS = 64;
const int MAX_SCORES = 10;

const int PAGE_MENU = 0;
const int PAGE_PLAY = 1;
const int PAGE_PAUSE = 2;
const int PAGE_WIN = 3;
const int PAGE_OVER = 4;
const int PAGE_SCORE = 5;

const int DIFFICULTY_EASY = 0;
const int DIFFICULTY_NORMAL = 1;
const int DIFFICULTY_HARD = 2;

const int DROP_NONE = 0;
const int DROP_LIFE = 1;
const int DROP_SPEED = 2;
const int DROP_WIDE = 3;

struct Brick {
    float x;
    float y;
    float w;
    float h;
    int alive;
    int points;
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
    int difficulty;
};

int winW = 1000;
int winH = 700;

int currentPage = PAGE_MENU;
int selectedDifficulty = DIFFICULTY_NORMAL;
int menuIndex = 0;

float paddleX = 500.0f;
float paddleY = 48.0f;
float paddleW = 130.0f;
float paddleH = 18.0f;
float paddleSpeed = 560.0f;
float paddleNormalW = 130.0f;
float paddleWideW = 190.0f;
float wideTimer = 0.0f;
float wideDur = 12.0f;

float ballX = 500.0f;
float ballY = 90.0f;
float ballR = 9.0f;
float ballVX = 180.0f;
float ballVY = 300.0f;
int ballAttached = 1;
float baseBallSpeed = 340.0f;
float speedBoostTimer = 0.0f;
float speedBoostDur = 8.0f;

int moveLeft = 0;
int moveRight = 0;

int lives = 3;
int score = 0;
float playTimeSec = 0.0f;
int scoreSaved = 0;

Brick bricks[MAX_BRICKS];
int brickCount = 0;
Drop drops[MAX_DROPS];
int dropCount = 0;
ScoreLine highscores[MAX_SCORES];
int highCount = 0;

int prevTick = 0;
const char* scorePath = ".dist/highscores_part3.txt";

float clampf(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

float ballSpeedNow() {
    float s = baseBallSpeed;
    if (speedBoostTimer > 0.0f) s *= 1.30f;
    return s;
}

void scaleBallVector() {
    if (ballAttached) return;
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

void drawText(float x, float y, const char* text, void* font = GLUT_BITMAP_HELVETICA_18) {
    glRasterPos2f(x, y);
    for (int i = 0; text[i] != '\0'; i++) glutBitmapCharacter(font, text[i]);
}

int textWidth(const char* text, void* font = GLUT_BITMAP_HELVETICA_18) {
    int w = 0;
    for (int i = 0; text[i] != '\0'; i++) w += glutBitmapWidth(font, text[i]);
    return w;
}

void drawTextCenter(float cx, float y, const char* text, void* font = GLUT_BITMAP_HELVETICA_18) {
    drawText(cx - textWidth(text, font) * 0.5f, y, text, font);
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

int circleRectHit(float cx, float cy, float cr, float rx, float ry, float rw, float rh) {
    float nx = clampf(cx, rx, rx + rw);
    float ny = clampf(cy, ry, ry + rh);
    float dx = cx - nx;
    float dy = cy - ny;
    return (dx * dx + dy * dy) <= (cr * cr);
}

const char* difficultyText(int d) {
    if (d == DIFFICULTY_EASY) return "Easy";
    if (d == DIFFICULTY_HARD) return "Hard";
    return "Normal";
}

const char* timeText(float sec) {
    static char buff[64];
    int total = (int)sec;
    int m = total / 60;
    int s = total % 60;
    sprintf(buff, "%d:%02d", m, s);
    return buff;
}

void sortScores() {
    for (int i = 0; i < highCount; i++) {
        for (int j = i + 1; j < highCount; j++) {
            int swapNow = 0;
            if (highscores[j].score > highscores[i].score) swapNow = 1;
            else if (highscores[j].score == highscores[i].score && highscores[j].timeSec < highscores[i].timeSec) swapNow = 1;

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
    if (!fp) return;

    while (highCount < MAX_SCORES) {
        int s = 0;
        float t = 0.0f;
        int d = DIFFICULTY_NORMAL;
        if (fscanf(fp, "%d %f %d", &s, &t, &d) != 3) break;
        highscores[highCount].score = s;
        highscores[highCount].timeSec = t;
        highscores[highCount].difficulty = d;
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
    if (!fp) return;
    for (int i = 0; i < highCount; i++) {
        fprintf(fp, "%d %.2f %d\n", highscores[i].score, highscores[i].timeSec, highscores[i].difficulty);
    }
    fclose(fp);
}

void submitScore() {
    if (scoreSaved) return;
    scoreSaved = 1;

    if (highCount < MAX_SCORES) {
        highscores[highCount].score = score;
        highscores[highCount].timeSec = playTimeSec;
        highscores[highCount].difficulty = selectedDifficulty;
        highCount++;
    } else {
        highscores[MAX_SCORES - 1].score = score;
        highscores[MAX_SCORES - 1].timeSec = playTimeSec;
        highscores[MAX_SCORES - 1].difficulty = selectedDifficulty;
    }

    sortScores();
    if (highCount > MAX_SCORES) highCount = MAX_SCORES;
    saveScores();
}

void setupBricks() {
    brickCount = 0;
    int rows = 6;
    int cols = 10;
    float gap = 8.0f;
    float bh = 28.0f;
    float margin = 56.0f;
    float startY = winH - 72.0f;
    float bw = (winW - 2.0f * margin - (cols - 1) * gap) / cols;

    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            Brick b;
            b.x = margin + c * (bw + gap);
            b.y = startY - r * (bh + gap);
            b.w = bw;
            b.h = bh;
            b.alive = 1;
            b.points = (rows - r) * 10;
            bricks[brickCount++] = b;
        }
    }
}

void clearDrops() {
    for (int i = 0; i < dropCount; i++) drops[i].active = 0;
}

void resetBall() {
    paddleW = (wideTimer > 0.0f) ? paddleWideW : paddleNormalW;
    paddleX = winW * 0.5f;
    ballAttached = 1;
    ballX = paddleX;
    ballY = paddleY + paddleH * 0.5f + ballR + 2.0f;
    ballVX = (rand() % 2 ? 1.0f : -1.0f) * ballSpeedNow() * 0.55f;
    ballVY = ballSpeedNow() * 0.85f;
}

void spawnDrop(float x, float y) {
    if (dropCount >= MAX_DROPS) return;

    int chance = rand() % 100;
    if (chance > 65) return;

    Drop d;
    d.x = x;
    d.y = y;
    d.size = 18.0f;
    d.vy = 150.0f;
    d.active = 1;

    int roll = rand() % 100;
    if (roll < 25) d.type = DROP_LIFE;
    else if (roll < 55) d.type = DROP_SPEED;
    else d.type = DROP_WIDE;

    drops[dropCount++] = d;
}

void applyDrop(int type) {
    if (type == DROP_LIFE) {
        lives++;
    } else if (type == DROP_SPEED) {
        speedBoostTimer = speedBoostDur;
        scaleBallVector();
    } else if (type == DROP_WIDE) {
        wideTimer = wideDur;
        paddleW = paddleWideW;
        paddleX = clampf(paddleX, paddleW * 0.5f, winW - paddleW * 0.5f);
    }
}

void updateDrops(float dt) {
    for (int i = 0; i < dropCount; i++) {
        if (!drops[i].active) continue;

        drops[i].y -= drops[i].vy * dt;

        float rx = paddleX - paddleW * 0.5f;
        float ry = paddleY - paddleH * 0.5f;
        if (circleRectHit(drops[i].x, drops[i].y, drops[i].size * 0.5f, rx, ry, paddleW, paddleH)) {
            drops[i].active = 0;
            applyDrop(drops[i].type);
            continue;
        }

        if (drops[i].y + drops[i].size < 0.0f) drops[i].active = 0;
    }
}

void startGame() {
    if (selectedDifficulty == DIFFICULTY_EASY) {
        baseBallSpeed = 290.0f;
        lives = 5;
    } else if (selectedDifficulty == DIFFICULTY_NORMAL) {
        baseBallSpeed = 340.0f;
        lives = 3;
    } else {
        baseBallSpeed = 420.0f;
        lives = 2;
    }

    score = 0;
    playTimeSec = 0.0f;
    scoreSaved = 0;
    speedBoostTimer = 0.0f;
    wideTimer = 0.0f;
    paddleW = paddleNormalW;
    dropCount = 0;

    setupBricks();
    resetBall();
    currentPage = PAGE_PLAY;
}

int allBricksBroken() {
    for (int i = 0; i < brickCount; i++) {
        if (bricks[i].alive) return 0;
    }
    return 1;
}

void launchBall() {
    if (currentPage == PAGE_PLAY && ballAttached) ballAttached = 0;
}

void movePaddle(float dt) {
    if (moveLeft) paddleX -= paddleSpeed * dt;
    if (moveRight) paddleX += paddleSpeed * dt;
    paddleX = clampf(paddleX, paddleW * 0.5f, winW - paddleW * 0.5f);
}

void paddleBounce() {
    float rx = paddleX - paddleW * 0.5f;
    float ry = paddleY - paddleH * 0.5f;
    if (!circleRectHit(ballX, ballY, ballR, rx, ry, paddleW, paddleH)) return;
    if (ballVY >= 0.0f) return;

    ballY = ry + paddleH + ballR + 0.5f;
    float rel = clampf((ballX - paddleX) / (paddleW * 0.5f), -1.0f, 1.0f);
    float ang = (90.0f - rel * 65.0f) * 3.1415926535f / 180.0f;
    float s = ballSpeedNow();
    ballVX = cosf(ang) * s;
    ballVY = sinf(ang) * s;
}

void updateTimers(float dt) {
    if (speedBoostTimer > 0.0f) {
        speedBoostTimer -= dt;
        if (speedBoostTimer <= 0.0f) {
            speedBoostTimer = 0.0f;
            scaleBallVector();
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
}

void updateGame(float dt) {
    playTimeSec += dt;
    updateTimers(dt);
    movePaddle(dt);

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

    for (int i = 0; i < brickCount; i++) {
        if (!bricks[i].alive) continue;
        Brick& b = bricks[i];
        if (!circleRectHit(ballX, ballY, ballR, b.x, b.y, b.w, b.h)) continue;

        b.alive = 0;
        score += b.points;
        spawnDrop(b.x + b.w * 0.5f, b.y + b.h * 0.5f);

        float nx = clampf(ballX, b.x, b.x + b.w);
        float ny = clampf(ballY, b.y, b.y + b.h);
        float dx = ballX - nx;
        float dy = ballY - ny;
        if (fabs(dx) > fabs(dy)) ballVX = -ballVX;
        else ballVY = -ballVY;

        break;
    }

    updateDrops(dt);

    if (ballY + ballR < 0.0f) {
        lives--;
        clearDrops();
        if (lives <= 0) {
            currentPage = PAGE_OVER;
            submitScore();
        } else {
            resetBall();
        }
    }

    if (allBricksBroken()) {
        currentPage = PAGE_WIN;
        submitScore();
    }
}

void drawBackground() {
    glBegin(GL_QUADS);
    glColor3f(0.05f, 0.08f, 0.15f);
    glVertex2f(0.0f, 0.0f);
    glVertex2f((float)winW, 0.0f);
    glColor3f(0.10f, 0.14f, 0.25f);
    glVertex2f((float)winW, (float)winH);
    glColor3f(0.06f, 0.10f, 0.18f);
    glVertex2f(0.0f, (float)winH);
    glEnd();
}

void drawDrops() {
    for (int i = 0; i < dropCount; i++) {
        if (!drops[i].active) continue;

        if (drops[i].type == DROP_LIFE) glColor3f(0.95f, 0.36f, 0.40f);
        else if (drops[i].type == DROP_SPEED) glColor3f(0.96f, 0.72f, 0.18f);
        else glColor3f(0.20f, 0.78f, 0.45f);

        drawCircle(drops[i].x, drops[i].y, drops[i].size * 0.5f);

        glColor3f(0.08f, 0.08f, 0.1f);
        if (drops[i].type == DROP_LIFE) drawTextCenter(drops[i].x, drops[i].y - 4.0f, "+1", GLUT_BITMAP_HELVETICA_12);
        else if (drops[i].type == DROP_SPEED) drawTextCenter(drops[i].x, drops[i].y - 4.0f, "S", GLUT_BITMAP_HELVETICA_12);
        else drawTextCenter(drops[i].x, drops[i].y - 4.0f, "W", GLUT_BITMAP_HELVETICA_12);
    }
}

void drawGameScene() {
    drawBackground();

    for (int i = 0; i < brickCount; i++) {
        if (!bricks[i].alive) continue;
        float shade = 0.25f + 0.75f * ((i % 10) / 10.0f);
        glColor3f(0.9f * shade, 0.5f, 0.2f + 0.5f * shade);
        drawRect(bricks[i].x, bricks[i].y, bricks[i].w, bricks[i].h);
    }

    drawDrops();

    glColor3f(0.92f, 0.96f, 1.0f);
    drawRect(paddleX - paddleW * 0.5f, paddleY - paddleH * 0.5f, paddleW, paddleH);

    glColor3f(1.0f, 0.84f, 0.25f);
    drawCircle(ballX, ballY, ballR);

    char line[160];
    glColor3f(0.92f, 0.95f, 1.0f);
    sprintf(line, "Lives: %d", lives);
    drawText(24.0f, winH - 34.0f, line);
    sprintf(line, "Score: %d", score);
    drawText(180.0f, winH - 34.0f, line);
    sprintf(line, "Time: %s", timeText(playTimeSec));
    drawText(340.0f, winH - 34.0f, line);
    sprintf(line, "Difficulty: %s", difficultyText(selectedDifficulty));
    drawText(500.0f, winH - 34.0f, line);

    if (wideTimer > 0.0f) {
        sprintf(line, "Wide: %ds", (int)ceil(wideTimer));
        drawText(760.0f, winH - 34.0f, line);
    }

    if (speedBoostTimer > 0.0f) {
        sprintf(line, "Speed: %ds", (int)ceil(speedBoostTimer));
        drawText(860.0f, winH - 34.0f, line);
    }

    if (ballAttached) {
        drawTextCenter(winW * 0.5f, 90.0f, "Press SPACE or Left Click to launch");
    }
}

void drawMenu() {
    drawBackground();

    glColor3f(0.95f, 0.90f, 0.28f);
    drawTextCenter(winW * 0.5f, winH - 110.0f, "DX BALL PART 3", GLUT_BITMAP_TIMES_ROMAN_24);

    glColor3f(0.86f, 0.92f, 0.99f);
    drawTextCenter(winW * 0.5f, winH - 150.0f, "Now includes drops and local highscores");

    for (int i = 0; i < 3; i++) {
        float y = winH * 0.56f - i * 70.0f;
        if (menuIndex == i) glColor3f(0.20f, 0.56f, 0.98f);
        else glColor3f(0.10f, 0.14f, 0.22f);
        drawRect(winW * 0.5f - 160.0f, y - 18.0f, 320.0f, 42.0f);
        glColor3f(0.94f, 0.96f, 1.0f);
        drawTextCenter(winW * 0.5f, y - 4.0f, difficultyText(i));
    }

    glColor3f(0.86f, 0.9f, 0.97f);
    drawTextCenter(winW * 0.5f, 130.0f, "H: Highscores, Enter: Start, Q: Quit", GLUT_BITMAP_HELVETICA_12);
    drawTextCenter(winW * 0.5f, 100.0f, "W/S or Up/Down to choose difficulty", GLUT_BITMAP_HELVETICA_12);
}

void drawScores() {
    drawBackground();

    glColor3f(0.96f, 0.93f, 0.33f);
    drawTextCenter(winW * 0.5f, winH - 110.0f, "HIGHSCORES", GLUT_BITMAP_TIMES_ROMAN_24);

    if (highCount == 0) {
        glColor3f(0.85f, 0.9f, 1.0f);
        drawTextCenter(winW * 0.5f, winH * 0.5f, "No records yet");
    } else {
        float y = winH - 180.0f;
        for (int i = 0; i < highCount; i++) {
            char line[200];
            sprintf(line, "%d. %d pts  |  %s  |  %s", i + 1, highscores[i].score, difficultyText(highscores[i].difficulty), timeText(highscores[i].timeSec));
            glColor3f(0.90f, 0.95f, 1.0f);
            drawTextCenter(winW * 0.5f, y, line);
            y -= 36.0f;
        }
    }

    glColor3f(0.86f, 0.9f, 0.97f);
    drawTextCenter(winW * 0.5f, 90.0f, "Press M or ESC to return");
}

void drawPause() {
    drawGameScene();
    glColor4f(0.0f, 0.0f, 0.0f, 0.55f);
    drawRect(0.0f, 0.0f, (float)winW, (float)winH);
    glColor3f(0.95f, 0.95f, 1.0f);
    drawTextCenter(winW * 0.5f, winH * 0.52f, "PAUSED", GLUT_BITMAP_TIMES_ROMAN_24);
    drawTextCenter(winW * 0.5f, winH * 0.45f, "Press P or R to resume, ESC for menu");
}

void drawResult(int won) {
    drawGameScene();
    glColor4f(0.0f, 0.0f, 0.0f, 0.55f);
    drawRect(0.0f, 0.0f, (float)winW, (float)winH);

    if (won) {
        glColor3f(0.32f, 0.95f, 0.55f);
        drawTextCenter(winW * 0.5f, winH * 0.58f, "YOU WIN!", GLUT_BITMAP_TIMES_ROMAN_24);
    } else {
        glColor3f(1.0f, 0.35f, 0.35f);
        drawTextCenter(winW * 0.5f, winH * 0.58f, "GAME OVER", GLUT_BITMAP_TIMES_ROMAN_24);
    }

    char line[128];
    sprintf(line, "Final Score: %d   Time: %s", score, timeText(playTimeSec));
    glColor3f(0.94f, 0.97f, 1.0f);
    drawTextCenter(winW * 0.5f, winH * 0.50f, line);
    drawTextCenter(winW * 0.5f, winH * 0.44f, "Press N for new game or M for menu");
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    if (currentPage == PAGE_MENU) drawMenu();
    else if (currentPage == PAGE_PLAY) drawGameScene();
    else if (currentPage == PAGE_PAUSE) drawPause();
    else if (currentPage == PAGE_WIN) drawResult(1);
    else if (currentPage == PAGE_OVER) drawResult(0);
    else if (currentPage == PAGE_SCORE) drawScores();

    glutSwapBuffers();
}

void keyboardDown(unsigned char key, int x, int y) {
    (void)x;
    (void)y;

    if (key == 'q' || key == 'Q') exit(0);

    if (currentPage == PAGE_PLAY) {
        if (key == 'a' || key == 'A') moveLeft = 1;
        else if (key == 'd' || key == 'D') moveRight = 1;
        else if (key == ' ') launchBall();
        else if (key == 'p' || key == 'P') currentPage = PAGE_PAUSE;
        else if (key == 27) currentPage = PAGE_MENU;
        return;
    }

    if (currentPage == PAGE_PAUSE) {
        if (key == 'p' || key == 'P' || key == 'r' || key == 'R') currentPage = PAGE_PLAY;
        else if (key == 27) currentPage = PAGE_MENU;
        return;
    }

    if (currentPage == PAGE_MENU) {
        if (key == 'w' || key == 'W') {
            menuIndex--;
            if (menuIndex < 0) menuIndex = 2;
        } else if (key == 's' || key == 'S') {
            menuIndex++;
            if (menuIndex > 2) menuIndex = 0;
        } else if (key == 13 || key == ' ') {
            selectedDifficulty = menuIndex;
            startGame();
        } else if (key == 'h' || key == 'H') {
            loadScores();
            currentPage = PAGE_SCORE;
        }
        return;
    }

    if (currentPage == PAGE_SCORE) {
        if (key == 'm' || key == 'M' || key == 27) currentPage = PAGE_MENU;
        return;
    }

    if (currentPage == PAGE_WIN || currentPage == PAGE_OVER) {
        if (key == 'n' || key == 'N') startGame();
        else if (key == 'm' || key == 'M' || key == 27) currentPage = PAGE_MENU;
    }
}

void keyboardUp(unsigned char key, int x, int y) {
    (void)x;
    (void)y;
    if (key == 'a' || key == 'A') moveLeft = 0;
    else if (key == 'd' || key == 'D') moveRight = 0;
}

void specialDown(int key, int x, int y) {
    (void)x;
    (void)y;

    if (currentPage == PAGE_PLAY) {
        if (key == GLUT_KEY_LEFT) moveLeft = 1;
        else if (key == GLUT_KEY_RIGHT) moveRight = 1;
    } else if (currentPage == PAGE_MENU) {
        if (key == GLUT_KEY_UP) {
            menuIndex--;
            if (menuIndex < 0) menuIndex = 2;
        } else if (key == GLUT_KEY_DOWN) {
            menuIndex++;
            if (menuIndex > 2) menuIndex = 0;
        }
    }
}

void specialUp(int key, int x, int y) {
    (void)x;
    (void)y;
    if (key == GLUT_KEY_LEFT) moveLeft = 0;
    else if (key == GLUT_KEY_RIGHT) moveRight = 0;
}

void mouseMove(int x, int y) {
    (void)y;
    if (currentPage == PAGE_PLAY || currentPage == PAGE_PAUSE) {
        paddleX = clampf((float)x, paddleW * 0.5f, winW - paddleW * 0.5f);
    }
}

void mouseClick(int button, int state, int x, int y) {
    (void)x;
    (void)y;
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN && currentPage == PAGE_PLAY) launchBall();
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

    if (currentPage == PAGE_PLAY) updateGame(dt);

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
    glutCreateWindow("DX Ball Part 3 - Drops and Highscores");

    init();
    loadScores();
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
