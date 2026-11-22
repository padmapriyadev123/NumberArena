/* achievements.h - Six permanent achievements. */
#ifndef ACHIEVEMENTS_H
#define ACHIEVEMENTS_H
#include "game.h"

enum { ACH_FIRST_BLOOD, ACH_SPEED_MASTER, ACH_PRECISION, ACH_UNSTOPPABLE,
       ACH_PERFECT, ACH_CHAMPION, ACH_COUNT };

typedef struct {
    int playerId;
    unsigned mask;                 /* bit n = achievement n unlocked */
    char dates[ACH_COUNT][11];     /* YYYY-MM-DD of unlock */
} AchRecord;

struct Database;

const char *achievements_name(int a);
const char *achievements_desc(int a);
int  achievements_is_unlocked(const struct Database *db, int playerId, int a);
int  achievements_count(const struct Database *db, int playerId);
int  achievements_unlock(struct Database *db, int playerId, int a);   /* 1 if newly unlocked, prints banner; -1 on memory error */
int  achievements_check_mission(struct Database *db, const Player *p, const MissionConfig *cfg, const MissionResult *res);
void achievements_show(const struct Database *db, const Player *p);
#endif
