
// Brick-breaker with themed backgrounds, weather, 10 brick types,
// 9 power/curse drops, multiball, combo system, persistent scoreboard.

// === 1. Includes + macros + constants ===
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

// Pool sizes
#define MAX_BRICKS      60
#define MAX_DROPS       100
#define MAX_SCORES      10
#define MAX_EXTRA_BALLS 5
#define MAX_FTEXTS      32
#define MAX_PARTS       128

// Drawing shorthands
#define V2(x,y)     glVertex2f(x,y)
#define C3(r,g,b)   glColor3f(r,g,b)
#define C4(r,g,b,a) glColor4f(r,g,b,a)

// Bitmap fonts
#define F18 GLUT_BITMAP_HELVETICA_18
#define F12 GLUT_BITMAP_HELVETICA_12
#define FTR GLUT_BITMAP_TIMES_ROMAN_24

#define PIF 3.1415926535f

// === 2. Enums + structs + forward decls ===

// Page IDs cover menu flow + gameplay + overlays
enum { PAGE_MENU=0, PAGE_PLAY, PAGE_PAUSE, PAGE_SCORE, PAGE_HELP, PAGE_WIN, PAGE_OVER,
       PAGE_DIFFICULTY, PAGE_MAP_CATEGORY, PAGE_MAP_SELECT, PAGE_WEATHER, PAGE_SETTINGS };
enum { DIFFICULTY_EASY=0, DIFFICULTY_NORMAL, DIFFICULTY_HARD, DIFFICULTY_HARDEST };
enum { MAP_CAT_CLASSIC=0, MAP_CAT_CHECKERBOARD, MAP_CAT_CIRCLE, MAP_CAT_PYRAMID, MAP_CAT_SPECIAL };
enum { WEATHER_SUNNY=0, WEATHER_RAINY, WEATHER_STORMY, WEATHER_NIGHT };
enum { THEME_NATURE=0, THEME_CITY, THEME_SPACE, THEME_NEON };

// Falling drop kinds (paddle catches these)
//   LIFE +1, SPEED ball boost, WIDE paddle, BOMB BAD, SHIELD absorb-one,
//   ROW_CLEAR wipe top, FIREBALL plow-thru, MULTIBALL x1.5, KEY unlocks
enum { DROP_NONE=0, DROP_LIFE, DROP_SPEED, DROP_WIDE, DROP_BOMB, DROP_SHIELD,
       DROP_ROW_CLEAR, DROP_FIREBALL, DROP_MULTIBALL, DROP_KEY };

// Brick type:
//   0 normal     | 1 tough (2 HP)  | 2 bomb (chain)  | 3 moving | 4 ghost
//   5 ice (slow) | 6 fire (fast)   | 7 locked (key)  | 8 regen  | 9 teleport
struct Brick     { float x,y,w,h, r,g,b; int points,alive,type,hp;
                   float regenTimer,moveDir; int isLocked,linkedBrick; };
struct Drop      { float x,y,size,vy; int type,active; };
struct ScoreLine { int score; float timeSec; int mapCategory,difficulty; };
struct FloatText { float x,y,vy,life,r,g,b; char txt[48]; };
struct Particle  { float x,y,vx,vy,life,r,g,b; };

// Forward declarations
void  addFloatText(float x,float y,const char* txt, float r=1,float g=1,float b=1);
void  assignBrickType(Brick& b);
void  drawMapPreview(int mapNum, float px, float py, float scale, int isSelected);
void  resetBall();
void  newGame();
void  quitGame();
void  scaleBallVector();
float ballSpeedNow();

// === 3. Globals ===

// Menu selection + cursor state
int selectedDifficulty=DIFFICULTY_NORMAL, difficultyIndex=DIFFICULTY_NORMAL;
int selectedMap=1, mapIndex=1;
int selectedMapCategory=MAP_CAT_CLASSIC, mapCategoryIndex=MAP_CAT_CLASSIC;
int selectedWeather=WEATHER_SUNNY, weatherIndex=WEATHER_SUNNY;
int selectedTheme=THEME_NATURE, themeIndex=THEME_NATURE, selectedMapVariant=0;
float mouseX=-1000, mouseY=-1000;
int   mouseVisible=0;

// Weather/lightning timers (stormy weather flashes screen)
float lightningTimer=0, lightningDuration=0.15f, stormIntensity=0, weatherTimer=0;
int   showLightning=0;

// Window + page state
int winW=1000, winH=700, currentPage=PAGE_MENU, canResume=0, menuIndex=0;

// Paddle + ball
float paddleX=500, paddleY=48, paddleW=130, paddleH=18, paddleSpeed=560;
float paddleNormalW=130, paddleWideW=190;
float ballX=500, ballY=90, ballR=9, ballVX=180, ballVY=300;
int   ballAttached=1, moveLeft=0, moveRight=0, lives=3, score=0, scoreSaved=0, hasKey=0;

// Run-time scalers (speed ramp grows every 12s)
float playTimeSec=0, keyAssistCooldown=0;
float speedRamp=1, speedRampNext=12, speedRampStep=0.06f, speedRampMax=2.2f;
float baseBallSpeed=340;

// Power-up timers + magnitudes
float speedBoostTimer=0,    speedBoostMul=1.30f, speedBoostDur=10;
float wideTimer=0,          wideDur=12;
float shieldTimer=0,        shieldDur=8;
float fireballTimer=0,      fireballDur=8;
float iceBallSlowTimer=0,   iceBallSlowDur=6;
float fireBallSpeedTimer=0, fireBallSpeedDur=6;

// Combo (chain of brick hits)
int   comboCount=0;
float comboTimer=0, comboDecay=2.0f;

// Multiball extras
int   extraBallActive[MAX_EXTRA_BALLS];
float extraBallX[MAX_EXTRA_BALLS],  extraBallY[MAX_EXTRA_BALLS];
float extraBallVX[MAX_EXTRA_BALLS], extraBallVY[MAX_EXTRA_BALLS];
float extraBallR=7;
float extraBallR_val[MAX_EXTRA_BALLS], extraBallG_val[MAX_EXTRA_BALLS], extraBallB_val[MAX_EXTRA_BALLS];
int   extraBallMultiplierActive=0;
float extraBallScoreMul=1.5f;

// FX pools + camera shake
FloatText ftexts[MAX_FTEXTS];
Particle  parts[MAX_PARTS];
float     shakeTimer=0, shakeIntensity=0;

// Brick + drop pools
int   brickCount=0; Brick bricks[MAX_BRICKS];
int   dropCount=0;  Drop  drops[MAX_DROPS];

// Persistent score table + paths
ScoreLine   highscores[MAX_SCORES];
int         highCount=0, prevTick=0;
const char* scorePath=".dist/highscores.txt";
int         musicVolumePercent=75;

// === 4. Drawing primitives ===
float clampf(float v,float lo,float hi){ return v<lo?lo:(v>hi?hi:v); }
float deg2rad(float d){ return d*PIF/180.0f; }
int pointInRect(float px,float py,float rx,float ry,float rw,float rh){
    return px>=rx&&px<=rx+rw&&py>=ry&&py<=ry+rh;
}

// Circle-vs-AABB overlap test
int circleRectHit(float cx,float cy,float cr,float rx,float ry,float rw,float rh){
    float nx=clampf(cx,rx,rx+rw), ny=clampf(cy,ry,ry+rh);
    float dx=cx-nx, dy=cy-ny;
    return (dx*dx+dy*dy) <= (cr*cr);
}

void drawRect(float x,float y,float w,float h){
    glBegin(GL_QUADS); V2(x,y); V2(x+w,y); V2(x+w,y+h); V2(x,y+h); glEnd();
}
void drawCircle(float cx,float cy,float r){
    glBegin(GL_TRIANGLE_FAN); V2(cx,cy);
    for(int i=0;i<=32;i++){ float a=(2*PIF*i)/32.0f; V2(cx+cosf(a)*r,cy+sinf(a)*r); }
    glEnd();
}
void drawLineLoopRect(float x,float y,float w,float h){
    glBegin(GL_LINE_LOOP); V2(x,y); V2(x+w,y); V2(x+w,y+h); V2(x,y+h); glEnd();
}
void drawText(float x,float y,const char* text,void* font=F18){
    glRasterPos2f(x,y);
    for(int i=0;text[i];i++) glutBitmapCharacter(font,text[i]);
}
int textWidth(const char* text,void* font=F18){
    int w=0; for(int i=0;text[i];i++) w+=glutBitmapWidth(font,text[i]); return w;
}
void drawTextCenter(float cx,float y,const char* text,void* font=F18){
    drawText(cx - textWidth(text,font)*0.5f, y, text, font);
}

// Two-color vertical gradients
void gradHalfSky(float r1,float g1,float b1, float r2,float g2,float b2){
    glBegin(GL_QUADS);
    C3(r1,g1,b1); V2(0,winH*0.5f); V2(winW,winH*0.5f);
    C3(r2,g2,b2); V2(winW,0);      V2(0,0);
    glEnd();
}
void gradFull(float r1,float g1,float b1, float r2,float g2,float b2){
    glBegin(GL_QUADS);
    C3(r1,g1,b1); V2(0,winH); V2(winW,winH);
    C3(r2,g2,b2); V2(winW,0); V2(0,0);
    glEnd();
}

// === 5. Audio (Windows MCI music + SFX) ===
#ifdef _WIN32

// Music track IDs
const int MUSIC_NONE=-1, MUSIC_MENU=0, MUSIC_GAME=1, MUSIC_RESULT=2, MUSIC_SCORE=3;
int currentMusicTrack=MUSIC_NONE, mciMusicActive=0, lastMusicStopTime=0;

// MCI uses 0..1000 volume scale
int mciVolumeFromPercent(int p){ if(p<0)p=0; if(p>100)p=100; return (p*1000)/100; }
void applyMusicVolume(){
    char cmd[96];
    snprintf(cmd,sizeof(cmd),"set dxball_bgm audio all volume to %d",
             mciVolumeFromPercent(musicVolumePercent));
    mciSendStringA(cmd,NULL,0,NULL);
}

// Per-track search paths (first existing file wins)
static const char* menuMusicPaths[]   = {
    "Game Music/08. In The Eyes(Menu Bgm).mp3",
    ".dist/menu_music.wav", "menu_music.wav", ".dist/music.wav", "music.wav" };
static const char* gameMusicPaths[]   = {
    "Game Music/10. Ultimate Battle (in game music).wav",
    "Game Music/10. Ultimate Battle (in game music).mp3",
    ".dist/game_music.wav", "game_music.wav", ".dist/music.wav", "music.wav" };
static const char* resultMusicPaths[] = {
    "Game Music/Rigor Mormist (gameover music).wav",
    "Game Music/Rigor Mormist (gameover music).mp3",
    ".dist/win_music.wav", "win_music.wav",
    ".dist/game_music.wav", "game_music.wav", ".dist/music.wav", "music.wav" };
static const char* scoreMusicPaths[]  = {
    "Game Music/10. High Score.mp3", ".dist/score_music.wav", "score_music.wav" };

int isWavPath(const char* p){
    const char* e=strrchr(p,'.'); if(!e||e[0]!='.') return 0;
    return (e[1]=='w'||e[1]=='W')&&(e[2]=='a'||e[2]=='A')&&(e[3]=='v'||e[3]=='V')&&e[4]==0;
}

// Resolve relative path to absolute next-to-exe path
void buildMusicPath(const char* rel, char* out, int mx){
    char ex[MAX_PATH]; GetModuleFileNameA(NULL,ex,sizeof(ex));
    char* ls=strrchr(ex,'\\'); if(ls) *ls=0;
    char np[MAX_PATH]; strncpy(np,rel,sizeof(np)-1); np[sizeof(np)-1]=0;
    for(int i=0;np[i];i++) if(np[i]=='/') np[i]='\\';
    snprintf(out,mx,"%s\\%s",ex,np);
}

