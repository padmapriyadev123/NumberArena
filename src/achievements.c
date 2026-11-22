/* achievements.c - Achievement definitions, unlock logic and display. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "achievements.h"
#include "storage.h"
#include "ui.h"

static const char *NAMES[ACH_COUNT] = {
    "First Blood", "Speed Master", "Precision Expert", "Unstoppable", "Perfect Detective", "Arena Champion"
};
static const char *DESCS[ACH_COUNT] = {
    "Win your first mission.",
    "Win any mission in 20 seconds or less.",
    "Win a mission (range 1-100 or bigger) using at most half of your attempts.",
    "Reach a winning streak of 5.",
    "Win a mission with range 500+ using no lifeline and no extra hints.",
    "Complete all three Championship rounds."
};

const char *achievements_name(int a) { return (a >= 0 && a < ACH_COUNT) ? NAMES[a] : "?"; }
const char *achievements_desc(int a) { return (a >= 0 && a < ACH_COUNT) ? DESCS[a] : "?"; }

static AchRecord *find_rec(const Database *db, int playerId)
{
    size_t i;
    for (i = 0; i < db->nAch; i++)
        if (db->ach[i].playerId == playerId) return &db->ach[i];
    return NULL;
}

static AchRecord *get_or_create(Database *db, int playerId)
{
    AchRecord *r = find_rec(db, playerId);
    if (r) return r;
    if (db->nAch == db->capAch) {
        size_t ncap = db->capAch ? db->capAch * 2 : 8;
        AchRecord *n = realloc(db->ach, ncap * sizeof *n);
        if (!n) return NULL;
        db->ach = n;
        db->capAch = ncap;
    }
    r = &db->ach[db->nAch++];
    memset(r, 0, sizeof *r);
    r->playerId = playerId;
    return r;
}

int achievements_is_unlocked(const Database *db, int playerId, int a)
{
    const AchRecord *r = find_rec(db, playerId);
    return r && a >= 0 && a < ACH_COUNT && (r->mask & (1u << a));
}

int achievements_count(const Database *db, int playerId)
{
    int a, n = 0;
    for (a = 0; a < ACH_COUNT; a++) if (achievements_is_unlocked(db, playerId, a)) n++;
    return n;
}

int achievements_unlock(Database *db, int playerId, int a)
{
    AchRecord *r;
    time_t t = time(NULL);
    struct tm *tmv = localtime(&t);
    if (a < 0 || a >= ACH_COUNT) return 0;
    if (achievements_is_unlocked(db, playerId, a)) return 0;
    r = get_or_create(db, playerId);
    if (!r) return -1;
    r->mask |= (1u << a);
    if (tmv) strftime(r->dates[a], sizeof r->dates[a], "%Y-%m-%d", tmv);
    printf("\n%s+==================================================================+\n", ui_c(CLR_PURPLE));
    printf("|  *** ACHIEVEMENT UNLOCKED: %-34s ***|\n", NAMES[a]);
    printf("|  %-64s|\n", DESCS[a]);
    printf("+==================================================================+%s\n", ui_c(CLR_RESET));
    return 1;
}

int achievements_check_mission(Database *db, const Player *p, const MissionConfig *cfg, const MissionResult *res)
{
    int n = 0, range = cfg->max - cfg->min + 1;
    if (!res->won) return 0;
    if (achievements_unlock(db, p->id, ACH_FIRST_BLOOD) == 1) n++;
    if (res->seconds <= 20.0 && achievements_unlock(db, p->id, ACH_SPEED_MASTER) == 1) n++;
    if (range >= 100 && res->attempts * 2 <= cfg->maxAttempts &&
        achievements_unlock(db, p->id, ACH_PRECISION) == 1) n++;
    if (p->streak >= 5 && achievements_unlock(db, p->id, ACH_UNSTOPPABLE) == 1) n++;
    if (range >= 500 && !res->lifelineUsed && res->extraHints == 0 &&
        achievements_unlock(db, p->id, ACH_PERFECT) == 1) n++;
    return n;
}

void achievements_show(const Database *db, const Player *p)
{
    int a;
    const AchRecord *r = find_rec(db, p->id);
    ui_title("ACHIEVEMENTS");
    for (a = 0; a < ACH_COUNT; a++) {
        int on = achievements_is_unlocked(db, p->id, a);
        printf("  %s[%s]%s %s%s%s\n", on ? ui_c(CLR_GREEN) : ui_c(CLR_RED),
               on ? "UNLOCKED" : " LOCKED ", ui_c(CLR_RESET),
               ui_c(CLR_PURPLE), NAMES[a], ui_c(CLR_RESET));
        printf("             %s", DESCS[a]);
        if (on && r && r->dates[a][0]) printf("  (unlocked %s)", r->dates[a]);
        printf("\n");
    }
    printf("\n  Progress: %d of %d unlocked.\n", achievements_count(db, p->id), ACH_COUNT);
    ui_rule();
}
