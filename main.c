/* main.c - NUMBER ARENA entry point: login, main menu, shutdown. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "ui.h"
#include "player.h"
#include "storage.h"
#include "game.h"
#include "tournament.h"
#include "leaderboard.h"
#include "achievements.h"

static void show_instructions(void)
{
    ui_title("INSTRUCTIONS");
    printf("  * The computer picks a secret number. Guess it within the attempt limit.\n");
    printf("  * After each wrong guess you are told HIGHER or LOWER.\n");
    printf("  * H = extra hint (Hot/Cold, Odd/Even, Range) - 2 per mission, free but 'aided'.\n");
    printf("  * L = lifeline (once per mission) - narrows the range, costs points.\n");
    printf("  * Q = forfeit (counts as a loss). Invalid or out-of-range guesses cost nothing.\n");
    printf("  * Score = base + accuracy + speed + remaining attempts + streak - lifeline.\n");
    printf("  * XP: win = base reward + score/10, loss = %d. XP decides your rank:\n", LOSS_XP);
    printf("    Rookie 0 | Bronze 500 | Silver 1000 | Gold 2000 | Platinum 3500 | Diamond 5000\n");
    printf("  * Championship: clear Round 1, 2 and 3 in order to become Champion.\n");
    printf("  * Time Attack: the time limit is checked when you press ENTER (no live countdown).\n");
    printf("  * Daily Challenge: same code all day, bonus XP once per day.\n");
    ui_rule();
}

static Player *login(Database *db)
{
    char line[64], nick[NICK_LEN];
    for (;;) {
        int c;
        ui_title("WELCOME - PLAYER LOGIN");
        printf("  1) New player registration\n  2) Load existing player\n  0) Exit\n");
        if (!ui_read_int("  Choose > ", 0, 2, &c) || c == 0) return NULL;
        if (c == 1) {
            int err;
            Player *p;
            printf("  Enter player name (2-31 chars, starts with a letter; letters, digits, space, _ -) > ");
            if (!ui_read_line(line, sizeof line)) return NULL;
            printf("  Enter nickname (ENTER to reuse name) > ");
            if (!ui_read_line(nick, sizeof nick)) return NULL;
            p = player_register(db, line, nick, &err);
            if (!p) {
                if (err == 1) printf("%s  Invalid name.%s\n", ui_c(CLR_RED), ui_c(CLR_RESET));
                else if (err == 2) printf("%s  That name is already registered. Use 'Load existing player'.%s\n", ui_c(CLR_RED), ui_c(CLR_RESET));
                else printf("%s  Out of memory.%s\n", ui_c(CLR_RED), ui_c(CLR_RESET));
                continue;
            }
            if (storage_save(db) != 0) printf("%s  Warning: could not save the new profile.%s\n", ui_c(CLR_RED), ui_c(CLR_RESET));
            {
                char id[16]; player_format_id(p, id, sizeof id);
                printf("%s  Registration complete! Welcome, %s. Your player ID is %s.%s\n",
                       ui_c(CLR_GREEN), p->name, id, ui_c(CLR_RESET));
            }
            return p;
        } else {
            Player *p;
            if (db->nPlayers == 0) { printf("%s  No registered players yet. Please register first.%s\n", ui_c(CLR_RED), ui_c(CLR_RESET)); continue; }
            printf("  Enter your name or player ID (e.g. NA-0001) > ");
            if (!ui_read_line(line, sizeof line)) return NULL;
            p = player_lookup(db, line);
            if (!p) { printf("%s  Player not found.%s\n", ui_c(CLR_RED), ui_c(CLR_RESET)); continue; }
            printf("%s  Welcome back, %s!%s\n", ui_c(CLR_GREEN), p->name, ui_c(CLR_RESET));
            return p;
        }
    }
}

static void main_menu(Database *db, Player *p)
{
    for (;;) {
        int c;
        player_print_dashboard(p);
        ui_title("MAIN MENU");
        printf("   1. Start Championship\n   2. Quick Play\n   3. Time Attack\n   4. Daily Challenge\n"
               "   5. Leaderboard\n   6. Player Profile\n   7. Achievements\n   8. Game History\n"
               "   9. Instructions\n  10. Exit Game\n");
        if (!ui_read_int("  Select an option (1-10) > ", 1, 10, &c)) return;
        switch (c) {
            case 1: tournament_run(db, p); break;
            case 2: game_quick_play(db, p); break;
            case 3: game_time_attack(db, p); break;
            case 4: game_daily_challenge(db, p); break;
            case 5: leaderboard_show(db, p->id); break;
            case 6: player_print_profile(p, achievements_count(db, p->id)); break;
            case 7: achievements_show(db, p); break;
            case 8: storage_show_history(p); break;
            case 9: show_instructions(); break;
            case 10: return;
        }
        if (c != 1 && c != 2 && c != 3) ui_pause();
    }
}

int main(int argc, char **argv)
{
    Database db;
    Player *p;
    int i;
    unsigned seed = (unsigned)time(NULL);

    ui_init();
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--no-color") == 0) ui_set_color(0);
        else if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc) seed = (unsigned)strtoul(argv[++i], NULL, 10);
    }
    srand(seed);                                /* seeded once; --seed makes runs repeatable for testing */

    if (storage_init() != 0) return 1;
    if (storage_load(&db) != 0) { printf("Fatal: out of memory while loading data.\n"); return 1; }

    ui_clear();
    ui_logo();
    printf("\n  Players loaded: %d   (data folder: %s/)\n\n", (int)db.nPlayers, DATA_DIR);

    p = login(&db);
    if (p) main_menu(&db, p);

    if (storage_save(&db) != 0) printf("%sWarning: final save failed.%s\n", ui_c(CLR_RED), ui_c(CLR_RESET));
    storage_free(&db);
    printf("\n  Thanks for playing NUMBER ARENA. Your progress has been saved. Goodbye!\n");
    return 0;
}
