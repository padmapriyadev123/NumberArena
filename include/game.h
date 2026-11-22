/* game.h - Core mission engine and the Quick Play / Time Attack / Daily modes. */
#ifndef GAME_H
#define GAME_H
#include "player.h"
#include "scoring.h"

struct Database;

typedef enum { MODE_CHAMPIONSHIP, MODE_QUICK, MODE_TIME, MODE_DAILY } GameMode;

typedef struct {
    GameMode mode;
    const char *modeName;
    const char *diffName;
    const char *title;
    int min, max;
    int maxAttempts;
    int baseReward;     /* base mission points AND base XP reward */
    int timeLimit;      /* seconds, 0 = unlimited */
    int fixedSecret;    /* 0 = random */
} MissionConfig;

typedef struct {
    int won, quit, timedOut;
    int secret, attempts;
    double seconds;
    int lifelineUsed, extraHints;
    int score, xpGained;
    ScoreBreakdown sb;
} MissionResult;

void game_run_mission(struct Database *db, Player *p, const MissionConfig *cfg, MissionResult *res);
void game_quick_play(struct Database *db, Player *p);
void game_time_attack(struct Database *db, Player *p);
void game_daily_challenge(struct Database *db, Player *p);
int  game_today_key(void);   /* YYYYMMDD */
#endif
