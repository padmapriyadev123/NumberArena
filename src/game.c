/* game.c - Mission engine plus Quick Play, Time Attack and Daily Challenge modes.
 *
 * Time tracking note: the console blocks inside fgets() while waiting for input, so
 * the program cannot redraw a live countdown.  Elapsed time (time(), 1-second
 * resolution) is checked before each prompt and again after the player presses ENTER.
 * Input submitted after the deadline is rejected.  The time left shown is a snapshot.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include "game.h"
#include "storage.h"
#include "hints.h"
#include "achievements.h"
#include "leaderboard.h"
#include "ui.h"

#define MAX_EXTRA_HINTS 2
#define DAILY_BONUS_XP  150

typedef struct { const char *name; int min, max, attempts, base; } DiffDef;
static const DiffDef DIFFS[4] = {
    { "Easy",   1,   50, 10,  50 },
    { "Medium", 1,  100,  8, 100 },
    { "Hard",   1,  500, 10, 200 },
    { "Expert", 1, 1000, 12, 300 }
};

/* Unbiased random integer in [lo, hi] using rejection sampling. */
static int rand_range(int lo, int hi)
{
    unsigned long span = (unsigned long)(hi - lo + 1);
    unsigned long limit = ((unsigned long)RAND_MAX + 1UL) / span * span;
    unsigned long r;
    do { r = (unsigned long)rand(); } while (r >= limit);
    return lo + (int)(r % span);
}

int game_today_key(void)
{
    time_t t = time(NULL);
    struct tm *tmv = localtime(&t);
    if (!tmv) return 19700101;
    return (tmv->tm_year + 1900) * 10000 + (tmv->tm_mon + 1) * 100 + tmv->tm_mday;
}

static int lifeline_penalty(int base)
{
    int pen = base * 30 / 100;
    return pen < 20 ? 20 : pen;
}

static int parse_guess(const char *s, int *out)
{
    char *end;
    long v;
    errno = 0;
    v = strtol(s, &end, 10);
    if (end == s || *end != '\0' || errno == ERANGE || v < INT_MIN || v > INT_MAX) return 0;
    *out = (int)v;
    return 1;
}

static void print_briefing(const MissionConfig *cfg)
{
    ui_title(cfg->title);
    printf("  MISSION BRIEFING\n");
    printf("  Mode        : %s (%s)\n", cfg->modeName, cfg->diffName);
    printf("  Secret      : a number from %d to %d\n", cfg->min, cfg->max);
    printf("  Attempts    : %d\n", cfg->maxAttempts);
    if (cfg->timeLimit > 0) printf("  Time limit  : %d seconds\n", cfg->timeLimit);
    printf("  Reward      : %s%d base points / XP%s\n", ui_c(CLR_YELLOW), cfg->baseReward, ui_c(CLR_RESET));
    printf("  Support     : %d extra hints (H) and 1 lifeline (L, costs %d points)\n",
           MAX_EXTRA_HINTS, lifeline_penalty(cfg->baseReward));
    ui_rule();
}

static void extra_hint_menu(int secret, const MissionConfig *cfg, int *lo, int *hi,
                            int last, int haveLast, int *hintsLeft, MissionResult *res)
{
    int choice;
    if (*hintsLeft <= 0) { printf("%sNo extra hints left in this mission.%s\n", ui_c(CLR_RED), ui_c(CLR_RESET)); return; }
    printf("  Extra hints left: %d (using one makes the mission 'aided': no accuracy bonus)\n", *hintsLeft);
    printf("  1) Hot/Cold (needs a previous guess)\n  2) Odd/Even\n  3) Range hint (which half of the known range)\n  0) Cancel\n");
    if (!ui_read_int("  Choose hint > ", 0, 3, &choice) || choice == 0) return;
    if (choice == 1) {
        if (!haveLast) { printf("%sMake a guess first - Hot/Cold compares against it. (not charged)%s\n", ui_c(CLR_RED), ui_c(CLR_RESET)); return; }
        printf("  %sHINT:%s your last guess (%d) is %s.\n", ui_c(CLR_CYAN), ui_c(CLR_RESET), last,
               hint_hot_cold(res->secret, last, cfg->min, cfg->max));
    } else if (choice == 2) {
        printf("  %sHINT:%s the secret number is %s.\n", ui_c(CLR_CYAN), ui_c(CLR_RESET), hint_odd_even(res->secret));
    } else {
        int nlo, nhi;
        hint_range(secret, *lo, *hi, &nlo, &nhi);
        *lo = nlo; *hi = nhi;
        printf("  %sHINT:%s the secret lies between %d and %d.\n", ui_c(CLR_CYAN), ui_c(CLR_RESET), *lo, *hi);
    }
    (*hintsLeft)--;
    res->extraHints++;
}

