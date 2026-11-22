/* player.h - Player profile structure, registration, ranks. */
#ifndef PLAYER_H
#define PLAYER_H
#include <stddef.h>

#define NAME_LEN 32
#define NICK_LEN 24

struct Database; /* defined in storage.h */

typedef struct {
    int  id;                 /* unique, never reused */
    char name[NAME_LEN];     /* unique (case-insensitive) */
    char nickname[NICK_LEN];
    int  xp;
    int  missions;           /* completed (won) missions */
    int  wins;
    int  losses;
    int  gamesPlayed;        /* wins + losses */
    int  streak;             /* current winning streak */
    int  bestStreak;
    int  champRound;         /* championship rounds completed (0..2) */
    int  titles;             /* championships won */
    int  dailyDate;          /* YYYYMMDD of last completed daily challenge */
} Player;

int    player_name_valid(const char *name);
Player *player_register(struct Database *db, const char *name, const char *nick, int *err);
Player *player_find_by_id(struct Database *db, int id);
Player *player_find_by_name(struct Database *db, const char *name);
Player *player_lookup(struct Database *db, const char *text); /* "Name", "7", "NA-0007" */
const char *player_rank(int xp);
int    player_rank_floor(int xp);
int    player_next_threshold(int xp);   /* -1 if already top rank */
double player_win_percent(const Player *p);
void   player_format_id(const Player *p, char *buf, size_t n);
void   player_print_dashboard(const Player *p);
void   player_print_profile(const Player *p, int achUnlocked);
#endif
