#include <GL/freeglut.h>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <cstdio>

const int MAX_BRICKS = 60;

struct Brick {
    float x;
    float y;
    float w;
    float h;
    int alive;
};

int winW = 1000;
int winH = 700;

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

int moveLeft = 0;
int moveRight = 0;

int lives = 3;
int gameOver = 0;
int gameWin = 0;

Brick bricks[MAX_BRICKS];
int brickCount = 0;
int prevTick = 0;

float clampf(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

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
            bricks[brickCount++] = b;
        }
    }
}

void resetBall() {
    paddleX = winW * 0.5f;
    ballAttached = 1;
    ballX = paddleX;
    ballY = paddleY + paddleH * 0.5f + ballR + 2.0f;
    float speed = 350.0f;
    ballVX = (rand() % 2 ? 1.0f : -1.0f) * speed * 0.55f;
    ballVY = speed * 0.85f;
}

void newGame() {
    lives = 3;
    gameOver = 0;
    gameWin = 0;
    setupBricks();
    resetBall();
}

void movePaddle(float dt) {
    if (moveLeft) paddleX -= paddleSpeed * dt;
    if (moveRight) paddleX += paddleSpeed * dt;
    paddleX = clampf(paddleX, paddleW * 0.5f, winW - paddleW * 0.5f);
}

void launchBall() {
    if (ballAttached && !gameOver && !gameWin) {
        ballAttached = 0;
    }
}

void paddleBounce() {
    float rx = paddleX - paddleW * 0.5f;
    float ry = paddleY - paddleH * 0.5f;
    if (!circleRectHit(ballX, ballY, ballR, rx, ry, paddleW, paddleH)) return;
    if (ballVY >= 0.0f) return;

    ballY = ry + paddleH + ballR + 0.5f;
    float rel = (ballX - paddleX) / (paddleW * 0.5f);
    rel = clampf(rel, -1.0f, 1.0f);

    float speed = 350.0f;
    float ang = (90.0f - rel * 65.0f) * 3.1415926535f / 180.0f;
    ballVX = cosf(ang) * speed;
    ballVY = sinf(ang) * speed;
}

int allBricksBroken() {
    for (int i = 0; i < brickCount; i++) {
        if (bricks[i].alive) return 0;
    }
    return 1;
}

void updateGame(float dt) {
    if (gameOver || gameWin) return;

    movePaddle(dt);

    if (ballAttached) {
        ballX = paddleX;
        ballY = paddleY + paddleH * 0.5f + ballR + 2.0f;
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

        float nx = clampf(ballX, b.x, b.x + b.w);
        float ny = clampf(ballY, b.y, b.y + b.h);
        float dx = ballX - nx;
        float dy = ballY - ny;
        if (fabs(dx) > fabs(dy)) ballVX = -ballVX;
        else ballVY = -ballVY;

        break;
    }

    if (ballY + ballR < 0.0f) {
        lives--;
        if (lives <= 0) {
            gameOver = 1;
        } else {
            resetBall();
        }
    }

    if (allBricksBroken()) {
        gameWin = 1;
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

void drawGame() {
    drawBackground();

    for (int i = 0; i < brickCount; i++) {
        if (!bricks[i].alive) continue;
        float shade = 0.25f + 0.75f * ((i % 10) / 10.0f);
        glColor3f(0.9f * shade, 0.5f, 0.2f + 0.5f * shade);
        drawRect(bricks[i].x, bricks[i].y, bricks[i].w, bricks[i].h);
    }

    glColor3f(0.92f, 0.96f, 1.0f);
    drawRect(paddleX - paddleW * 0.5f, paddleY - paddleH * 0.5f, paddleW, paddleH);

    glColor3f(1.0f, 0.84f, 0.25f);
    drawCircle(ballX, ballY, ballR);

    char hud[64];
    std::snprintf(hud, sizeof(hud), "Lives: %d", lives);
    glColor3f(0.92f, 0.95f, 1.0f);
    drawText(24.0f, winH - 34.0f, hud);

    if (ballAttached && !gameOver && !gameWin) {
        drawTextCenter(winW * 0.5f, 90.0f, "Press SPACE or Left Click to launch");
    }

    if (gameOver) {
        glColor4f(0.0f, 0.0f, 0.0f, 0.55f);
        drawRect(0.0f, 0.0f, (float)winW, (float)winH);
        glColor3f(1.0f, 0.35f, 0.35f);
        drawTextCenter(winW * 0.5f, winH * 0.55f, "GAME OVER", GLUT_BITMAP_TIMES_ROMAN_24);
        glColor3f(0.92f, 0.95f, 1.0f);
        drawTextCenter(winW * 0.5f, winH * 0.47f, "Press N for new game");
    }

    if (gameWin) {
        glColor4f(0.0f, 0.0f, 0.0f, 0.55f);
        drawRect(0.0f, 0.0f, (float)winW, (float)winH);
        glColor3f(0.32f, 0.95f, 0.55f);
        drawTextCenter(winW * 0.5f, winH * 0.55f, "YOU WIN!", GLUT_BITMAP_TIMES_ROMAN_24);
        glColor3f(0.92f, 0.95f, 1.0f);
        drawTextCenter(winW * 0.5f, winH * 0.47f, "Press N for new game");
    }
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();
    drawGame();
    glutSwapBuffers();
}

void keyboardDown(unsigned char key, int x, int y) {
    (void)x;
    (void)y;

    if (key == 'a' || key == 'A') moveLeft = 1;
    else if (key == 'd' || key == 'D') moveRight = 1;
    else if (key == ' ') launchBall();
    else if (key == 'n' || key == 'N') newGame();
    else if (key == 'q' || key == 'Q') exit(0);
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
    if (key == GLUT_KEY_LEFT) moveLeft = 1;
    else if (key == GLUT_KEY_RIGHT) moveRight = 1;
}

void specialUp(int key, int x, int y) {
    (void)x;
    (void)y;
    if (key == GLUT_KEY_LEFT) moveLeft = 0;
    else if (key == GLUT_KEY_RIGHT) moveRight = 0;
}

void mouseMove(int x, int y) {
    (void)y;
    paddleX = clampf((float)x, paddleW * 0.5f, winW - paddleW * 0.5f);
}

void mouseClick(int button, int state, int x, int y) {
    (void)x;
    (void)y;
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        launchBall();
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

    updateGame(dt);

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
    glutCreateWindow("DX Ball Part 1 - Core Game");

    init();
    newGame();

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