// Open + loop a single file via MCI
int startMciMusic(const char* path){
    char cmd[1024];
    if(_access(path,0)!=0) return 0;
    mciSendStringA("close dxball_bgm",NULL,0,NULL); Sleep(50);
    snprintf(cmd,sizeof(cmd),"open \"%s\" alias dxball_bgm",path);
    if(mciSendStringA(cmd,NULL,0,NULL)!=0) return 0; Sleep(50);
    if(mciSendStringA("play dxball_bgm repeat",NULL,0,NULL)!=0){
        mciSendStringA("close dxball_bgm",NULL,0,NULL); return 0;
    }
    Sleep(30); applyMusicVolume();
    mciMusicActive=1; lastMusicStopTime=glutGet(GLUT_ELAPSED_TIME);
    return 1;
}
void stopMusic(){
    PlaySoundA(NULL,NULL,0);
    if(mciMusicActive){
        mciSendStringA("stop dxball_bgm",NULL,0,NULL);  Sleep(30);
        mciSendStringA("close dxball_bgm",NULL,0,NULL);
        mciMusicActive=0; lastMusicStopTime=glutGet(GLUT_ELAPSED_TIME);
    }
}
// Try MCI for each path; fall back to PlaySound for .wav files
int startMusicFromPaths(const char** paths, int count){
    for(int i=0;i<count;i++){
        char fp[MAX_PATH]; buildMusicPath(paths[i],fp,sizeof(fp));
        if(startMciMusic(fp)) return 1;
        if(isWavPath(fp) && PlaySoundA(fp,NULL,SND_ASYNC|SND_LOOP|SND_FILENAME|SND_NODEFAULT)){
            mciMusicActive=0; return 1;
        }
    }
    return 0;
}
#define MP_COUNT(a) ((int)(sizeof(a)/sizeof(a[0])))
void startMusicTrack(int track){
    if(track==currentMusicTrack) return;
    stopMusic(); Sleep(80); int ok=0;
    if(track==MUSIC_MENU)        ok=startMusicFromPaths(menuMusicPaths,   MP_COUNT(menuMusicPaths));
    else if(track==MUSIC_GAME)   ok=startMusicFromPaths(gameMusicPaths,   MP_COUNT(gameMusicPaths));
    else if(track==MUSIC_RESULT) ok=startMusicFromPaths(resultMusicPaths, MP_COUNT(resultMusicPaths));
    else if(track==MUSIC_SCORE)  ok=startMusicFromPaths(scoreMusicPaths,  MP_COUNT(scoreMusicPaths));
    currentMusicTrack = ok ? track : MUSIC_NONE;
}

// Pick track based on which page is currently active
void syncMusicForPage(){
    if(currentPage==PAGE_PLAY)                            startMusicTrack(MUSIC_GAME);
    else if(currentPage==PAGE_WIN||currentPage==PAGE_OVER) startMusicTrack(MUSIC_RESULT);
    else if(currentPage==PAGE_SCORE)                       startMusicTrack(MUSIC_SCORE);
    else                                                   startMusicTrack(MUSIC_MENU);
}

#else
void applyMusicVolume(){}
#endif

// Cheap SFX via system beeps (rate-limited)
void playSfx(int kind, int minGapMs=45){
#ifdef _WIN32
    static int lastSfxTick=0; int now=glutGet(GLUT_ELAPSED_TIME);
    if(now-lastSfxTick<minGapMs) return; lastSfxTick=now;
    UINT bt=MB_OK;
    if(kind==1)      bt=MB_ICONASTERISK;
    else if(kind==2) bt=MB_ICONHAND;
    else if(kind==3) bt=MB_ICONEXCLAMATION;
    MessageBeep(bt);
#else
    (void)kind; (void)minGapMs;
#endif
}

// One-shot file SFX via MCI (tries MPEG decoder first)
void playSfxFile(const char* filePath){
#ifdef _WIN32
    static int lastSfxTime=0; int now=glutGet(GLUT_ELAPSED_TIME);
    if(now-lastSfxTime<50) return; lastSfxTime=now;
    char cmd[1024];
    mciSendStringA("close dxball_sfx",NULL,0,NULL); Sleep(30);
    snprintf(cmd,sizeof(cmd),"open \"%s\" type mpegvideo alias dxball_sfx",filePath);
    if(mciSendStringA(cmd,NULL,0,NULL)!=0){
        snprintf(cmd,sizeof(cmd),"open \"%s\" alias dxball_sfx",filePath);
        if(mciSendStringA(cmd,NULL,0,NULL)!=0) return;
    }
    Sleep(20); mciSendStringA("play dxball_sfx",NULL,0,NULL);
#else
    (void)filePath;
#endif
}

void quitGame(){
#ifdef _WIN32
    stopMusic();
#endif
    exit(0);
}

// === 6. Score I/O + helpers ===

// Format seconds as "M:SS"
const char* timeText(float sec){
    static char b[64]; int total=(int)sec, m=total/60, s=total%60;
    sprintf(b,"%d:%02d",m,s); return b;
}
const char* difficultyText(int d){
    static const char* t[]={"Easy","Normal","Hard","Hardest"};
    return d>=0&&d<4 ? t[d] : "Unknown";
}
const char* mapCategoryText(int c){
    static const char* t[]={"Classic","Checkerboard","Circle","Pyramid","Special"};
    return c>=0&&c<5 ? t[c] : "Unknown";
}

// Bubble-sort: higher score first; faster time breaks ties
void sortScores(){
    for(int i=0;i<highCount;i++) for(int j=i+1;j<highCount;j++){
        int swap=0;
        if(highscores[j].score>highscores[i].score) swap=1;
        else if(highscores[j].score==highscores[i].score
             && highscores[j].timeSec<highscores[i].timeSec) swap=1;
        if(swap){ ScoreLine t=highscores[i]; highscores[i]=highscores[j]; highscores[j]=t; }
    }
}

void loadScores(){
    highCount=0; FILE* fp=fopen(scorePath,"r"); if(!fp) return;
    char line[256];
    while(highCount<MAX_SCORES && fgets(line,sizeof(line),fp)){
        int s=0; float t=0; int mc=-1, d=-1;
        if(sscanf(line,"%d %f %d %d",&s,&t,&mc,&d) < 2) continue;
        highscores[highCount].score=s;        highscores[highCount].timeSec=t;
        highscores[highCount].mapCategory=mc; highscores[highCount].difficulty=d;
        highCount++;
    }
    fclose(fp); sortScores();
}

void saveScores(){
#ifdef _WIN32
    _mkdir(".dist");
#else
    mkdir(".dist",0755);
#endif
    FILE* fp=fopen(scorePath,"w"); if(!fp) return;
    for(int i=0;i<highCount;i++)
        fprintf(fp,"%d %.2f %d %d\n",highscores[i].score,highscores[i].timeSec,
            highscores[i].mapCategory,highscores[i].difficulty);
    fclose(fp);
}

// Push current run into table (replaces last slot when full)
void submitScore(){
    if(scoreSaved) return; scoreSaved=1;
    int idx = (highCount<MAX_SCORES) ? highCount++ : (MAX_SCORES-1);
    highscores[idx].score=score;                highscores[idx].timeSec=playTimeSec;
    highscores[idx].mapCategory=selectedMapCategory; highscores[idx].difficulty=selectedDifficulty;
    sortScores();
    if(highCount>MAX_SCORES) highCount=MAX_SCORES;
    saveScores();
}

// Best score for one map category (out: difficulty, time)
int getBestScoreForMap(int c, int* d, float* t){
    int s=-1, bd=-1; float bt=0;
    for(int i=0;i<highCount;i++){
        if(highscores[i].mapCategory!=c) continue;
        if(highscores[i].score>s){
            s=highscores[i].score; bd=highscores[i].difficulty; bt=highscores[i].timeSec;
        } else if(highscores[i].score==s && s>=0 && highscores[i].timeSec<bt){
            bd=highscores[i].difficulty; bt=highscores[i].timeSec;
        }
    }
    if(d) *d=bd; if(t) *t=bt;
    return s;
}

// === 7. Bricks (build, types, helpers) ===

// Row colors (top to bottom rainbow gradient)
static const float brickRowColors[6][3] = {
    {0.93f,0.33f,0.27f}, {0.96f,0.52f,0.24f}, {0.94f,0.75f,0.24f},
    {0.31f,0.72f,0.29f}, {0.23f,0.55f,0.93f}, {0.55f,0.38f,0.89f}
};

int countLockedBricksAlive(){
    int c=0;
    for(int i=0;i<brickCount;i++)
        if(bricks[i].alive && bricks[i].type==7 && bricks[i].isLocked) c++;
    return c;
}

// Consume one key, unlock a random locked brick
int unlockRandomLockedBrick(){
    if(hasKey<=0) return 0;
    int idxs[MAX_BRICKS], cnt=0;
    for(int i=0;i<brickCount;i++)
        if(bricks[i].alive && bricks[i].type==7 && bricks[i].isLocked) idxs[cnt++]=i;
    if(cnt<=0) return 0;
    int p=idxs[rand()%cnt]; bricks[p].isLocked=0; hasKey--;
    addFloatText(bricks[p].x+bricks[p].w*0.5f, bricks[p].y+bricks[p].h*0.5f,
                 "UNLOCK", 1,0.82f,0.24f);
    playSfx(1,80);
    return 1;
}

// Place one brick at (row, colIdx); row points scale by row
void mkBrick(int r,int colIdx, float mx,float sy, float bw,float bh,float gap,
             const float cols[6][3], int rows){
    Brick b;
    b.x=mx+colIdx*(bw+gap); b.y=sy-r*(bh+gap); b.w=bw; b.h=bh;
    b.r=cols[r][0]; b.g=cols[r][1]; b.b=cols[r][2];
    b.points=(rows-r)*10; b.alive=1;
    bricks[brickCount++]=b;
}

// Roll random special type + override color (~55% stay normal)
void assignBrickType(Brick& b){
    int r=rand()%100;
    b.regenTimer=0; b.moveDir=1; b.isLocked=0; b.linkedBrick=-1;
    if(r<6)       { b.type=2; b.hp=1;             b.r=0.85f; b.g=0.28f; b.b=0.18f; } // bomb
    else if(r<15) { b.type=1; b.hp=2;             b.r=0.6f;  b.g=0.6f;  b.b=0.65f; } // tough
    else if(r<20) { b.type=3; b.hp=1;             b.r=0.7f;  b.g=0.95f; b.b=0.4f;  } // moving
    else if(r<24) { b.type=4; b.hp=1;             b.r=0.3f;  b.g=0.3f;  b.b=0.4f;  } // ghost
    else if(r<28) { b.type=5; b.hp=1;             b.r=0.3f;  b.g=0.8f;  b.b=0.95f; } // ice
    else if(r<32) { b.type=6; b.hp=1;             b.r=1;     b.g=0.4f;  b.b=0.1f;  } // fire
    else if(r<36) { b.type=7; b.hp=1; b.isLocked=1; b.r=0.5f; b.g=0.5f; b.b=0.8f;  } // locked
    else if(r<40) { b.type=8; b.hp=1; b.regenTimer=0; b.r=0.95f; b.g=0.6f; b.b=0.2f;} // regen
    else if(r<44) { b.type=9; b.hp=1;             b.r=0.75f; b.g=0.35f; b.b=0.95f; } // teleport
    else          { b.type=0; b.hp=1; }                                               // normal
}

