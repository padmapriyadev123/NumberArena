/* storage.h - Permanent binary file storage (players, achievements, leaderboard, history). */
#ifndef STORAGE_H
#define STORAGE_H
#include <stddef.h>
#include "player.h"
#include "achievements.h"

#define DATA_DIR         "data"
#define PLAYERS_FILE     DATA_DIR "/players.dat"
#define LEADERBOARD_FILE DATA_DIR "/leaderboard.dat"
#define ACH_FILE         DATA_DIR "/achievements.dat"
#define HISTORY_FILE     DATA_DIR "/history.dat"

typedef struct {
    int    playerId;
    char   mode[16];
    char   difficulty[16];
    int    secret;
    int    attempts;
    double seconds;
    int    score;
    int    won;
    char   date[20];
} HistoryRecord;

struct Database {
    Player    *players;  size_t nPlayers, capPlayers;
    AchRecord *ach;      size_t nAch, capAch;
};
typedef struct Database Database;

int  storage_init(void);                 /* create data dir + empty files if missing; 0 = ok */
int  storage_load(Database *db);         /* tolerant of missing/corrupt files; 0 = ok */
int  storage_save(const Database *db);   /* players + achievements + leaderboard snapshot; 0 = ok */
void storage_free(Database *db);
int  storage_append_history(const HistoryRecord *r);
void storage_show_history(const Player *p);
#endif