static void show_result(const Player *p, const MissionConfig *cfg, const MissionResult *res,
                        const char *oldRank)
{
    const ScoreBreakdown *s = &res->sb;
    printf("\n");
    if (res->won) {
        printf("%s%s", ui_c(CLR_GREEN), ui_c(CLR_BOLD));
        printf("  ****************************************\n  *     V I C T O R Y  -  CODE CRACKED   *\n  ****************************************%s\n", ui_c(CLR_RESET));
        printf("  Secret number : %d\n  Attempts used : %d of %d\n  Time taken    : %.0f s\n",
               res->secret, res->attempts, cfg->maxAttempts, res->seconds);
        printf("  --- Score breakdown ---\n");
        printf("  Base points        : %6d\n  Accuracy bonus     : %6d\n  Speed bonus        : %6d\n",
               s->base, s->accuracy, s->speed);
        printf("  Remaining attempts : %6d\n  Streak bonus       : %6d\n  Lifeline penalty   : %6d\n",
               s->attemptsBonus, s->streakBonus, -s->lifelinePenalty);
        printf("  %sTOTAL SCORE        : %6d%s\n", ui_c(CLR_YELLOW), res->score, ui_c(CLR_RESET));
    } else {
        printf("%s%s", ui_c(CLR_RED), ui_c(CLR_BOLD));
        printf("  ****************************************\n  *      D E F E A T  -  MISSION FAILED  *\n  ****************************************%s\n", ui_c(CLR_RESET));
        if (res->timedOut) printf("  Time ran out!\n");
        else if (res->quit) printf("  You forfeited the mission.\n");
        else printf("  You ran out of attempts.\n");
        printf("  The secret number was %d.\n", res->secret);
    }
    printf("  %sXP gained: +%d%s   Total XP: %d\n", ui_c(CLR_YELLOW), res->xpGained, ui_c(CLR_RESET), p->xp);
    if (strcmp(oldRank, player_rank(p->xp)) != 0)
        printf("  %s>>> RANK UP! %s -> %s <<<%s\n", ui_c(CLR_PURPLE), oldRank, player_rank(p->xp), ui_c(CLR_RESET));
    printf("  Streak: %d\n", p->streak);
}