// Build brick field for chosen category + variant
void setupBricks(){
    brickCount=0;
    int rows=6, cols=10;
    float gap=8, bh=28, margin=56, startY=winH-72;
    float bw=(winW - 2*margin - (cols-1)*gap) / cols;
    int mv=selectedMapVariant%3;

    if(selectedMapCategory==MAP_CAT_CLASSIC){
        if(mv<=1){ // full grid
            for(int r=0;r<rows;r++) for(int c=0;c<cols;c++)
                mkBrick(r,c,margin,startY,bw,bh,gap,brickRowColors,rows);
        } else { // punch holes
            for(int r=0;r<rows;r++) for(int c=0;c<cols;c++)
                if(r%2==0 || c%3!=0)
                    mkBrick(r,c,margin,startY,bw,bh,gap,brickRowColors,rows);
        }
    }
    else if(selectedMapCategory==MAP_CAT_CHECKERBOARD){
        if(mv<=1){
            for(int r=0;r<rows;r++) for(int c=0;c<cols;c++)
                if((r+c)%2==0)
                    mkBrick(r,c,margin,startY,bw,bh,gap,brickRowColors,rows);
        } else {
            for(int r=0;r<rows;r++) for(int c=0;c<cols;c++)
                if((r+c)%3==0)
                    mkBrick(r,c,margin,startY,bw,bh,gap,brickRowColors,rows);
        }
    }
    else if(selectedMapCategory==MAP_CAT_CIRCLE){
        int th = (mv<=1) ? 9 : 7;
        for(int r=0;r<rows;r++) for(int c=0;c<cols;c++){
            int dist=(c-5)*(c-5)+(r-2)*(r-2);
            if(dist<=th) mkBrick(r,c,margin,startY,bw,bh,gap,brickRowColors,rows);
        }
    }
    else if(selectedMapCategory==MAP_CAT_PYRAMID){
        for(int r=0;r<rows;r++){
            int birr=cols-r, sc=r/2;
            for(int c=0;c<birr;c++) if(sc+c<cols){
                if(mv==2){ if(c%2==0) mkBrick(r,sc+c,margin,startY,bw,bh,gap,brickRowColors,rows); }
                else      mkBrick(r,sc+c,margin,startY,bw,bh,gap,brickRowColors,rows);
            }
        }
    }
    else if(selectedMapCategory==MAP_CAT_SPECIAL){
        if(mv<=1){ // carve corners + center hole
            for(int r=0;r<rows;r++) for(int c=0;c<cols;c++){
                int sk=0;
                if(r<2 && (c<2||c>=8))   sk=1;
                if(r>=4 && c>=3 && c<=6) sk=1;
                if(!sk) mkBrick(r,c,margin,startY,bw,bh,gap,brickRowColors,rows);
            }
        } else { // diagonal sparse
            for(int r=0;r<rows;r++) for(int c=0;c<cols;c++)
                if((c+r)%2==1 && (r+c*2)%3==0)
                    mkBrick(r,c,margin,startY,bw,bh,gap,brickRowColors,rows);
        }
    }

    for(int i=0;i<brickCount;i++) assignBrickType(bricks[i]);
}

// Clear N topmost rows (used by ROW_CLEAR drop)
void clearTopRows(int r){
    while(r>0){
        float ty=-100000;
        for(int i=0;i<brickCount;i++)
            if(bricks[i].alive && bricks[i].y>ty) ty=bricks[i].y;
        if(ty<-99999) return;
        for(int i=0;i<brickCount;i++)
            if(bricks[i].alive && fabsf(bricks[i].y-ty)<0.5f){
                bricks[i].alive=0; score+=bricks[i].points;
            }
        r--;
    }
}

// Kill all bricks within radius 100 (bomb brick chain)
void bombExplosion(float bx,float by){
    float r=100;
    for(int i=0;i<brickCount;i++){
        if(!bricks[i].alive) continue;
        float dx=(bricks[i].x+bricks[i].w*0.5f)-bx, dy=(bricks[i].y+bricks[i].h*0.5f)-by;
        if(sqrtf(dx*dx+dy*dy)<r){ bricks[i].alive=0; score+=bricks[i].points*2; }
    }
}

int allBricksBroken(){
    for(int i=0;i<brickCount;i++) if(bricks[i].alive) return 0;
    return 1;
}

// === 8. Particles + float text ===

void addFloatText(float x,float y, const char* txt, float r,float g,float bcol){
    for(int i=0;i<MAX_FTEXTS;i++) if(ftexts[i].life<=0){
        ftexts[i].x=x; ftexts[i].y=y; ftexts[i].vy=28; ftexts[i].life=1.2f;
        strncpy(ftexts[i].txt,txt,sizeof(ftexts[i].txt)-1);
        ftexts[i].txt[sizeof(ftexts[i].txt)-1]=0;
        ftexts[i].r=r; ftexts[i].g=g; ftexts[i].b=bcol;
        return;
    }
}
void updateFloatTexts(float dt){
    for(int i=0;i<MAX_FTEXTS;i++) if(ftexts[i].life>0){
        ftexts[i].life-=dt; ftexts[i].y+=ftexts[i].vy*dt;
    }
}
void drawFloatTexts(){
    for(int i=0;i<MAX_FTEXTS;i++) if(ftexts[i].life>0){
        C4(ftexts[i].r,ftexts[i].g,ftexts[i].b, clampf(ftexts[i].life,0,1));
        drawTextCenter(ftexts[i].x,ftexts[i].y,ftexts[i].txt,F12);
    }
}

// Burst N orange particles around (x,y)
void spawnParticles(float x,float y,int c){
    for(int i=0;i<MAX_PARTS && c>0;i++) if(parts[i].life<=0){
        parts[i].life=0.7f+((rand()%100)/200.0f);
        parts[i].x=x+(rand()%40-20); parts[i].y=y+(rand()%40-20);
        parts[i].vx=(rand()%200-100)*0.6f; parts[i].vy=(rand()%200-100)*0.6f;
        parts[i].r=1; parts[i].g=0.6f; parts[i].b=0.2f;
        c--;
    }
}
void updateParticles(float dt){
    for(int i=0;i<MAX_PARTS;i++) if(parts[i].life>0){
        parts[i].life-=dt; parts[i].x+=parts[i].vx*dt; parts[i].y+=parts[i].vy*dt;
    }
}
void drawParticles(){
    for(int i=0;i<MAX_PARTS;i++) if(parts[i].life>0){
        C4(parts[i].r,parts[i].g,parts[i].b, clampf(parts[i].life,0,1));
        drawCircle(parts[i].x,parts[i].y,3);
    }
}

// === 9. Ball physics ===

// Current ball speed = base * ramp * powerup mods
float ballSpeedNow(){
    float s=baseBallSpeed*speedRamp;
    if(speedBoostTimer>0)    s*=speedBoostMul;
    if(iceBallSlowTimer>0)   s*=0.5f;
    if(fireBallSpeedTimer>0) s*=1.8f;
    return s;
}

// Rescale velocity to new target speed without changing direction
void scaleBallVector(){
    if(ballAttached) return;
    float len=sqrtf(ballVX*ballVX+ballVY*ballVY), t=ballSpeedNow();
    if(len<0.001f){ ballVX=t*0.55f; ballVY=t*0.85f; return; }
    ballVX=(ballVX/len)*t; ballVY=(ballVY/len)*t;
}

// Stick ball back to paddle, kick at 72deg
void resetBall(){
    paddleW = (wideTimer>0) ? paddleWideW : paddleNormalW;
    paddleX = winW*0.5f;
    ballAttached=1; ballX=paddleX;
    ballY=paddleY+paddleH*0.5f+ballR+2.0f;
    float a=deg2rad(72.0f), s=ballSpeedNow(), sg=(rand()%2)?1.0f:-1.0f;
    ballVX=cosf(a)*s*sg; ballVY=sinf(a)*s;
    for(int i=0;i<MAX_EXTRA_BALLS;i++) extraBallActive[i]=0;
}

void clearDrops(){ for(int i=0;i<dropCount;i++) drops[i].active=0; }

void movePaddle(float dt){
    if(moveLeft)  paddleX -= paddleSpeed*dt;
    if(moveRight) paddleX += paddleSpeed*dt;
    paddleX=clampf(paddleX, paddleW*0.5f, winW-paddleW*0.5f);
}

// Reflect ball off paddle; deflect angle by hit position
void paddleBounce(){
    float rx=paddleX-paddleW*0.5f, ry=paddleY-paddleH*0.5f;
    if(!circleRectHit(ballX,ballY,ballR,rx,ry,paddleW,paddleH)) return;
    if(ballVY>=0) return;
    ballY=ry+paddleH+ballR+0.5f;
    float rel=clampf((ballX-paddleX)/(paddleW*0.5f),-1,1);
    float a=deg2rad(90-rel*65), s=ballSpeedNow();
    ballVX=cosf(a)*s; ballVY=sinf(a)*s;
    playSfxFile("Game Music/14. Padexplo.mp3"); playSfx(0,35);
}

// === 10. Drops (apply, spawn, update) ===

// Inject specific drop type at (x,y) regardless of RNG
void spawnForcedDrop(float x,float y,int type){
    if(dropCount>=MAX_DROPS) return;
    Drop d; d.x=x; d.y=y; d.size=18; d.vy=150; d.type=type; d.active=1;
    drops[dropCount++]=d;
}

// Resolve a caught drop based on type
void applyDrop(int type){
    if(type==DROP_LIFE){ lives++; playSfx(1,70); }
    else if(type==DROP_SPEED){
        speedBoostTimer=speedBoostDur; scaleBallVector();
        playSfxFile("Game Music/11. Humm (power or item gain).mp3"); playSfx(1,70);
    }
    else if(type==DROP_WIDE){
        wideTimer=wideDur; paddleW=paddleWideW;
        playSfxFile("Game Music/11. Humm (power or item gain).mp3"); playSfx(1,70);
    }
    else if(type==DROP_BOMB){ // shield eats it, else lose life
        playSfxFile("Game Music/14. Padexplo.mp3");
        if(shieldTimer>0){ shieldTimer=0; playSfx(3,70); }
        else { lives--; resetBall(); playSfx(2,70); }
    }
    else if(type==DROP_SHIELD){
        shieldTimer=shieldDur;
        playSfxFile("Game Music/11. Humm (power or item gain).mp3"); playSfx(1,70);
    }
    else if(type==DROP_FIREBALL){
        fireballTimer=fireballDur;
        addFloatText(ballX,ballY,"FIREBALL",1,0.6f,0.1f);
        playSfxFile("Game Music/11. Humm (power or item gain).mp3"); playSfx(1,70);
    }
    else if(type==DROP_MULTIBALL){ // spawn 2-5 extra balls with x1.5 score
        int spawnCount = 2 + (rand()%4);
        extraBallMultiplierActive=1;
        static const float cols[5][3] = {
            {1,0.5f,0.2f}, {0.3f,0.8f,1}, {0.9f,0.3f,0.7f}, {0.4f,1,0.3f}, {1,0.9f,0.2f}
        };
        for(int i=0;i<MAX_EXTRA_BALLS && i<spawnCount;i++) if(!extraBallActive[i]){
            extraBallActive[i]=1;
            extraBallX[i] = ballX + (i+1)*12.0f * ((i%2)?-1:1);
            extraBallY[i] = ballY + 8;
            float ang=deg2rad(30+i*25), s=ballSpeedNow();
            extraBallVX[i] = cosf(ang)*s*((i%2)?-1.0f:1.0f);
            extraBallVY[i] = sinf(ang)*s;
            extraBallR_val[i]=cols[i][0]; extraBallG_val[i]=cols[i][1]; extraBallB_val[i]=cols[i][2];
        }
        char mb[64]; sprintf(mb,"MULTI x%d",spawnCount);
        addFloatText(ballX,ballY,mb,0.9f,0.6f,0.95f);
        playSfxFile("Game Music/11. Humm (power or item gain).mp3"); playSfx(1,70);
    }
    else if(type==DROP_ROW_CLEAR){
        clearTopRows((rand()%2)+1);
        playSfxFile("Game Music/11. Humm (power or item gain).mp3"); playSfx(1,70);
    }
    else if(type==DROP_KEY){
        if(hasKey<9) hasKey++;
        char kbuf[64]; sprintf(kbuf,"KEY +1 (%d)",hasKey);
        addFloatText(ballX,ballY,kbuf,1,0.85f,0.2f); playSfx(1,70);
    }
}

