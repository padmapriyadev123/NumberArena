/* tournament.c - Championship: three rounds that must be cleared in order. */
#include <stdio.h>
#include <string.h>
#include "tournament.h"
#include "game.h"
#include "ui.h"

static const MissionConfig ROUNDS[3] = {
    { .mode = MODE_CHAMPIONSHIP, .modeName = "Championship", .diffName = "Beginner",
      .title = "ROUND 1 - QUALIFICATION", .min = 1, .max = 50,   .maxAttempts = 10, .baseReward = 100 },
    { .mode = MODE_CHAMPIONSHIP, .modeName = "Championship", .diffName = "Intermediate",
      .title = "ROUND 2 - SEMI-FINAL",    .min = 1, .max = 200,  .maxAttempts = 7,  .baseReward = 200 },
    { .mode = MODE_CHAMPIONSHIP, .modeName = "Championship", .diffName = "Expert",
      .title = "ROUND 3 - GRAND FINALE",  .min = 1, .max = 1000, .maxAttempts = 8,  .baseReward = 500 }
};

static void show_progress(const Player *p, const int scores[3])
{
    int i;
    ui_title("CHAMPIONSHIP PROGRESS");
    for (i = 0; i < 3; i++) {
        const char *status = i < p->champRound ? "CLEARED" : (i == p->champRound ? "NEXT   " : "LOCKED ");
        const char *col = i < p->champRound ? ui_c(CLR_GREEN) : (i == p->champRound ? ui_c(CLR_YELLOW) : ui_c(CLR_RED));
        printf("  %s[%s]%s %-26s range 1-%-4d %2d attempts  %3d XP",
               col, status, ui_c(CLR_RESET), ROUNDS[i].title, ROUNDS[i].max, ROUNDS[i].maxAttempts, ROUNDS[i].baseReward);
        if (scores[i] >= 0) printf("  score this session: %d", scores[i]);
        printf("\n");
    }
    ui_rule();
}

static void celebrate(const Player *p, const int scores[3])
{
    int i, total = 0;
    for (i = 0; i < 3; i++) if (scores[i] > 0) total += scores[i];
    printf("\n%s%s", ui_c(CLR_PURPLE), ui_c(CLR_BOLD));
    printf("  ==============================================\n");
    printf("   *  *  *   C H A M P I O N   *  *  *\n");
    printf("   %s has conquered NUMBER ARENA!\n", p->name);
    printf("  ==============================================%s\n", ui_c(CLR_RESET));
    printf("  Championship titles : %d\n  Total XP            : %d (%s)\n", p->titles, p->xp, player_rank(p->xp));
    printf("  Score earned this session across rounds: %d\n", total);
    printf("  Your championship run has been reset so you can compete again.\n");
}

void tournament_run(Database *db, Player *p)
{
    int scores[3] = { -1, -1, -1 };
    for (;;) {
        int choice;
        MissionResult res;
        char prompt[64];
        show_progress(p, scores);
        snprintf(prompt, sizeof prompt, "  1) Play Round %d   0) Back to menu > ", p->champRound + 1);
        if (!ui_read_int(prompt, 0, 1, &choice) || choice == 0) return;
        game_run_mission(db, p, &ROUNDS[p->champRound], &res);
        if (res.won) {
            scores[p->champRound] = res.score;
            p->champRound++;
            if (p->champRound >= 3) {
                p->titles++;
                p->champRound = 0;
                achievements_unlock(db, p->id, ACH_CHAMPION);
                celebrate(p, scores);
                if (storage_save(db) != 0) printf("%sWARNING: could not save!%s\n", ui_c(CLR_RED), ui_c(CLR_RESET));
                ui_pause();
                return;
            }
            printf("\n  %sRound cleared! The next round is unlocked.%s\n", ui_c(CLR_GREEN), ui_c(CLR_RESET));
        } else {
            printf("\n  %sYou did not clear this round. Your progress is kept - try again!%s\n", ui_c(CLR_RED), ui_c(CLR_RESET));
            if (res.quit && feof(stdin)) return;
        }
        if (storage_save(db) != 0) printf("%sWARNING: could not save!%s\n", ui_c(CLR_RED), ui_c(CLR_RESET));
        ui_pause();
    }
}