void game_run_mission(Database *db, Player *p, const MissionConfig *cfg, MissionResult *res)
{
    int lo = cfg->min, hi = cfg->max, last = 0, haveLast = 0, hintsLeft = MAX_EXTRA_HINTS;
    int solved = 0;
    char line[64], oldRank[16];
    time_t start;
    HistoryRecord hr;
    struct tm *tmv;
    time_t now;

    memset(res, 0, sizeof *res);
    res->secret = cfg->fixedSecret > 0 ? cfg->fixedSecret : rand_range(cfg->min, cfg->max);
    snprintf(oldRank, sizeof oldRank, "%s", player_rank(p->xp));
    print_briefing(cfg);
    start = time(NULL);

    while (res->attempts < cfg->maxAttempts) {
        int guess;
        if (cfg->timeLimit > 0) {
            double el = difftime(time(NULL), start);
            if (el >= cfg->timeLimit) { res->timedOut = 1; break; }
        }
        printf("\n%s[Attempt %d/%d]%s Known range: %s%d-%d%s | Extra hints: %d | Lifeline: %s",
               ui_c(CLR_CYAN), res->attempts + 1, cfg->maxAttempts, ui_c(CLR_RESET),
               ui_c(CLR_YELLOW), lo, hi, ui_c(CLR_RESET), hintsLeft,
               res->lifelineUsed ? "used" : "ready");
        if (cfg->timeLimit > 0)
            printf(" | Time left (snapshot): %ds",
                   cfg->timeLimit - (int)difftime(time(NULL), start));
        printf("\n  Guess, or H=hint, L=lifeline, Q=quit > ");
        if (!ui_read_line(line, sizeof line)) { res->quit = 1; break; }
        if (cfg->timeLimit > 0 && difftime(time(NULL), start) > cfg->timeLimit) {
            printf("%s  Time limit exceeded - that input arrived after the deadline.%s\n", ui_c(CLR_RED), ui_c(CLR_RESET));
            res->timedOut = 1; break;
        }
        if (line[0] == '\0') { printf("%s  Please type a number.%s\n", ui_c(CLR_RED), ui_c(CLR_RESET)); continue; }

        if (line[1] == '\0' && toupper((unsigned char)line[0]) == 'H') {
            extra_hint_menu(res->secret, cfg, &lo, &hi, last, haveLast, &hintsLeft, res);
            continue;
        }
        if (line[1] == '\0' && toupper((unsigned char)line[0]) == 'L') {
            if (res->lifelineUsed) { printf("%s  Lifeline already used in this mission.%s\n", ui_c(CLR_RED), ui_c(CLR_RESET)); continue; }
            printf("  The lifeline narrows the range to about 25%% but costs %d points.\n", lifeline_penalty(cfg->baseReward));
            if (ui_confirm("  Use lifeline? (y/n) > ")) {
                int nlo, nhi;
                hint_lifeline(res->secret, lo, hi, &nlo, &nhi);
                lo = nlo; hi = nhi;
                res->lifelineUsed = 1;
                printf("  %sLIFELINE:%s the secret is between %d and %d.\n", ui_c(CLR_PURPLE), ui_c(CLR_RESET), lo, hi);
            }
            continue;
        }
        if (line[1] == '\0' && toupper((unsigned char)line[0]) == 'Q') {
            if (ui_confirm("  Forfeit this mission? It counts as a loss. (y/n) > ")) { res->quit = 1; break; }
            continue;
        }
        if (!parse_guess(line, &guess)) {
            printf("%s  Invalid input. Enter a whole number, H, L or Q. (no attempt used)%s\n", ui_c(CLR_RED), ui_c(CLR_RESET));
            continue;
        }
        if (guess < cfg->min || guess > cfg->max) {
            printf("%s  Out of range! Enter a number from %d to %d. (no attempt used)%s\n",
                   ui_c(CLR_RED), cfg->min, cfg->max, ui_c(CLR_RESET));
            continue;
        }
        res->attempts++;
        if (guess < lo || guess > hi)
            printf("  Note: you already know the secret is in %d-%d, so that guess was wasted.\n", lo, hi);
        if (guess == res->secret) { solved = 1; break; }
        printf("  %sWrong!%s The secret number is %s%s%s.\n", ui_c(CLR_RED), ui_c(CLR_RESET),
               ui_c(CLR_YELLOW), hint_higher_lower(res->secret, guess), ui_c(CLR_RESET));
        if (guess < res->secret) { if (guess + 1 > lo) lo = guess + 1; }
        else                     { if (guess - 1 < hi) hi = guess - 1; }
        last = guess; haveLast = 1;
    }

    res->seconds = difftime(time(NULL), start);
    res->won = solved;

    if (solved) {
        ScoreInput in;
        in.base = cfg->baseReward; in.maxAttempts = cfg->maxAttempts; in.attemptsUsed = res->attempts;
        in.unaided = (!res->lifelineUsed && res->extraHints == 0);
        in.seconds = res->seconds; in.timeLimit = cfg->timeLimit;
        in.lifelineUsed = res->lifelineUsed; in.newStreak = p->streak + 1;
        res->sb = scoring_compute(&in);
        res->score = res->sb.total;
        p->streak++;
        if (p->streak > p->bestStreak) p->bestStreak = p->streak;
        p->wins++; p->missions++;
    } else {
        p->streak = 0;
        p->losses++;
    }
    p->gamesPlayed++;
    res->xpGained = scoring_xp(cfg->baseReward, res->score, solved);
    p->xp = scoring_add_xp(p->xp, res->xpGained);

    memset(&hr, 0, sizeof hr);
    hr.playerId = p->id;
    snprintf(hr.mode, sizeof hr.mode, "%s", cfg->modeName);
    snprintf(hr.difficulty, sizeof hr.difficulty, "%s", cfg->diffName);
    hr.secret = res->secret; hr.attempts = res->attempts; hr.seconds = res->seconds;
    hr.score = res->score; hr.won = solved;
    now = time(NULL); tmv = localtime(&now);
    if (tmv) strftime(hr.date, sizeof hr.date, "%Y-%m-%d %H:%M", tmv);

    show_result(p, cfg, res, oldRank);
    achievements_check_mission(db, p, cfg, res);
    if (storage_append_history(&hr) != 0)
        printf("%sWARNING: could not write game history.%s\n", ui_c(CLR_RED), ui_c(CLR_RESET));
    if (storage_save(db) != 0)
        printf("%sWARNING: progress could not be saved!%s\n", ui_c(CLR_RED), ui_c(CLR_RESET));
}