// Roll a drop on brick destroy (~28% chance; key chance scales with locked bricks)
void spawnDrop(float x,float y){
    if(dropCount>=MAX_DROPS) return;
    if((rand()%100)>72) return;
    Drop d; d.x=x; d.y=y; d.size=18; d.vy=150;

    // Boost key drop chance when locked bricks > keys held
    int k = countLockedBricksAlive() - hasKey, kb=0;
    if(k>=3) kb=45; else if(k==2) kb=30; else if(k==1) kb=18;
    if((rand()%100)<kb){
        d.type=DROP_KEY; d.active=1; drops[dropCount++]=d; return;
    }

    // Cumulative weights: life 16, speed 14, wide 13, rowclear 14, shield 11,
    //                     bomb 8, fireball 10, multi 11, key 3
    static const int t[]={ DROP_LIFE,DROP_SPEED,DROP_WIDE,DROP_ROW_CLEAR,DROP_SHIELD,
                           DROP_BOMB,DROP_FIREBALL,DROP_MULTIBALL,DROP_KEY };
    static const int p[]={ 16,30,43,57,68,76,86,97,100 };
    int r=rand()%100; d.type=DROP_KEY;
    for(int i=0;i<9;i++) if(r<p[i]){ d.type=t[i]; break; }
    d.active=1; drops[dropCount++]=d;
}

void updateDrops(float dt){
    for(int i=0;i<dropCount;i++){
        if(!drops[i].active) continue;
        drops[i].y -= drops[i].vy*dt;
        float rx=paddleX-paddleW*0.5f, ry=paddleY-paddleH*0.5f;
        if(circleRectHit(drops[i].x,drops[i].y,drops[i].size*0.5f, rx,ry,paddleW,paddleH)){
            drops[i].active=0; applyDrop(drops[i].type); continue;
        }
        if(drops[i].y+drops[i].size<0) drops[i].active=0;
    }
}

// === 11. Hit detection ===

// Generic brick hit handler used by main ball + every extra ball.
// Mutates pos/velocity via pointers so it works for multiball too.
int hitBrickForBall(float* bxp,float* byp, float* bvx,float* bvy,
                    Brick* b, float usedBallR){
    if(!b->alive) return 0;
    if(!circleRectHit(*bxp,*byp,usedBallR, b->x,b->y,b->w,b->h)) return 0;

    float nx=clampf(*bxp,b->x,b->x+b->w), ny=clampf(*byp,b->y,b->y+b->h);
    float dx=*bxp-nx, dy=*byp-ny;

    if(b->type==4){ b->r=0.8f; b->g=0.95f; b->b=0.3f; } // ghost lights up

    // Fireball ignores bounce, plows through
    int fireActive = (fireballTimer>0) ? 1 : 0;
    if(!fireActive){ if(fabs(dx)>fabs(dy)) *bvx=-*bvx; else *bvy=-*bvy; }

    // Locked brick rejects ball when no key — toss key drop assist
    if(b->type==7 && b->isLocked && !hasKey){
        addFloatText(b->x+b->w*0.5f,b->y+b->h*0.5f,"LOCKED",0.7f,0.7f,0.95f);
        if(keyAssistCooldown<=0 && (rand()%100)<65){
            spawnForcedDrop(b->x+b->w*0.5f,b->y+b->h*0.5f,DROP_KEY);
            keyAssistCooldown=1.2f;
        }
        playSfx(2,120);
        return 1;
    }

    // Side-effect bricks: ice slows, fire speeds
    if(b->type==5){
        iceBallSlowTimer=iceBallSlowDur; scaleBallVector();
        addFloatText(b->x+b->w*0.5f,b->y+b->h*0.5f,"SLOW",0.3f,0.8f,0.95f);
    }
    if(b->type==6){
        fireBallSpeedTimer=fireBallSpeedDur; scaleBallVector();
        addFloatText(b->x+b->w*0.5f,b->y+b->h*0.5f,"FAST",1,0.4f,0.1f);
    }

    b->hp -= 1;
    if(b->hp<=0){
        // Regen brick re-spawns in 4s; else dies for good
        if(b->type==8){ b->hp=1; b->regenTimer=4.0f; b->alive=0; } else b->alive=0;

        // Bomb cascade + shake
        if(b->type==2){
            bombExplosion(b->x+b->w*0.5f,b->y+b->h*0.5f);
            spawnParticles(b->x+b->w*0.5f,b->y+b->h*0.5f,16);
            shakeTimer=0.5f; shakeIntensity=8;
        }
        // Locked brick that opens because we had a key
        if(b->type==7 && b->isLocked && hasKey>0){
            hasKey--; b->isLocked=0;
            addFloatText(b->x+b->w*0.5f,b->y+b->h*0.5f,"UNLOCKED",1,0.8f,0.2f);
        }

        // Combo bookkeeping + score
        comboCount++; comboTimer=comboDecay;
        int bonusMul=1;
        if(comboCount>=2 && comboCount<5) bonusMul=2;
        if(comboCount>=10){ score+=200; shakeTimer=0.8f; shakeIntensity=12; }
        int finalScore=(int)(b->points * bonusMul *
                             (extraBallMultiplierActive ? extraBallScoreMul : 1.0f));
        score+=finalScore;
        char tbuf[64]; sprintf(tbuf,"+%d",finalScore);
        addFloatText(b->x+b->w*0.5f,b->y+b->h*0.5f,tbuf,1,0.95f,0.3f);

        if(fireActive) playSfxFile("Game Music/26. Xplosht1.mp3");
        spawnDrop(b->x+b->w*0.5f,b->y+b->h*0.5f);
        playSfx(0,40);
    } else { // tough brick partial hit (half points)
        score+=(b->points/2);
        addFloatText(b->x+b->w*0.5f,b->y+b->h*0.5f,"HIT",0.9f,0.9f,0.95f);
        comboCount++; comboTimer=comboDecay; playSfx(0,50);
    }

    // 5-hit combo auto-grants fireball
    if(comboCount>=5 && fireballTimer<=0){
        fireballTimer=fireballDur;
        addFloatText(*bxp,*byp,"FIREBALL!",1,0.5f,0.1f);
    }
    return 1;
}

// === 12. Game update ===

// Tick down all timed powerups; rescale speed when boost ends
void updateTimers(float dt){
    if(keyAssistCooldown>0){ keyAssistCooldown-=dt; if(keyAssistCooldown<0) keyAssistCooldown=0; }
    if(wideTimer>0){
        wideTimer-=dt;
        if(wideTimer<=0){
            wideTimer=0; paddleW=paddleNormalW;
            paddleX=clampf(paddleX,paddleW*0.5f,winW-paddleW*0.5f);
        }
    }
    if(speedBoostTimer>0)   { speedBoostTimer-=dt;    if(speedBoostTimer<=0)   { speedBoostTimer=0;    scaleBallVector(); } }
    if(shieldTimer>0)       { shieldTimer-=dt;        if(shieldTimer<0)        shieldTimer=0; }
    if(fireballTimer>0)     { fireballTimer-=dt;      if(fireballTimer<0)      fireballTimer=0; }
    if(iceBallSlowTimer>0)  { iceBallSlowTimer-=dt;   if(iceBallSlowTimer<=0)  { iceBallSlowTimer=0;   scaleBallVector(); } }
    if(fireBallSpeedTimer>0){ fireBallSpeedTimer-=dt; if(fireBallSpeedTimer<=0){ fireBallSpeedTimer=0; scaleBallVector(); } }
}

// Reset per-run state + start a fresh playthrough
void newGame(){
    lives=3; score=0; playTimeSec=0;
    comboCount=0;
    comboTimer=fireballTimer=iceBallSlowTimer=fireBallSpeedTimer=shakeTimer=0;
    shakeIntensity=0; hasKey=0; keyAssistCooldown=0;
    speedRamp=1; speedRampNext=12;
    speedBoostTimer=wideTimer=shieldTimer=0;
    scoreSaved=0;
    weatherTimer=lightningTimer=0; showLightning=0;

    // Per-difficulty starting speed + life count
    static const float ds[]={280,340,420,500};
    static const int   dl[]={5,3,2,2};
    baseBallSpeed = ds[selectedDifficulty];
    lives         = dl[selectedDifficulty];

    dropCount=0;
    for(int i=0;i<MAX_EXTRA_BALLS;i++) extraBallActive[i]=0;
    for(int i=0;i<MAX_FTEXTS;i++) ftexts[i].life=0;
    for(int i=0;i<MAX_PARTS;i++)  parts[i].life=0;

    setupBricks(); resetBall();
    canResume=1; currentPage=PAGE_PLAY;
}

// --- Per-frame sub-helpers ---

static void updateWeather(float dt){
    if(selectedWeather!=WEATHER_STORMY) return;
    if(weatherTimer>2.0f){
        lightningTimer=lightningDuration; showLightning=1; weatherTimer=0;
    }
    if(lightningTimer>0){
        lightningTimer-=dt; if(lightningTimer<=0) showLightning=0;
    }
}

static void rampSpeedOverTime(){
    while(playTimeSec>=speedRampNext){
        speedRampNext+=12; speedRamp+=speedRampStep;
        if(speedRamp>speedRampMax) speedRamp=speedRampMax;
        scaleBallVector();
    }
}

// Moving brick sweep + regen brick re-spawn
static void updateBrickLogic(float dt){
    for(int i=0;i<brickCount;i++){
        if(bricks[i].type==3 && bricks[i].alive){
            bricks[i].x += bricks[i].moveDir*70.0f*dt;
            if(bricks[i].x<20){ bricks[i].x=20; bricks[i].moveDir=1; }
            if(bricks[i].x+bricks[i].w>winW-20){
                bricks[i].x=winW-20-bricks[i].w; bricks[i].moveDir=-1;
            }
        }
        if(bricks[i].type==8 && !bricks[i].alive && bricks[i].regenTimer>0){
            bricks[i].regenTimer-=dt;
            if(bricks[i].regenTimer<=0){
                bricks[i].alive=1; bricks[i].hp=1; bricks[i].regenTimer=0;
                addFloatText(bricks[i].x+bricks[i].w*0.5f,bricks[i].y+bricks[i].h*0.5f,
                             "REGEN",0.95f,0.7f,0.2f);
            }
        }
    }
}

// Hardest difficulty: bricks descend; paddle contact = loss
static int hardestModeFallAndCheck(float dt){
    if(selectedDifficulty!=DIFFICULTY_HARDEST) return 0;
    float fs=60;
    for(int i=0;i<brickCount;i++)
        if(bricks[i].alive) bricks[i].y-=fs*dt;
    float pT=paddleY+paddleH*0.5f, pB=paddleY-paddleH*0.5f;
    for(int i=0;i<brickCount;i++){
        if(!bricks[i].alive) continue;
        float bB=bricks[i].y, bT=bricks[i].y+bricks[i].h;
        if(bT>=pB && bB<=pT){
            currentPage=PAGE_OVER; canResume=0; submitScore();
            return 1;
        }
    }
    return 0;
}

// Integrate one extra ball + paddle/brick interactions
static void updateExtraBall(int bi, float dt){
    if(!extraBallActive[bi]) return;
    extraBallX[bi] += extraBallVX[bi]*dt;
    extraBallY[bi] += extraBallVY[bi]*dt;

    if(extraBallX[bi]-extraBallR<0){
        extraBallX[bi]=extraBallR; extraBallVX[bi]=fabs(extraBallVX[bi]);
    } else if(extraBallX[bi]+extraBallR>winW){
        extraBallX[bi]=winW-extraBallR; extraBallVX[bi]=-fabs(extraBallVX[bi]);
    }
    if(extraBallY[bi]+extraBallR>winH){
        extraBallY[bi]=winH-extraBallR; extraBallVY[bi]=-fabs(extraBallVY[bi]);
    }

    float rx=paddleX-paddleW*0.5f, ry=paddleY-paddleH*0.5f;
    if(circleRectHit(extraBallX[bi],extraBallY[bi],extraBallR, rx,ry,paddleW,paddleH)){
        if(extraBallVY[bi]<0){
            extraBallY[bi]=ry+paddleH+extraBallR+0.5f;
            float rel=clampf((extraBallX[bi]-paddleX)/(paddleW*0.5f),-1,1);
            float ang=deg2rad(90-rel*65), spd=ballSpeedNow();
            extraBallVX[bi]=cosf(ang)*spd; extraBallVY[bi]=sinf(ang)*spd;
        }
    }
    for(int i=0;i<brickCount;i++){
        if(hitBrickForBall(&extraBallX[bi],&extraBallY[bi],
                           &extraBallVX[bi],&extraBallVY[bi], &bricks[i], extraBallR)){
            extraBallX[bi]+=extraBallVX[bi]*dt*0.2f;
            extraBallY[bi]+=extraBallVY[bi]*dt*0.2f;
            break;
        }
    }
    if(extraBallY[bi]+extraBallR<0) extraBallActive[bi]=0;
}

