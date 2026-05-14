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