static int choose_difficulty(void)
{
    int c, i;
    ui_title("SELECT DIFFICULTY");
    for (i = 0; i < 4; i++)
        printf("  %d) %-7s range %d-%-5d %2d attempts   base %d XP\n", i + 1, DIFFS[i].name,
               DIFFS[i].min, DIFFS[i].max, DIFFS[i].attempts, DIFFS[i].base);
    printf("  0) Back\n");
    if (!ui_read_int("  Choose > ", 0, 4, &c)) return -1;
    return c - 1;   /* -1 = back */
}

void game_quick_play(Database *db, Player *p)
{
    int again = 1;
    while (again) {
        int d = choose_difficulty();
        MissionConfig cfg;
        MissionResult res;
        if (d < 0) return;
        memset(&cfg, 0, sizeof cfg);
        cfg.mode = MODE_QUICK; cfg.modeName = "Quick Play"; cfg.diffName = DIFFS[d].name;
        cfg.title = "QUICK PLAY"; cfg.min = DIFFS[d].min; cfg.max = DIFFS[d].max;
        cfg.maxAttempts = DIFFS[d].attempts; cfg.baseReward = DIFFS[d].base;
        game_run_mission(db, p, &cfg, &res);
        if (res.quit && !res.won && res.attempts == 0 && feof(stdin)) return;
        again = ui_confirm("\n  Play again? (y/n) > ");
    }
}

void game_time_attack(Database *db, Player *p)
{
    int again = 1;
    ui_title("TIME ATTACK");
    printf("  Solve the number before the clock runs out. Faster = bigger speed bonus.\n");
    printf("  NOTE: the console waits for you to press ENTER, so the time limit is checked\n");
    printf("  when you submit each input (1-second resolution). There is no live countdown.\n");
    while (again) {
        int d, limit;
        MissionConfig cfg;
        MissionResult res;
        if (!ui_read_int("  Time limit in seconds (15-600) > ", 15, 600, &limit)) return;
        d = choose_difficulty();
        if (d < 0) return;
        memset(&cfg, 0, sizeof cfg);
        cfg.mode = MODE_TIME; cfg.modeName = "Time Attack"; cfg.diffName = DIFFS[d].name;
        cfg.title = "TIME ATTACK"; cfg.min = DIFFS[d].min; cfg.max = DIFFS[d].max;
        cfg.maxAttempts = DIFFS[d].attempts; cfg.baseReward = DIFFS[d].base; cfg.timeLimit = limit;
        game_run_mission(db, p, &cfg, &res);
        again = ui_confirm("\n  Play again? (y/n) > ");
    }
}

/* Deterministic daily secret: same date => same number for everyone. */
static int daily_secret(int dateKey)
{
    unsigned int x = (unsigned int)dateKey * 2654435761u;
    x ^= x >> 15; x *= 2246822519u; x ^= x >> 13;
    return 1 + (int)(x % 500u);
}

void game_daily_challenge(Database *db, Player *p)
{
    int key = game_today_key();
    MissionConfig cfg;
    MissionResult res;
    ui_title("DAILY CHALLENGE");
    if (p->dailyDate == key) {
        printf("  You already completed today's challenge (%d). Come back tomorrow!\n", key);
        ui_rule();
        return;
    }
    printf("  Today's code is the same all day. Crack it for +%d bonus XP (once per day).\n", DAILY_BONUS_XP);
    printf("  You may retry until you succeed; the bonus is paid on the first success only.\n");
    memset(&cfg, 0, sizeof cfg);
    cfg.mode = MODE_DAILY; cfg.modeName = "Daily"; cfg.diffName = "Daily";
    cfg.title = "DAILY CHALLENGE"; cfg.min = 1; cfg.max = 500; cfg.maxAttempts = 9;
    cfg.baseReward = 150; cfg.fixedSecret = daily_secret(key);
    game_run_mission(db, p, &cfg, &res);
    if (res.won) {
        p->xp = scoring_add_xp(p->xp, DAILY_BONUS_XP);
        p->dailyDate = key;
        printf("  %sDAILY BONUS: +%d XP!%s  Total XP: %d (%s)\n", ui_c(CLR_YELLOW), DAILY_BONUS_XP,
               ui_c(CLR_RESET), p->xp, player_rank(p->xp));
        if (storage_save(db) != 0)
            printf("%sWARNING: progress could not be saved!%s\n", ui_c(CLR_RED), ui_c(CLR_RESET));
    }
}