// Main per-frame simulation step
void updateGame(float dt){
    playTimeSec+=dt; weatherTimer+=dt;
    updateWeather(dt);
    rampSpeedOverTime();
    updateTimers(dt); movePaddle(dt);
    updateBrickLogic(dt);

    // Combo decay
    if(comboTimer>0){
        comboTimer-=dt;
        if(comboTimer<=0){ comboTimer=0; comboCount=0; }
    }
    updateParticles(dt); updateFloatTexts(dt);
    if(shakeTimer>0){
        shakeTimer-=dt;
        if(shakeTimer<=0){ shakeTimer=0; shakeIntensity=0; }
    }

    if(hardestModeFallAndCheck(dt)) return;

    // Ball stuck to paddle until launch
    if(ballAttached){
        ballX=paddleX; ballY=paddleY+paddleH*0.5f+ballR+2.0f;
        updateDrops(dt); return;
    }

    // Integrate + wall bounces (top + sides)
    ballX+=ballVX*dt; ballY+=ballVY*dt;
    if(ballX-ballR<0){ ballX=ballR; ballVX=fabs(ballVX); }
    else if(ballX+ballR>winW){ ballX=winW-ballR; ballVX=-fabs(ballVX); }
    if(ballY+ballR>winH){ ballY=winH-ballR; ballVY=-fabs(ballVY); }
    paddleBounce();

    // Main ball brick collisions (with teleport-brick warp)
    for(int i=0;i<brickCount;i++){
        if(hitBrickForBall(&ballX,&ballY,&ballVX,&ballVY,&bricks[i],ballR)){
            if(bricks[i].type==9){
                int tries=12;
                while(tries-->0){
                    int t=rand()%brickCount;
                    if(bricks[t].alive && t!=i){
                        ballX=bricks[t].x+bricks[t].w*0.5f;
                        ballY=bricks[t].y+bricks[t].h*0.5f;
                        addFloatText(ballX,ballY,"TELEPORT",0.8f,0.5f,1);
                        break;
                    }
                }
            }
            ballX+=ballVX*dt*0.2f; ballY+=ballVY*dt*0.2f;
            break;
        }
    }

    // Extra balls + multiball cleanup
    for(int bi=0;bi<MAX_EXTRA_BALLS;bi++) updateExtraBall(bi,dt);
    int anyActive=0;
    for(int i=0;i<MAX_EXTRA_BALLS;i++) if(extraBallActive[i]){ anyActive=1; break; }
    if(!anyActive && extraBallMultiplierActive) extraBallMultiplierActive=0;

    updateDrops(dt);

    // Main ball fell off — shield rebounds, else lose a life
    if(ballY+ballR<0){
        if(shieldTimer>0){ shieldTimer=0; ballY=100; ballVY=-ballVY; }
        else {
            lives--; clearDrops();
            if(lives<=0){
                currentPage=PAGE_OVER; canResume=0; submitScore(); return;
            }
            resetBall();
        }
    }

    if(allBricksBroken()){
        currentPage=PAGE_WIN; canResume=0; submitScore();
    }
}

// === 13. Background themes ===

// Nature: sky + ground + props vary by weather
static void drawNatureBackground(){
    if(selectedWeather==WEATHER_SUNNY){
        gradHalfSky(0.53f,0.81f,0.92f, 0.34f,0.68f,0.24f);
        C4(1,1,0.8f,0.4f); drawCircle(winW*0.85f,winH*0.8f,60);
        for(int i=0;i<4;i++){
            float tx=100+i*250;
            C3(0.4f,0.25f,0.1f); drawRect(tx-8,80,16,60);
            C3(0.2f,0.6f,0.2f);  drawCircle(tx,140,35);
        }
        C3(0.25f,0.5f,0.15f);
        for(int i=0;i<6;i++) drawRect(i*180,20,160,25);
        C4(1,1,1,0.5f);
        drawCircle(150,600,30); drawCircle(200,590,40); drawCircle(250,600,28);
        drawCircle(700,620,35); drawCircle(780,610,45); drawCircle(850,620,32);
    }
    else if(selectedWeather==WEATHER_RAINY){
        gradHalfSky(0.3f,0.4f,0.5f, 0.15f,0.35f,0.1f);
        for(int i=0;i<4;i++){
            float tx=100+i*250;
            C3(0.3f,0.2f,0.08f);   drawRect(tx-8,80,16,60);
            C3(0.15f,0.45f,0.15f); drawCircle(tx,140,35);
        }
        C4(0.7f,0.8f,0.95f,0.4f); // rain streaks
        for(int i=0;i<40;i++){
            float rx=(float)(rand()%winW), ry=(float)(rand()%winH);
            drawRect(rx,ry,2,8);
        }
    }
    else if(selectedWeather==WEATHER_STORMY){
        gradHalfSky(0.15f,0.15f,0.2f, 0.08f,0.15f,0.05f);
        if(showLightning){ C4(1,1,1,0.8f); drawRect(0,0,(float)winW,(float)winH); }
    }
    else if(selectedWeather==WEATHER_NIGHT){
        gradFull(0.02f,0.02f,0.08f, 0.04f,0.04f,0.12f);
        C4(0.8f,0.9f,1,0.6f); // stars upper half
        for(int i=0;i<30;i++){
            float sx=(float)(rand()%winW), sy=(float)(winH*0.5f+rand()%(int)(winH*0.5f));
            drawCircle(sx,sy,1.5f);
        }
        C3(0.05f,0.05f,0.08f); // tree silhouettes
        for(int i=0;i<4;i++){
            float tx=100+i*250;
            drawRect(tx-8,80,16,60); drawCircle(tx,140,35);
        }
    }
}

// City: skyscrapers with lit windows
static void drawCityBackground(){
    gradFull(0.2f,0.2f,0.25f, 0.08f,0.1f,0.15f);
    static const float bh[]={250,200,280,150,220};
    for(int i=0;i<5;i++){
        float bx=i*200;
        C3(0.25f,0.25f,0.28f); drawRect(bx,0,200,bh[i]);
        C3(0.95f,0.85f,0.3f);
        for(int y=0;y<(int)(bh[i]/30);y++) for(int x=0;x<5;x++){
            float wx=bx+20+x*35, wy=bh[i]-25-y*30;
            drawRect(wx,wy,15,15);
        }
    }
    C3(0.12f,0.12f,0.15f); drawRect(0,0,(float)winW,25); // road
    C3(0.4f,0.4f,0.4f); for(int i=0;i<10;i++) drawRect(i*100,10,50,4);
    if(selectedWeather==WEATHER_STORMY && showLightning){
        C4(1,1,0.8f,0.6f); drawRect(0,0,(float)winW,(float)winH);
    }
}

// Space: planets + asteroid belt + nebula + stars
static void drawSpaceBackground(){
    gradFull(0.01f,0.01f,0.05f, 0.02f,0.01f,0.08f);
    C3(0.8f,0.4f,0.2f); drawCircle(150,550,50);
    C3(0.3f,0.6f,0.9f); drawCircle(800,480,40);
    C3(0.9f,0.7f,0.3f); drawCircle(950,600,35);
    C3(0.5f,0.2f,0.7f); drawCircle(300,150,45);
    C3(0.6f,0.6f,0.6f); // asteroid belt
    for(int i=0;i<15;i++){
        float ax=200+i*50, ay=250+(float)(rand()%200), sz=8+(float)(rand()%12);
        drawCircle(ax,ay,sz);
    }
    C4(1,0.3f,0.8f,0.15f); drawCircle(winW*0.5f,winH*0.5f,300); // nebula glow
    C4(1,1,1,0.5f); // starfield
    for(int i=0;i<50;i++){
        float sx=(float)(rand()%winW), sy=(float)(rand()%winH);
        drawCircle(sx,sy,1);
    }
}

// Neon: retro grid + colored discs
static void drawNeonBackground(){
    gradFull(0.05f,0,0.1f, 0.08f,0,0.15f);
    glLineWidth(2); C4(0,1,1,0.2f);
    for(int i=0;i<8;i++){
        float y=40+i*80;
        glBegin(GL_LINE_STRIP); V2(0,y); V2((float)winW,y); glEnd();
    }
    for(int i=0;i<12;i++){
        float x=80+i*75;
        glBegin(GL_LINE_STRIP); V2(x,0); V2(x,(float)winH); glEnd();
    }
    C4(0,1,1,0.4f); for(int i=0;i<5;i++) drawCircle(150+i*180,350,40);
    C4(1,0,1,0.3f); for(int i=0;i<5;i++) drawCircle(250+i*180,250,35);
}

// Top-level dispatcher + scanline overlay
void drawBackground(){
    if(selectedTheme==THEME_NATURE)     drawNatureBackground();
    else if(selectedTheme==THEME_CITY)  drawCityBackground();
    else if(selectedTheme==THEME_SPACE) drawSpaceBackground();
    else if(selectedTheme==THEME_NEON)  drawNeonBackground();

    // Horizontal scanlines overlay (subtle CRT vibe)
    C4(1,1,1,0.06f);
    for(int i=0;i<14;i++){ float y=40+i*45; drawRect(0,y,(float)winW,1.2f); }
}

// === 14. HUD + bricks/drops/paddle draw ===

void drawHUD(){
    char buff[128];
    C3(0.95f,0.97f,1);
    sprintf(buff,"Time: %s",timeText(playTimeSec)); drawText(24, winH-34,buff);
    sprintf(buff,"Score: %d",score);                drawText(210,winH-34,buff);
    sprintf(buff,"Lives: %d",lives);                drawText(400,winH-34,buff);

    if(wideTimer>0){       sprintf(buff,"Wide Paddle: %ds",(int)ceil(wideTimer));   drawText(winW-520,winH-34,buff); }
    if(speedBoostTimer>0){ sprintf(buff,"Speed Boost: %ds",(int)ceil(speedBoostTimer));drawText(winW-520,winH-58,buff); }
    if(shieldTimer>0){ C3(0.2f,0.6f,0.95f); sprintf(buff,"Shield: %ds",(int)ceil(shieldTimer));
        drawText(winW-260,winH-34,buff); }
    if(selectedDifficulty==DIFFICULTY_HARDEST){ C3(1,0.35f,0.35f);
        drawText(winW-260,winH-82,"HARDEST: BRICKS FALL",F12); }
    if(comboCount>1){ C3(1,0.6f,0.12f);
        char cb[64]; sprintf(cb,"COMBO x%d",comboCount); drawText(winW-260,winH-58,cb); }
    if(extraBallMultiplierActive){ C3(0.9f,0.5f,0.8f);
        char mb[64]; sprintf(mb,"MULTI BALLS: x%.1f",extraBallScoreMul);
        drawText(winW-420,winH-82,mb); }
    if(hasKey){ C3(1,0.85f,0.2f);
        char kl[96]; sprintf(kl,"KEYS: %d (hit locked brick or press K)",hasKey);
        drawText(24,winH-82,kl,F12); }
    if(iceBallSlowTimer>0){ C3(0.3f,0.8f,0.95f);
        sprintf(buff,"Ball Slow: %ds",(int)ceil(iceBallSlowTimer)); drawText(24,winH-58,buff,F12); }
    if(fireBallSpeedTimer>0){ C3(1,0.4f,0.1f);
        sprintf(buff,"Ball Fast: %ds",(int)ceil(fireBallSpeedTimer)); drawText(24,winH-82,buff,F12); }
}

