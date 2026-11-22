/* leaderboard.h - Local Top 10 leaderboard. */
#ifndef LEADERBOARD_H
#define LEADERBOARD_H
#include "storage.h"

#define LB_SIZE 10

typedef struct {
    int  playerId;
    char name[NAME_LEN];
    int  xp, gamesPlayed, wins;
} LeaderEntry;

int  leaderboard_save(const Database *db);                     /* write snapshot to leaderboard.dat */
void leaderboard_show(const Database *db, int highlightId);
#endif
