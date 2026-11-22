/* leaderboard.c - Local Top 10, sorted by XP (desc), then wins (desc), then player ID (asc). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "leaderboard.h"
#include "ui.h"

static int cmp_players(const void *a, const void *b)
{
    const Player *pa = *(const Player *const *)a;
    const Player *pb = *(const Player *const *)b;
    if (pa->xp != pb->xp)     return pb->xp > pa->xp ? 1 : -1;
    if (pa->wins != pb->wins) return pb->wins > pa->wins ? 1 : -1;
    return pa->id - pb->id;
}

/* Returns a malloc'd, sorted array of pointers into db->players (caller frees). */
static const Player **sorted_all(const Database *db)
{
    size_t i;
    const Player **arr;
    if (db->nPlayers == 0) return NULL;
    arr = malloc(db->nPlayers * sizeof *arr);
    if (!arr) return NULL;
    for (i = 0; i < db->nPlayers; i++) arr[i] = &db->players[i];
    qsort(arr, db->nPlayers, sizeof *arr, cmp_players);
    return arr;
}

int leaderboard_save(const Database *db)
{
    const Player **arr = sorted_all(db);
    LeaderEntry e;
    size_t i, n = db->nPlayers < LB_SIZE ? db->nPlayers : LB_SIZE;
    FILE *f = fopen(LEADERBOARD_FILE, "wb");
    if (!f) { free(arr); return -1; }
    for (i = 0; arr && i < n; i++) {
        memset(&e, 0, sizeof e);
        e.playerId = arr[i]->id;
        snprintf(e.name, sizeof e.name, "%s", arr[i]->name);
        e.xp = arr[i]->xp; e.gamesPlayed = arr[i]->gamesPlayed; e.wins = arr[i]->wins;
        if (fwrite(&e, sizeof e, 1, f) != 1) { fclose(f); free(arr); return -1; }
    }
    free(arr);
    return fclose(f) == 0 ? 0 : -1;
}

void leaderboard_show(const Database *db, int highlightId)
{
    const Player **arr = sorted_all(db);
    size_t i, n = db->nPlayers < LB_SIZE ? db->nPlayers : LB_SIZE;
    ui_title("LOCAL LEADERBOARD - TOP 10");
    if (!arr) { printf("  No players registered yet.\n"); ui_rule(); return; }
    printf("  %-3s %-16s %-8s %-7s %-9s %-6s %-5s %s\n", "#", "Player", "ID", "XP", "Rank", "Games", "Won", "Win%");
    printf("  ------------------------------------------------------------------\n");
    for (i = 0; i < n; i++) {
        const Player *p = arr[i];
        char id[16];
        player_format_id(p, id, sizeof id);
        printf("  %s%-3d %-16.16s %-8s %-7d %-9s %-6d %-5d %.1f%%%s\n",
               p->id == highlightId ? ui_c(CLR_YELLOW) : "",
               (int)i + 1, p->name, id, p->xp, player_rank(p->xp), p->gamesPlayed, p->wins,
               player_win_percent(p), ui_c(CLR_RESET));
    }
    for (i = n; i < db->nPlayers; i++)
        if (arr[i]->id == highlightId) printf("  ...\n  Your position: #%d of %d players\n", (int)i + 1, (int)db->nPlayers);
    printf("\n  Ties: more wins first, then the lower (older) player ID.\n");
    printf("  This is a LOCAL leaderboard stored on this computer only.\n");
    ui_rule();
    free(arr);
}