void drawBricks(){
    for(int i=0;i<brickCount;i++){
        if(!bricks[i].alive) continue;
        if(bricks[i].type==4) C4(0.3f,0.3f,0.4f,0.3f);          // ghost is translucent
        else                  C3(bricks[i].r,bricks[i].g,bricks[i].b);
        drawRect(bricks[i].x,bricks[i].y,bricks[i].w,bricks[i].h);

        if(bricks[i].type==3){ // moving: indicator dots
            C4(0.7f,0.95f,0.4f,0.5f);
            drawCircle(bricks[i].x+bricks[i].w*0.25f,bricks[i].y+bricks[i].h*0.5f,4);
            drawCircle(bricks[i].x+bricks[i].w*0.75f,bricks[i].y+bricks[i].h*0.5f,4);
        }
        if(bricks[i].type==7 && bricks[i].isLocked){ // locked: keyhole dot
            C3(1,0.9f,0.3f);
            drawCircle(bricks[i].x+bricks[i].w*0.5f,bricks[i].y+bricks[i].h*0.5f,5);
        }

        C3(0.08f,0.08f,0.1f); glLineWidth(2);
        drawLineLoopRect(bricks[i].x,bricks[i].y,bricks[i].w,bricks[i].h);
    }
}

// Color + letter glyph per drop type (index matches DROP_* enum)
void drawDrops(){
    static const float dc[][3] = {
        {0.95f,0.36f,0.40f}, {0.96f,0.72f,0.18f}, {0.20f,0.78f,0.45f},
        {0.76f,0.95f,0.30f}, {1,    0.48f,0.12f}, {0.9f, 0.5f, 0.9f},
        {0.85f,0.20f,0.20f}, {0.20f,0.60f,0.95f}, {0.9f, 0.5f, 0.9f},
        {1,    0.85f,0.2f }
    };
    static const char* dtxt[]={"","+1","S","W","R","F","M","X","P","K"};
    for(int i=0;i<dropCount;i++){
        if(!drops[i].active) continue;
        C3(dc[drops[i].type][0],dc[drops[i].type][1],dc[drops[i].type][2]);
        drawCircle(drops[i].x,drops[i].y,drops[i].size*0.5f);
        C3(0.08f,0.08f,0.1f);
        drawTextCenter(drops[i].x,drops[i].y-4,dtxt[drops[i].type],F12);
    }
}

void drawPaddleBall(){
    C3(0.92f,0.96f,1);
    drawRect(paddleX-paddleW*0.5f,paddleY-paddleH*0.5f,paddleW,paddleH);
    if(shieldTimer>0){   C4(0.2f,0.6f,0.95f,0.3f); drawCircle(paddleX,paddleY,paddleW*0.6f); }
    if(fireballTimer>0){ C4(1,0.45f,0.1f,0.85f);   drawCircle(ballX,ballY,ballR*1.9f); }
    C3(1,0.84f,0.25f); drawCircle(ballX,ballY,ballR);
    for(int i=0;i<MAX_EXTRA_BALLS;i++){
        if(!extraBallActive[i]) continue;
        C3(extraBallR_val[i],extraBallG_val[i],extraBallB_val[i]);
        drawCircle(extraBallX[i],extraBallY[i],extraBallR);
        C4(extraBallR_val[i],extraBallG_val[i],extraBallB_val[i],0.3f);
        drawCircle(extraBallX[i],extraBallY[i],extraBallR*1.6f);
    }
}

// Composite playfield (with optional screen shake)
void drawGame(){
    if(shakeTimer>0){
        float ox=(rand()%100-50)/50.0f*shakeIntensity;
        float oy=(rand()%100-50)/50.0f*shakeIntensity;
        glPushMatrix(); glTranslatef(ox,oy,0);
    }
    drawBackground(); drawBricks(); drawDrops(); drawPaddleBall();
    drawHUD(); drawParticles(); drawFloatTexts();
    if(shakeTimer>0) glPopMatrix();

    if(ballAttached && currentPage==PAGE_PLAY){
        C3(0.88f,0.9f,1);
        drawTextCenter(winW*0.5f,108,"Press SPACE or Left Click to launch",F18);
    }
}

// === 15. Menu UI helpers ===

// Modern dark gradient + tinted blobs + grid lines
void drawModernBackdrop(){
    glBegin(GL_QUADS);
    C3(0.05f,0.06f,0.10f); V2(0,0); V2((float)winW,0);
    C3(0.10f,0.14f,0.24f); V2((float)winW,(float)winH);
    C3(0.04f,0.08f,0.15f); V2(0,(float)winH);
    glEnd();
    C4(0.20f,0.56f,0.98f,0.13f); drawCircle(winW*0.18f,winH*0.78f,180);
    C4(1,0.84f,0.25f,0.12f);     drawCircle(winW*0.85f,winH*0.82f,150);
    C4(0.35f,0.90f,0.60f,0.10f); drawCircle(winW*0.55f,winH*0.28f,240);
    C4(1,1,1,0.05f); for(int i=0;i<12;i++){ float y=50+i*52; drawRect(0,y,(float)winW,1.2f); }
    C4(1,1,1,0.04f); for(int i=0;i<18;i++){ float x=40+i*54; drawRect(x,0,1,(float)winH); }
}

// Cursor glow + crosshair
void drawCursorSpotlight(){
    if(!mouseVisible) return;
    C4(0.20f,0.56f,0.98f,0.12f); drawCircle(mouseX,mouseY,54);
    C4(0.20f,0.56f,0.98f,0.18f); drawCircle(mouseX,mouseY,30);
    C4(1,1,1,0.70f); glLineWidth(1.5f);
    glBegin(GL_LINES);
    V2(mouseX-12,mouseY); V2(mouseX+12,mouseY);
    V2(mouseX,mouseY-12); V2(mouseX,mouseY+12);
    glEnd();
}

void drawBackButton(){
    float bx=28, by=winH-76, bw=122, bh=34;
    int hov=pointInRect(mouseX,mouseY,bx,by,bw,bh);
    if(hov) C3(0.20f,0.56f,0.98f); else C3(0.10f,0.14f,0.23f);
    drawRect(bx,by,bw,bh);
    C3(0.86f,0.92f,1); glLineWidth(2); drawLineLoopRect(bx,by,bw,bh);
    drawTextCenter(bx+bw*0.5f,by+11,"Back",F18);
}

// Generic card backing (selected/hover/idle states)
void drawCardFrame(float cx,float cy, float cardW,float cardH, int selected,int hovered){
    float x=cx-cardW*0.5f, y=cy-cardH*0.5f;
    if(selected)      C3(0.20f,0.56f,0.98f);
    else if(hovered)  C3(0.95f,0.75f,0.20f);
    else              C3(0.18f,0.22f,0.32f);
    drawRect(x,y,cardW,cardH);
    C4(1,1,1,0.07f); drawRect(x+5,y+cardH-18,cardW-10,12);
    C3(0.04f,0.05f,0.08f); glLineWidth(2); drawLineLoopRect(x,y,cardW,cardH);
}

// Mini theme thumbnails on theme picker
void drawThemePreview(int theme, float cx,float cy, int selected,int hovered){
    float cardW=170, cardH=160, x=cx-cardW*0.5f, y=cy-cardH*0.5f;
    drawCardFrame(cx,cy,cardW,cardH,selected,hovered);

    if(theme==THEME_NATURE){
        C3(0.40f,0.72f,0.95f); drawRect(x+8,y+62,cardW-16,90);
        C3(0.24f,0.62f,0.25f); drawRect(x+8,y+8, cardW-16,58);
        C4(1,0.92f,0.32f,0.85f); drawCircle(x+132,y+125,16);
        C3(0.15f,0.48f,0.18f); drawRect(x+36,y+28,10,26); drawRect(x+54,y+28,10,36);
        C3(0.28f,0.75f,0.30f); drawCircle(x+41,y+58,18); drawCircle(x+60,y+64,16);
    }
    else if(theme==THEME_CITY){
        C3(0.10f,0.12f,0.18f); drawRect(x+8,y+8,cardW-16,144);
        C3(0.18f,0.20f,0.28f);
        drawRect(x+14,y+8,28,72); drawRect(x+46,y+8,24,96);
        drawRect(x+74,y+8,34,82); drawRect(x+112,y+8,40,58);
        C3(0.98f,0.82f,0.24f); drawCircle(x+130,y+122,13);
        C4(0.95f,0.95f,1,0.25f);
        drawRect(x+22,y+18,6,6); drawRect(x+52,y+26,5,5); drawRect(x+83,y+24,5,5);
    }
    else if(theme==THEME_SPACE){
        C3(0.02f,0.02f,0.08f); drawRect(x+8,y+8,cardW-16,144);
        C3(0.92f,0.94f,1);     drawCircle(x+124,y+120,10);
        C3(0.32f,0.62f,0.98f); drawCircle(x+58,y+82,22);
        C3(0.92f,0.62f,0.24f); drawCircle(x+84,y+52,8);
        C4(1,1,1,0.8f);
        for(int i=0;i<8;i++) drawCircle(x+22+i*18,y+122-(i%3)*26,1.4f);
    }
    else {
        C3(0.06f,0.01f,0.10f); drawRect(x+8,y+8,cardW-16,144);
        C3(0.18f,0.76f,0.95f);
        drawRect(x+10,y+24,cardW-20,2);
        drawRect(x+10,y+50,cardW-20,2);
        drawRect(x+10,y+76,cardW-20,2);
        C3(0.96f,0.25f,0.82f); drawCircle(x+126,y+118,16);
        C3(0.96f,0.72f,0.18f);
        drawRect(x+28,y+28,18,56); drawRect(x+56,y+28,18,74); drawRect(x+84,y+28,18,50);
    }
}

// Mini brick layout preview (used on map cards)
void drawMapPreview(int mapNum, float px,float py, float scale, int isSelected){
    float cardW=140, cardH=140, pX=px-cardW*0.5f, pY=py-cardH*0.5f;
    if(isSelected){ C3(0.20f,0.56f,0.98f); glLineWidth(3); }
    else          { C3(0.4f, 0.4f, 0.5f);  glLineWidth(2); }
    drawLineLoopRect(pX,pY,cardW,cardH);
    C4(0.08f,0.1f,0.2f,0.8f); drawRect(pX,pY,cardW,cardH);

    float bpX=pX+8, bpY=pY+cardH-12, bw=12, bh=8, gap=1;
    if(mapNum==1){
        for(int r=0;r<4;r++) for(int c=0;c<10;c++){
            C3(brickRowColors[r][0],brickRowColors[r][1],brickRowColors[r][2]);
            drawRect(bpX+c*(bw+gap),bpY-r*(bh+gap),bw,bh);
        }
    }
    else if(mapNum==2){
        for(int r=0;r<4;r++) for(int c=0;c<10;c++) if((r+c)%2==0){
            C3(brickRowColors[r][0],brickRowColors[r][1],brickRowColors[r][2]);
            drawRect(bpX+c*(bw+gap),bpY-r*(bh+gap),bw,bh);
        }
    }
    else if(mapNum==3){
        for(int r=0;r<4;r++) for(int c=0;c<10;c++){
            int sk=0;
            if(r<1 && (c<1||c>=8))   sk=1;
            if(r>=2 && c>=3 && c<=6) sk=1;
            if(!sk){
                C3(brickRowColors[r][0],brickRowColors[r][1],brickRowColors[r][2]);
                drawRect(bpX+c*(bw+gap),bpY-r*(bh+gap),bw,bh);
            }
        }
    }
    else if(mapNum==4){
        for(int r=0;r<4;r++){
            int birr=10-r, sc=r/2;
            for(int c=0;c<birr;c++) if(sc+c<10){
                C3(brickRowColors[r][0],brickRowColors[r][1],brickRowColors[r][2]);
                drawRect(bpX+(sc+c)*(bw+gap),bpY-r*(bh+gap),bw,bh);
            }
        }
    }
    else if(mapNum==5){
        for(int r=0;r<4;r++) for(int c=0;c<10;c++){
            int dist=(c-5)*(c-5)+(r-1)*(r-1);
            if(dist<=8){
                C3(brickRowColors[r][0],brickRowColors[r][1],brickRowColors[r][2]);
                drawRect(bpX+c*(bw+gap),bpY-r*(bh+gap),bw,bh);
            }
        }
    }
}

