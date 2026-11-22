/* storage.c - Binary file persistence with corruption handling. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "storage.h"
#include "leaderboard.h"
#include "ui.h"

#ifdef _WIN32
#include <direct.h>
#define MAKE_DIR(p) _mkdir(p)
#else
#include <sys/stat.h>
#include <sys/types.h>
#define MAKE_DIR(p) mkdir((p), 0755)
#endif

static int touch_file(const char *path)
{
    FILE *f = fopen(path, "ab");
    if (!f) return -1;
    return fclose(f) == 0 ? 0 : -1;
}

int storage_init(void)
{
    const char *files[4] = { PLAYERS_FILE, LEADERBOARD_FILE, ACH_FILE, HISTORY_FILE };
    int i;
    MAKE_DIR(DATA_DIR);                       /* fails harmlessly if it already exists */
    for (i = 0; i < 4; i++) {
        if (touch_file(files[i]) != 0) {
            printf("%sERROR: cannot create/open '%s'. Run the game from the NUMBER_ARENA folder.%s\n",
                   ui_c(CLR_RED), files[i], ui_c(CLR_RESET));
            return -1;
        }
    }
    return 0;
}

/* Load all records of size rec.  Returns 0 = ok/empty, 1 = corrupt (backed up, starts empty), -1 = no memory. */
static int load_records(const char *path, void **out, size_t *count, size_t rec)
{
    FILE *f;
    long size;
    size_t n;
    void *buf;
    *out = NULL; *count = 0;
    f = fopen(path, "rb");
    if (!f) return 0;                         /* missing file == empty data */
    if (fseek(f, 0, SEEK_END) != 0 || (size = ftell(f)) < 0 || fseek(f, 0, SEEK_SET) != 0)
        size = -1;
    if (size < 0 || (size_t)size % rec != 0) {
        char bak[128];
        fclose(f);
        snprintf(bak, sizeof bak, "%s.bak", path);
        remove(bak);
        rename(path, bak);
        printf("%sWARNING: '%s' is corrupted. Backed up as '%s'; starting with empty data.%s\n",
               ui_c(CLR_RED), path, bak, ui_c(CLR_RESET));
        touch_file(path);
        return 1;
    }
    if (size == 0) { fclose(f); return 0; }
    n = (size_t)size / rec;
    buf = malloc(n * rec);
    if (!buf) { fclose(f); return -1; }
    if (fread(buf, rec, n, f) != n) { free(buf); fclose(f); return 1; }
    fclose(f);
    *out = buf; *count = n;
    return 0;
}

/* Drops invalid or duplicate players so the rest of the program can trust the data. */
static void sanitise_players(Database *db)
{
    size_t i, j, keep = 0;
    for (i = 0; i < db->nPlayers; i++) {
        Player *p = &db->players[i];
        int dup = 0;
        p->name[NAME_LEN - 1] = '\0';
        p->nickname[NICK_LEN - 1] = '\0';
        if (p->id <= 0 || p->name[0] == '\0') continue;
        for (j = 0; j < keep; j++)
            if (db->players[j].id == p->id || ui_ci_equal(db->players[j].name, p->name)) { dup = 1; break; }
        if (dup) continue;
        if (p->xp < 0) p->xp = 0;
        if (p->champRound < 0 || p->champRound > 2) p->champRound = 0;
        if (p->streak < 0) p->streak = 0;
        db->players[keep++] = *p;
    }
    db->nPlayers = keep;
}

int storage_load(Database *db)
{
    void *p = NULL, *a = NULL;
    size_t np = 0, na = 0;
    int r1, r2, i;
    memset(db, 0, sizeof *db);
    r1 = load_records(PLAYERS_FILE, &p, &np, sizeof(Player));
    r2 = load_records(ACH_FILE, &a, &na, sizeof(AchRecord));
    if (r1 < 0 || r2 < 0) { free(p); free(a); return -1; }
    db->players = p; db->nPlayers = db->capPlayers = np;
    db->ach = a;     db->nAch = db->capAch = na;
    sanitise_players(db);
    for (i = 0; i < (int)db->nAch; i++) db->ach[i].mask &= (1u << ACH_COUNT) - 1u;
    return 0;
}

static int write_records(const char *path, const void *data, size_t rec, size_t n)
{
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    if (n > 0 && fwrite(data, rec, n, f) != n) { fclose(f); return -1; }
    return fclose(f) == 0 ? 0 : -1;
}

int storage_save(const Database *db)
{
    int bad = 0;
    if (write_records(PLAYERS_FILE, db->players, sizeof(Player), db->nPlayers) != 0) {
        printf("%sERROR: could not write %s%s\n", ui_c(CLR_RED), PLAYERS_FILE, ui_c(CLR_RESET)); bad = 1;
    }
    if (write_records(ACH_FILE, db->ach, sizeof(AchRecord), db->nAch) != 0) {
        printf("%sERROR: could not write %s%s\n", ui_c(CLR_RED), ACH_FILE, ui_c(CLR_RESET)); bad = 1;
    }
    if (leaderboard_save(db) != 0) {
        printf("%sERROR: could not write %s%s\n", ui_c(CLR_RED), LEADERBOARD_FILE, ui_c(CLR_RESET)); bad = 1;
    }
    return bad ? -1 : 0;
}

void storage_free(Database *db)
{
    free(db->players);
    free(db->ach);
    memset(db, 0, sizeof *db);
}

int storage_append_history(const HistoryRecord *r)
{
    FILE *f = fopen(HISTORY_FILE, "ab");
    size_t ok;
    if (!f) return -1;
    ok = fwrite(r, sizeof *r, 1, f);
    if (fclose(f) != 0 || ok != 1) return -1;
    return 0;
}

void storage_show_history(const Player *p)
{
    HistoryRecord *all = NULL;
    size_t n = 0, i, shown = 0, mine = 0;
    int wins = 0;
    long totalScore = 0;
    ui_title("GAME HISTORY");
    if (load_records(HISTORY_FILE, (void **)&all, &n, sizeof(HistoryRecord)) != 0) {
        printf("  Could not read history.\n"); ui_rule(); free(all); return;
    }
    for (i = 0; i < n; i++)
        if (all[i].playerId == p->id) { mine++; if (all[i].won) wins++; totalScore += all[i].score; }
    if (mine == 0) {
        printf("  No games recorded yet. Play a mission first!\n");
    } else {
        size_t skip = mine > 20 ? mine - 20 : 0, seen = 0;
        printf("  %-3s %-16s %-13s %-13s %-6s %-9s %-5s %-6s %s\n",
               "#", "Date", "Mode", "Level", "Secret", "Attempts", "Sec", "Score", "Result");
        printf("  ---------------------------------------------------------------------------------------\n");
        for (i = 0; i < n; i++) {
            if (all[i].playerId != p->id) continue;
            if (seen++ < skip) continue;
            all[i].mode[15] = all[i].difficulty[15] = all[i].date[19] = '\0';
            printf("  %-3d %-16s %-13s %-13s %-6d %-9d %-5.0f %-6d %s%s%s\n",
                   (int)seen, all[i].date, all[i].mode, all[i].difficulty, all[i].secret,
                   all[i].attempts, all[i].seconds, all[i].score,
                   all[i].won ? ui_c(CLR_GREEN) : ui_c(CLR_RED), all[i].won ? "WIN" : "LOSS", ui_c(CLR_RESET));
            shown++;
        }
        printf("\n  Showing last %d of %d games | Wins: %d | Total score: %ld\n",
               (int)shown, (int)mine, wins, totalScore);
    }
    ui_rule();
    free(all);
}
