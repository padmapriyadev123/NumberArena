/* player.c - Registration, lookup, ranks and profile display. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "player.h"
#include "storage.h"
#include "ui.h"

int player_name_valid(const char *name)
{
    size_t i, len = strlen(name);
    if (len < 2 || len >= NAME_LEN) return 0;
    if (!isalpha((unsigned char)name[0])) return 0;   /* so a name is never confused with an ID */
    for (i = 0; i < len; i++) {
        unsigned char c = (unsigned char)name[i];
        if (!isalnum(c) && c != ' ' && c != '_' && c != '-') return 0;
    }
    return 1;
}

Player *player_find_by_id(Database *db, int id)
{
    size_t i;
    for (i = 0; i < db->nPlayers; i++)
        if (db->players[i].id == id) return &db->players[i];
    return NULL;
}

Player *player_find_by_name(Database *db, const char *name)
{
    size_t i;
    for (i = 0; i < db->nPlayers; i++)
        if (ui_ci_equal(db->players[i].name, name)) return &db->players[i];
    return NULL;
}

Player *player_lookup(Database *db, const char *text)
{
    const char *s = text;
    char *end;
    long id;
    if ((s[0] == 'N' || s[0] == 'n') && (s[1] == 'A' || s[1] == 'a') && s[2] == '-') s += 3;
    if (isdigit((unsigned char)s[0])) {
        id = strtol(s, &end, 10);
        if (*end == '\0' && id > 0 && id < 100000000) return player_find_by_id(db, (int)id);
        return NULL;
    }
    return player_find_by_name(db, text);
}

/* err: 1 = invalid name, 2 = duplicate name, 3 = out of memory */
Player *player_register(Database *db, const char *name, const char *nick, int *err)
{
    Player *p;
    size_t i;
    int maxId = 0;
    *err = 0;
    if (!player_name_valid(name)) { *err = 1; return NULL; }
    if (player_find_by_name(db, name)) { *err = 2; return NULL; }
    if (db->nPlayers == db->capPlayers) {
        size_t ncap = db->capPlayers ? db->capPlayers * 2 : 8;
        Player *np = realloc(db->players, ncap * sizeof *np);
        if (!np) { *err = 3; return NULL; }
        db->players = np;
        db->capPlayers = ncap;
    }
    for (i = 0; i < db->nPlayers; i++)
        if (db->players[i].id > maxId) maxId = db->players[i].id;
    p = &db->players[db->nPlayers++];
    memset(p, 0, sizeof *p);
    p->id = maxId + 1;
    snprintf(p->name, sizeof p->name, "%s", name);
    snprintf(p->nickname, sizeof p->nickname, "%s", (nick && nick[0]) ? nick : name);
    return p;
}

int player_rank_floor(int xp)
{
    if (xp >= 5000) return 5000;
    if (xp >= 3500) return 3500;
    if (xp >= 2000) return 2000;
    if (xp >= 1000) return 1000;
    if (xp >= 500)  return 500;
    return 0;
}

int player_next_threshold(int xp)
{
    if (xp >= 5000) return -1;
    if (xp >= 3500) return 5000;
    if (xp >= 2000) return 3500;
    if (xp >= 1000) return 2000;
    if (xp >= 500)  return 1000;
    return 500;
}

const char *player_rank(int xp)
{
    if (xp >= 5000) return "Diamond";
    if (xp >= 3500) return "Platinum";
    if (xp >= 2000) return "Gold";
    if (xp >= 1000) return "Silver";
    if (xp >= 500)  return "Bronze";
    return "Rookie";
}

double player_win_percent(const Player *p)
{
    return p->gamesPlayed > 0 ? 100.0 * p->wins / p->gamesPlayed : 0.0;
}

void player_format_id(const Player *p, char *buf, size_t n)
{
    snprintf(buf, n, "NA-%04d", p->id);
}

static void print_rank_progress(const Player *p)
{
    int next = player_next_threshold(p->xp);
    printf("  Rank            : %s%s%s", ui_c(CLR_PURPLE), player_rank(p->xp), ui_c(CLR_RESET));
    if (next < 0) {
        printf("  (maximum rank reached)\n");
    } else {
        int floor = player_rank_floor(p->xp), span = next - floor, done = p->xp - floor;
        int filled = span > 0 ? done * 20 / span : 0, i;
        printf("  [");
        for (i = 0; i < 20; i++) putchar(i < filled ? '#' : '-');
        printf("] %d/%d XP to next rank\n", done, span);
    }
}

void player_print_dashboard(const Player *p)
{
    char id[16];
    player_format_id(p, id, sizeof id);
    ui_title("PLAYER DASHBOARD");
    printf("  Player          : %s%s%s  \"%s\"  (%s)\n", ui_c(CLR_CYAN), p->name, ui_c(CLR_RESET), p->nickname, id);
    printf("  Total XP        : %s%d%s\n", ui_c(CLR_YELLOW), p->xp, ui_c(CLR_RESET));
    print_rank_progress(p);
    printf("  Streak          : %d (best %d)   Championship: round %d of 3 next\n",
           p->streak, p->bestStreak, p->champRound + 1);
    ui_rule();
}

void player_print_profile(const Player *p, int achUnlocked)
{
    char id[16];
    player_format_id(p, id, sizeof id);
    ui_title("PLAYER PROFILE");
    printf("  Name            : %s\n", p->name);
    printf("  Nickname        : %s\n", p->nickname);
    printf("  Player ID       : %s\n", id);
    printf("  Total XP        : %s%d%s\n", ui_c(CLR_YELLOW), p->xp, ui_c(CLR_RESET));
    print_rank_progress(p);
    printf("  Games played    : %d\n", p->gamesPlayed);
    printf("  Completed (won) : %d\n", p->missions);
    printf("  Wins / Losses   : %d / %d  (%.1f%% win rate)\n", p->wins, p->losses, player_win_percent(p));
    printf("  Current streak  : %d   Best streak: %d\n", p->streak, p->bestStreak);
    printf("  Championships   : %d   Current run: %d of 3 rounds cleared\n", p->titles, p->champRound);
    printf("  Achievements    : %d of 6 unlocked\n", achUnlocked);
    ui_rule();
}