// Map card combines preview + best-score footer
void drawMapCategoryCard(int category, float cx,float cy, int selected,int hovered){
    drawCardFrame(cx,cy,170,160,selected,hovered);
    drawMapPreview(category+1,cx,cy+6,1,selected);

    int bd=-1; float bt=0;
    int bs=getBestScoreForMap(category,&bd,&bt);
    C3(0.94f,0.96f,1);
    if(bs>=0){
        char sl[128],dl[128];
        sprintf(sl,"Best: %d",bs); sprintf(dl,"%s",difficultyText(bd));
        drawTextCenter(cx,cy-60,sl,F12);
        drawTextCenter(cx,cy-76,dl,F12);
    } else {
        drawTextCenter(cx,cy-66,"Best: --",F12);
        drawTextCenter(cx,cy-82,"Difficulty: --",F12);
    }
}

// === 16. Menu pages ===

// Item count per page
int menuSize(){
    if(currentPage==PAGE_DIFFICULTY)    return 4;
    if(currentPage==PAGE_MAP_CATEGORY)  return 5;
    if(currentPage==PAGE_WEATHER)       return 4;
    if(currentPage==PAGE_SETTINGS)      return 6;
    return canResume ? 6 : 5;
}

// Label for menu item at index `idx` on current page
const char* menuText(int idx){
    static const char* d[]   = {"Easy","Normal","Hard","Hardest"};
    static const char* m[]   = {"Classic","Checkerboard","Circle","Pyramid","Special"};
    static const char* t[]   = {"Nature","City","Space","Neon"};
    static const char* s[]   = {"Volume: Mute","Volume: 25%","Volume: 50%",
                                "Volume: 75%","Volume: 100%","Back"};
    static const char* mNo[] = {"Start New Game","High Scores","Help","Game Settings","Exit"};
    static const char* mRes[]= {"Start New Game","Resume","High Scores","Help","Game Settings","Exit"};
    if(currentPage==PAGE_DIFFICULTY)   return d[idx];
    if(currentPage==PAGE_MAP_CATEGORY) return m[idx];
    if(currentPage==PAGE_WEATHER)      return t[idx];
    if(currentPage==PAGE_SETTINGS)     return s[idx];
    return canResume ? mRes[idx] : mNo[idx];
}

// Activate menu item at `idx`; drives page flow
void runMenuAction(int idx){
    if(currentPage==PAGE_DIFFICULTY){
        selectedDifficulty=difficultyIndex=idx;
        menuIndex=0; currentPage=PAGE_MAP_CATEGORY; return;
    }
    if(currentPage==PAGE_MAP_CATEGORY){
        selectedMapCategory=mapCategoryIndex=idx;
        menuIndex=0; currentPage=PAGE_WEATHER; return;
    }
    if(currentPage==PAGE_WEATHER){ // theme also picks matching weather mood
        static const int t[]={THEME_NATURE,THEME_CITY,THEME_SPACE,THEME_NEON};
        static const int w[]={WEATHER_SUNNY,WEATHER_NIGHT,WEATHER_NIGHT,WEATHER_STORMY};
        selectedTheme=t[idx]; selectedWeather=w[idx];
        themeIndex=weatherIndex=idx;
        selectedMapVariant=rand()%3; selectedMap=selectedMapVariant+1;
        newGame(); return;
    }
    if(currentPage==PAGE_SETTINGS){
        static const int vol[]={0,25,50,75,100};
        if(idx<5){ musicVolumePercent=vol[idx]; applyMusicVolume(); }
        else     { currentPage=PAGE_MENU; menuIndex=0; }
        return;
    }

    // Main menu (different layout when paused run can resume)
    if(canResume){
        if(idx==0){ menuIndex=0; currentPage=PAGE_DIFFICULTY; }
        else if(idx==1) currentPage=PAGE_PLAY;
        else if(idx==2){ loadScores(); currentPage=PAGE_SCORE; }
        else if(idx==3) currentPage=PAGE_HELP;
        else if(idx==4){ currentPage=PAGE_SETTINGS; menuIndex=0; }
        else quitGame();
        return;
    }
    if(idx==0){ menuIndex=0; currentPage=PAGE_DIFFICULTY; }
    else if(idx==1){ loadScores(); currentPage=PAGE_SCORE; }
    else if(idx==2) currentPage=PAGE_HELP;
    else if(idx==3){ currentPage=PAGE_SETTINGS; menuIndex=0; }
    else quitGame();
}

// Stack of rectangular menu buttons (top-down)
void drawMenuButtonList(int count){
    float by=winH*0.58f, bx=winW*0.5f-170;
    for(int i=0;i<count;i++){
        int hov=pointInRect(mouseX,mouseY,bx,by,340,42);
        if(i==menuIndex||hov) C3(0.20f,0.56f,0.98f); else C3(0.10f,0.14f,0.22f);
        drawRect(bx,by,340,42);
        C3(0.94f,0.96f,1); glLineWidth(2); drawLineLoopRect(bx,by,340,42);
        drawTextCenter(winW*0.5f,by+13,menuText(i),F18);
        by-=56;
    }
}

// Difficulty page: 4 colored cards
static void drawDifficultyPage(){
    drawModernBackdrop(); drawBackButton();
    C3(0.95f,0.90f,0.28f); drawTextCenter(winW*0.5f,winH-108,"SELECT DIFFICULTY",FTR);

    float cardY=winH*0.54f, startX=winW*0.5f-315;
    static const char* tl[]={"5 lives","Balanced run","Fast and risky","Bricks fall down"};
    for(int i=0;i<4;i++){
        float cardX=startX+i*210;
        int hov=pointInRect(mouseX,mouseY,cardX-85,cardY-80,170,160);
        drawCardFrame(cardX,cardY,170,160,i==menuIndex,hov);
        // Color disc per difficulty
        if(i==0)      C3(0.28f,0.78f,0.42f);
        else if(i==1) C3(0.98f,0.74f,0.24f);
        else if(i==2) C3(0.96f,0.35f,0.36f);
        else          C3(0.78f,0.30f,0.98f);
        drawCircle(cardX,cardY+22,24);
        C3(0.95f,0.97f,1);
        drawTextCenter(cardX,cardY-34,menuText(i),FTR);
        drawTextCenter(cardX,cardY-64,tl[i],F12);
    }
    C3(0.86f,0.9f,0.97f);
    drawTextCenter(winW*0.5f,88,"W/S or Mouse to choose, Enter to continue",F12);
}

// Map category page: 5 cards in 3+2 grid
static void drawMapCategoryPage(){
    drawModernBackdrop(); drawBackButton();
    C3(0.95f,0.90f,0.28f); drawTextCenter(winW*0.5f,winH-108,"SELECT MAP CATEGORY",FTR);
    drawTextCenter(winW*0.5f,winH-138,"Map variants are randomized after you choose a category",F12);

    float xs[5]={winW*0.18f,winW*0.40f,winW*0.62f,winW*0.31f,winW*0.53f};
    float ys[5]={winH*0.61f,winH*0.61f,winH*0.61f,winH*0.34f,winH*0.34f};
    for(int i=0;i<5;i++){
        int hov=pointInRect(mouseX,mouseY,xs[i]-85,ys[i]-80,170,160);
        drawMapCategoryCard(i,xs[i],ys[i],i==menuIndex,hov);
        C3(0.94f,0.96f,1); drawTextCenter(xs[i],ys[i]-98,menuText(i),F18);
    }
    C3(0.86f,0.9f,0.97f);
    drawTextCenter(winW*0.5f,88,"W/S or Mouse to choose, Enter to continue",F12);
}

// Theme/weather page: 4 cards in 2x2 grid
static void drawWeatherPage(){
    drawModernBackdrop(); drawBackButton();
    C3(0.95f,0.90f,0.28f); drawTextCenter(winW*0.5f,winH-108,"SELECT THEME",FTR);
    drawTextCenter(winW*0.5f,winH-138,"Theme cards preview the background style before play starts",F12);

    float xs[4]={winW*0.30f,winW*0.60f,winW*0.30f,winW*0.60f};
    float ys[4]={winH*0.61f,winH*0.61f,winH*0.34f,winH*0.34f};
    for(int i=0;i<4;i++){
        int hov=pointInRect(mouseX,mouseY,xs[i]-85,ys[i]-80,170,160);
        drawThemePreview(i,xs[i],ys[i],i==menuIndex,hov);
        C3(0.94f,0.96f,1); drawTextCenter(xs[i],ys[i]-98,menuText(i),F18);
    }
    C3(0.86f,0.9f,0.97f);
    drawTextCenter(winW*0.5f,88,"W/S or Mouse to choose, Enter to start",F12);
}

// Settings page: volume buttons
static void drawSettingsPage(){
    drawModernBackdrop(); drawBackButton();
    C3(0.95f,0.90f,0.28f); drawTextCenter(winW*0.5f,winH-108,"GAME SETTINGS",FTR);
    char vl[64]; sprintf(vl,"Current Music Volume: %d%%",musicVolumePercent);
    C3(0.86f,0.9f,0.97f); drawTextCenter(winW*0.5f,winH-144,vl,F18);
    drawMenuButtonList(menuSize());
    drawTextCenter(winW*0.5f,88,"W/S or Mouse to choose, Enter to apply",F12);
}

// Main menu (default fallback)
static void drawMainMenu(){
    drawModernBackdrop();
    C3(0.95f,0.90f,0.28f); drawTextCenter(winW*0.5f,winH-112,"DX BALL",FTR);
    drawTextCenter(winW*0.5f,winH-144,
        "Modern arcade battle with themed backgrounds and randomized maps",F12);
    int msize=menuSize();
    if(menuIndex<0)      menuIndex=msize-1;
    if(menuIndex>=msize) menuIndex=0;
    drawMenuButtonList(msize);
    C3(0.86f,0.9f,0.97f);
    drawTextCenter(winW*0.5f,88,"W/S or Mouse to choose, Enter to continue",F12);
}

// Top-level menu dispatcher
void drawMenu(){
    if(currentPage==PAGE_DIFFICULTY)         drawDifficultyPage();
    else if(currentPage==PAGE_MAP_CATEGORY)  drawMapCategoryPage();
    else if(currentPage==PAGE_WEATHER)       drawWeatherPage();
    else if(currentPage==PAGE_SETTINGS)      drawSettingsPage();
    else                                     drawMainMenu();
}

// === 17. Score / Help / Result pages ===

void drawScoresPage(){
    drawBackground();
    C3(0.96f,0.93f,0.33f); drawTextCenter(winW*0.5f,winH-110,"HIGH SCORES BY MAP",FTR);

    if(highCount==0){
        C3(0.85f,0.9f,1);
        drawTextCenter(winW*0.5f,winH*0.5f,"No records yet");
    } else {
        // One row per map category, showing best run
        float startY=winH-190;
        for(int category=0;category<5;category++){
            float rowY=startY-category*72;
            C3(0.10f,0.14f,0.22f); drawRect(winW*0.18f,rowY-22,winW*0.64f,54);
            C3(0.94f,0.96f,1); glLineWidth(2);
            glBegin(GL_LINE_LOOP);
            V2(winW*0.18f,rowY-22); V2(winW*0.82f,rowY-22);
            V2(winW*0.82f,rowY+32); V2(winW*0.18f,rowY+32);
            glEnd();

            int bd=-1; float bt=0;
            int bs=getBestScoreForMap(category,&bd,&bt);
            C3(0.96f,0.93f,0.33f);
            drawText(winW*0.21f,rowY+2,mapCategoryText(category),F18);
            C3(0.85f,0.92f,0.98f);
            char line[128];
            if(bs>=0)
                sprintf(line,"Best: %d   Difficulty: %s   Time: %s",
                    bs,difficultyText(bd),timeText(bt));
            else
                sprintf(line,"Best: --   Difficulty: --   Time: --");
            drawText(winW*0.46f,rowY+2,line,F12);
        }
    }
    C3(0.86f,0.9f,0.97f);
    drawTextCenter(winW*0.5f,88,"Press M or ESC to go back");
}

void drawHelpPage(){
    drawBackground();
    C3(0.96f,0.93f,0.33f); drawTextCenter(winW*0.5f,winH-110,"HELP",FTR);
    C3(0.86f,0.92f,0.99f);
    drawTextCenter(winW*0.5f,winH-190,"Move Paddle: Left/Right or A/D keys, or Mouse movement");
    drawTextCenter(winW*0.5f,winH-232,"Launch Ball: SPACE or Left Click");
    drawTextCenter(winW*0.5f,winH-274,"Pause/Resume: P");
    drawTextCenter(winW*0.5f,winH-316,"Return to Menu: ESC");
    drawTextCenter(winW*0.5f,winH-358,"Exit Any Time: Q");
    drawTextCenter(winW*0.5f,winH-400,"Perks: +1 Life, Speed Up (S), Wide Paddle (W), Row Clear (R)");
    drawTextCenter(winW*0.5f,winH-442,"Bomb is bad drop. Shield can block it.");
    drawTextCenter(winW*0.5f,winH-484,"Collect K drops. Press K to unlock one locked brick.");
    C3(0.86f,0.9f,0.97f);
    drawTextCenter(winW*0.5f,88,"Press M or ESC to go back");
}

// Win/Game-over overlay on top of frozen playfield
void drawResultPage(int won){
    drawGame();
    C4(0,0,0,0.55f); drawRect(0,0,(float)winW,(float)winH);
    if(won){
        C3(0.32f,0.95f,0.55f); drawTextCenter(winW*0.5f,winH*0.58f,"YOU WIN!",FTR);
    } else {
        C3(1,0.35f,0.35f); drawTextCenter(winW*0.5f,winH*0.58f,"GAME OVER",FTR);
    }
    char line[200]; sprintf(line,"Score: %d   Time: %s",score,timeText(playTimeSec));
    C3(0.94f,0.97f,1);
    drawTextCenter(winW*0.5f,winH*0.51f,line);
    drawTextCenter(winW*0.5f,winH*0.45f,"Press N for new game or M for menu");
}

// === 18. Display ===

void display(){
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    // Menu family pages share backdrop + cursor overlay
    if(currentPage==PAGE_MENU       || currentPage==PAGE_DIFFICULTY ||
       currentPage==PAGE_MAP_CATEGORY || currentPage==PAGE_WEATHER  ||
       currentPage==PAGE_SETTINGS){
        drawMenu(); drawCursorSpotlight();
    }
    else if(currentPage==PAGE_PLAY) drawGame();
    else if(currentPage==PAGE_PAUSE){
        drawGame();
        C4(0,0,0,0.55f); drawRect(0,0,(float)winW,(float)winH);
        C3(0.95f,0.95f,1); drawTextCenter(winW*0.5f,winH*0.52f,"PAUSED",FTR);
        drawTextCenter(winW*0.5f,winH*0.46f,"Press P/R to resume, ESC for menu");
    }
    else if(currentPage==PAGE_SCORE) drawScoresPage();
    else if(currentPage==PAGE_HELP)  drawHelpPage();
    else if(currentPage==PAGE_WIN)   drawResultPage(1);
    else if(currentPage==PAGE_OVER)  drawResultPage(0);

    glutSwapBuffers();
}

// === 19. Input (keyboard + mouse) ===

void launchBall(){
    if(currentPage==PAGE_PLAY && ballAttached) ballAttached=0;
}

// --- Per-page keyboard sub-handlers ---

static void keyPlay(unsigned char key){
    if(key=='a'||key=='A')      moveLeft=1;
    else if(key=='d'||key=='D') moveRight=1;
    else if(key=='k'||key=='K'){ // manual key-spend hotkey
        if(unlockRandomLockedBrick()) addFloatText(paddleX,paddleY+36,"KEY USED",1,0.85f,0.25f);
        else if(hasKey<=0)            addFloatText(paddleX,paddleY+36,"NO KEY",0.95f,0.55f,0.35f);
    }
    else if(key==' ')           launchBall();
    else if(key=='p'||key=='P') currentPage=PAGE_PAUSE;
    else if(key==27){ currentPage=PAGE_MENU; canResume=1; menuIndex=0; }
}

static void keyPause(unsigned char key){
    if(key=='p'||key=='P'||key=='r'||key=='R') currentPage=PAGE_PLAY;
    else if(key==27){ currentPage=PAGE_MENU; canResume=1; menuIndex=0; }
}

static void keyMenu(unsigned char key){
    int msize=menuSize();
    if(key=='w'||key=='W'){
        menuIndex--; if(menuIndex<0) menuIndex=msize-1;
    } else if(key=='s'||key=='S'){
        menuIndex++; if(menuIndex>=msize) menuIndex=0;
    } else if(key==13||key==' '){
        runMenuAction(menuIndex);
    } else if(key==27){
        // ESC backs out of sub-pages; quits from main menu
        if(currentPage==PAGE_DIFFICULTY    || currentPage==PAGE_MAP_CATEGORY ||
           currentPage==PAGE_WEATHER       || currentPage==PAGE_SETTINGS){
            currentPage=PAGE_MENU; menuIndex=0;
        } else {
            quitGame();
        }
    }
}

void keyboardDown(unsigned char key, int x, int y){
    (void)x; (void)y;
    if(key=='q'||key=='Q') quitGame();

    if(currentPage==PAGE_PLAY){ keyPlay(key); return; }
    if(currentPage==PAGE_PAUSE){ keyPause(key); return; }
    if(currentPage==PAGE_MENU       || currentPage==PAGE_DIFFICULTY ||
       currentPage==PAGE_MAP_CATEGORY || currentPage==PAGE_WEATHER  ||
       currentPage==PAGE_SETTINGS){
        keyMenu(key); return;
    }
    if(currentPage==PAGE_SCORE || currentPage==PAGE_HELP){
        if(key=='m'||key=='M'||key==27){ currentPage=PAGE_MENU; menuIndex=0; }
        return;
    }
    if(currentPage==PAGE_WIN || currentPage==PAGE_OVER){
        if(key=='n'||key=='N'){ menuIndex=0; currentPage=PAGE_DIFFICULTY; }
        else if(key=='m'||key=='M'||key==27){ currentPage=PAGE_MENU; menuIndex=0; }
    }
}

void keyboardUp(unsigned char key, int x, int y){
    (void)x; (void)y;
    if(key=='a'||key=='A')      moveLeft=0;
    else if(key=='d'||key=='D') moveRight=0;
}

void specialDown(int key, int x, int y){
    (void)x; (void)y;
    if(currentPage==PAGE_PLAY){
        if(key==GLUT_KEY_LEFT)       moveLeft=1;
        else if(key==GLUT_KEY_RIGHT) moveRight=1;
    }
    else if(currentPage==PAGE_MENU       || currentPage==PAGE_DIFFICULTY ||
            currentPage==PAGE_MAP_CATEGORY || currentPage==PAGE_WEATHER  ||
            currentPage==PAGE_SETTINGS){
        int msize=menuSize();
        if(key==GLUT_KEY_UP){
            menuIndex--; if(menuIndex<0) menuIndex=msize-1;
        } else if(key==GLUT_KEY_DOWN){
            menuIndex++; if(menuIndex>=msize) menuIndex=0;
        }
    }
}

void specialUp(int key, int x, int y){
    (void)x; (void)y;
    if(key==GLUT_KEY_LEFT)       moveLeft=0;
    else if(key==GLUT_KEY_RIGHT) moveRight=0;
}

// Mouse drives paddle during gameplay
void mouseMove(int x, int y){
    mouseX=(float)x; mouseY=(float)(winH-y); mouseVisible=1;
    if(currentPage==PAGE_PLAY||currentPage==PAGE_PAUSE)
        paddleX=clampf((float)x, paddleW*0.5f, winW-paddleW*0.5f);
}

// Vertical button list hit test
int hitMenuButton(float wx, float wy, int msize){
    float by=winH*0.58f, bx=winW*0.5f-170;
    for(int i=0;i<msize;i++){
        if(pointInRect(wx,wy,bx,by,340,42)){
            menuIndex=i; runMenuAction(i); return 1;
        }
        by-=56;
    }
    return 0;
}

void mouseClick(int button, int state, int x, int y){
    if(button!=GLUT_LEFT_BUTTON||state!=GLUT_DOWN) return;
    float wx=(float)x, wy=(float)(winH-y);

    // Back button on sub-pages
    if(currentPage==PAGE_DIFFICULTY    || currentPage==PAGE_MAP_CATEGORY ||
       currentPage==PAGE_WEATHER       || currentPage==PAGE_SETTINGS){
        if(pointInRect(wx,wy,28,winH-76,122,34)){
            currentPage=PAGE_MENU; menuIndex=0; return;
        }
    }
    if(currentPage==PAGE_PLAY){ launchBall(); return; }

    if(currentPage==PAGE_MENU       || currentPage==PAGE_DIFFICULTY ||
       currentPage==PAGE_MAP_CATEGORY || currentPage==PAGE_WEATHER  ||
       currentPage==PAGE_SETTINGS){
        int msize=menuSize();
        if(currentPage==PAGE_DIFFICULTY){
            float cardY=winH*0.54f, startX=winW*0.5f-210;
            for(int i=0;i<msize;i++){
                float cardX=startX+i*210;
                if(pointInRect(wx,wy,cardX-85,cardY-80,170,160)){
                    menuIndex=i; runMenuAction(i); return;
                }
            }
        }
        else if(currentPage==PAGE_MAP_CATEGORY){
            float xs[5]={winW*0.18f,winW*0.40f,winW*0.62f,winW*0.31f,winW*0.53f};
            float ys[5]={winH*0.61f,winH*0.61f,winH*0.61f,winH*0.34f,winH*0.34f};
            for(int i=0;i<msize;i++)
                if(pointInRect(wx,wy,xs[i]-85,ys[i]-80,170,160)){
                    menuIndex=i; runMenuAction(i); return;
                }
        }
        else if(currentPage==PAGE_WEATHER){
            float xs[4]={winW*0.30f,winW*0.60f,winW*0.30f,winW*0.60f};
            float ys[4]={winH*0.61f,winH*0.61f,winH*0.34f,winH*0.34f};
            for(int i=0;i<msize;i++)
                if(pointInRect(wx,wy,xs[i]-85,ys[i]-80,170,160)){
                    menuIndex=i; runMenuAction(i); return;
                }
        }
        else if(currentPage==PAGE_MENU||currentPage==PAGE_SETTINGS){
            hitMenuButton(wx,wy,msize);
        }
    }
}

// === 20. Reshape + timer + init ===

void reshape(int w, int h){
    if(w<800) w=800; if(h<600) h=600;
    winW=w; winH=h;
    glViewport(0,0,winW,winH);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluOrtho2D(0,(double)winW,0,(double)winH);
    glMatrixMode(GL_MODELVIEW);  glLoadIdentity();
    paddleY=48;
    paddleX=clampf(paddleX,paddleW*0.5f,winW-paddleW*0.5f);
}

// Fixed ~60 FPS tick
void update(int value){
    (void)value;
    int now=glutGet(GLUT_ELAPSED_TIME);
    float dt=(now-prevTick)/1000.0f; prevTick=now; dt=clampf(dt,0,0.05f);
#ifdef _WIN32
    syncMusicForPage();
#endif
    if(currentPage==PAGE_PLAY) updateGame(dt);
    glutPostRedisplay();
    glutTimerFunc(16,update,0);
}

void init(){
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluOrtho2D(0,(double)winW,0,(double)winH);
    glMatrixMode(GL_MODELVIEW);  glLoadIdentity();
}

// === 21. Main ===

int main(int argc, char** argv){
    srand((unsigned int)time(NULL));

    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGBA);
    glutInitWindowSize(winW,winH);
    glutInitWindowPosition(60,30);
    glutCreateWindow("DX Ball - OpenGL C++");

    init();
    loadScores();
    setupBricks();
    resetBall();
#ifdef _WIN32
    syncMusicForPage();
#endif
    prevTick=glutGet(GLUT_ELAPSED_TIME);

    glutDisplayFunc(display);
    glutKeyboardFunc(keyboardDown);
    glutKeyboardUpFunc(keyboardUp);
    glutSpecialFunc(specialDown);
    glutSpecialUpFunc(specialUp);
    glutPassiveMotionFunc(mouseMove);
    glutMotionFunc(mouseMove);
    glutMouseFunc(mouseClick);
    glutReshapeFunc(reshape);
    glutTimerFunc(16,update,0);

    glutMainLoop();
    return 0;
}
