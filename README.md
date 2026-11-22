# NUMBER ARENA - Ultimate Code Breaker Championship

A console esports-style number-cracking game written in **pure C (C11)**. Multiple game modes,
player profiles, XP and ranks, achievements, local leaderboard, daily challenge and permanent
file storage. Tested with GCC on Linux; the same code uses `#ifdef _WIN32` for MinGW-w64.

> Local game only. There is no online authentication and the leaderboard is **local** to this computer.

## 1. Project structure

```
NUMBER_ARENA/
  .vscode/tasks.json  .vscode/launch.json
  include/  ui.h player.h scoring.h hints.h game.h achievements.h storage.h leaderboard.h tournament.h
  src/      ui.c player.c scoring.c hints.c game.c achievements.c storage.c leaderboard.c tournament.c
  data/     players.dat leaderboard.dat achievements.dat history.dat   (created automatically)
  main.c  Makefile  README.md
```

| Module | Responsibility |
|---|---|
| `ui` | ANSI colours (auto-detected, `--no-color` fallback), logo, boxes, safe `fgets`-based input |
| `player` | Registration (unique name + ID), lookup, ranks, dashboard/profile |
| `scoring` | Score and XP formulas, clamping/saturation |
| `hints` | Higher/lower, hot/cold, odd/even, range hint, lifeline |
| `game` | Mission engine (one function runs every mode) + Quick Play, Time Attack, Daily |
| `tournament` | 3-round Championship with locking and retry |
| `achievements` | 6 achievements with unlock dates |
| `leaderboard` | Top 10 sort + snapshot file |
| `storage` | Binary load/save, corruption handling, history |

Dependency rule: `ui`, `scoring`, `hints` depend on nothing; `game`/`tournament` use `storage`; `storage` uses `leaderboard`.

## 2. Setup for beginners (VS Code + GCC)

### Windows
1. Install VS Code from https://code.visualstudio.com (tick "Add to PATH").
2. In VS Code open **Extensions** (Ctrl+Shift+X), search **C/C++** (Microsoft) and install it.
3. Install MinGW-w64 via MSYS2: download from https://www.msys2.org, install to `C:\msys64`, open **MSYS2 UCRT64** and run:
   `pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-gdb make`
4. Add `C:\msys64\ucrt64\bin` to PATH: Start menu -> "Edit the system environment variables" -> Environment Variables -> select **Path** -> Edit -> New -> paste the folder -> OK. **Close and reopen VS Code.**
5. Verify in the VS Code terminal (Ctrl+`): `gcc --version` and `gdb --version`.
6. Create the folder `NUMBER_ARENA`, then **File -> Open Folder** it. Create `include`, `src`, `data`, `.vscode` and copy in the files from this project.

### Linux (Debian/Ubuntu)
`sudo apt update && sudo apt install build-essential gdb` then `gcc --version`.

## 3. Compile and run (from the NUMBER_ARENA folder - the game uses the relative `data/` folder)

**Windows (PowerShell/VS Code terminal):**
```
gcc -std=c11 -Wall -Wextra -g -Iinclude main.c src/ui.c src/scoring.c src/hints.c src/player.c src/achievements.c src/leaderboard.c src/storage.c src/game.c src/tournament.c -o NUMBER_ARENA.exe
.\NUMBER_ARENA.exe
```
**Linux:**
```
gcc -std=c11 -Wall -Wextra -g -Iinclude main.c src/*.c -o NUMBER_ARENA
./NUMBER_ARENA
```
Or press **Ctrl+Shift+B** in VS Code (uses `tasks.json`), or run `make` / `make run`.
Options: `--no-color` (plain text), `--seed N` (repeatable random numbers for testing).

**Debugging:** set a breakpoint (click left of a line number), press **F5**, choose the Windows or Linux configuration. If gdb is elsewhere, edit `miDebuggerPath` in `launch.json`.

### Common problems
| Symptom | Fix |
|---|---|
| `'gcc' is not recognized` | PATH not set or VS Code not restarted (step 4) |
| `undefined reference to ...` | A `.c` file is missing from the compile command |
| `fatal error: xxx.h: No such file` | Missing `-Iinclude` or wrong working folder |
| `ERROR: cannot create/open 'data/...'` | Run from the NUMBER_ARENA folder, not from `src/` |
| Garbled `←[36m` characters | Old terminal: run with `--no-color` |

## 4. Rules and formulas

**Championship** (rounds unlock in order; a failed round keeps your progress so you can retry; Champion only after Round 3; then the run resets and `titles` increases):

| Round | Range | Attempts | Base XP/points |
|---|---|---|---|
| 1 Qualification | 1-50 | 10 | 100 |
| 2 Semi-Final | 1-200 | 7 | 200 |
| 3 Grand Finale | 1-1000 | 8 | 500 |

**Quick Play / Time Attack levels** (my choice of attempts, spec gave only ranges): Easy 1-50 (10 att, base 50), Medium 1-100 (8, 100), Hard 1-500 (10, 200), Expert 1-1000 (12, 300). Daily Challenge: 1-500, 9 attempts, base 150 + 150 bonus XP once per date.

**Score** (win only; a loss scores 0):
```
score = base + accuracy + speed + attemptsBonus + streakBonus - lifelinePenalty
accuracy        = base/4 when no lifeline and no extra hints were used
attemptsBonus   = 25 x (maxAttempts - attemptsUsed)
speed           = Time Attack: 5 x (timeLimit - seconds); other modes: 2 x (60 - seconds); clamped to 0..base
streakBonus     = 20 x min(winStreak including this win, 5)
lifelinePenalty = max(20, 30% of base)
score is clamped to 0..1,000,000
XP: win = baseReward + score/10, loss = 10 (XP clamped to 99,999,999)
```
**Hints:** after every wrong guess you get HIGHER/LOWER. `H` opens extra hints (max 2 per mission, free but they remove the accuracy bonus): Hot/Cold (vs your last guess), Odd/Even, Range (which half of the known range). `L` is the single lifeline: narrows to a window of ~25% of the known range, with a score penalty. No hint prints the secret.

**Ranks:** Rookie 0-499, Bronze 500-999, Silver 1000-1999, Gold 2000-3499, Platinum 3500-4999, Diamond 5000+.

**Achievements:** First Blood (first win) | Speed Master (win in <= 20 s) | Precision Expert (range >= 100, win using <= half the attempts) | Unstoppable (streak of 5) | Perfect Detective (range >= 500, no lifeline, no extra hints) | Arena Champion (finish the Championship).

**Leaderboard:** top 10 by XP desc, tie -> more wins, tie -> lower ID. `leaderboard.dat` is a snapshot rewritten on every save (`players.dat` is the source of truth).

**Time tracking limitation:** `fgets()` blocks, so a live countdown is impossible in a plain console. Elapsed time (`time()`, 1-second resolution) is checked before each prompt and again when ENTER is pressed; input submitted after the deadline is rejected. The "time left" shown is a snapshot, not a ticking clock.

**Other design decisions:** out-of-range or non-numeric guesses do not use an attempt; quitting (`Q`) counts as a loss; Daily Challenge may be retried until solved, the bonus is paid once; files are native binary structs (not portable between different compilers/platforms).

## 5. Robustness features
Bounded `fgets` input with overflow flushing and `strtol` validation; duplicate-name and duplicate-ID prevention; corrupted data files are renamed to `.bak` and the game continues with empty data; every `fopen/fwrite/fclose` is checked; EOF on stdin ends the game cleanly (no infinite loops); all heap memory is freed (checked with AddressSanitizer/UBSan - clean).

## 6. Real sample outputs (captured from the compiled program, `--no-color`)

**Startup**
```


 _   _ _   _ __  __ ____  _____ ____      _    ____  _____ _   _    _    
| \ | | | | |  \/  | __ )| ____|  _ \    / \  |  _ \| ____| \ | |  / \   
|  \| | | | | |\/| |  _ \|  _| | |_) |  / _ \ | |_) |  _| |  \| | / _ \  
| |\  | |_| | |  | | |_) | |___|  _ <  / ___ \|  _ <| |___| |\  |/ ___ \ 
|_| \_|\___/|_|  |_|____/|_____|_| \_\/_/   \_\_| \_\_____|_| \_/_/   \_\
      >>> ULTIMATE CODE BREAKER CHAMPIONSHIP <<<
```
**Registration**
```
  Registration complete! Welcome, Alice Ace. Your player ID is NA-0001.
+------------------------------------------------------------------+
| PLAYER DASHBOARD                                                 |
+------------------------------------------------------------------+
  Player          : Alice Ace  "Ace"  (NA-0001)
  Total XP        : 0
  Rank            : Rookie  [--------------------] 0/500 XP to next rank
  Streak          : 0 (best 0)   Championship: round 1 of 3 next
+------------------------------------------------------------------+
```
**Championship progress**
```
+------------------------------------------------------------------+
| CHAMPIONSHIP PROGRESS                                            |
+------------------------------------------------------------------+
  [NEXT   ] ROUND 1 - QUALIFICATION    range 1-50   10 attempts  100 XP
  [LOCKED ] ROUND 2 - SEMI-FINAL       range 1-200   7 attempts  200 XP
  [LOCKED ] ROUND 3 - GRAND FINALE     range 1-1000  8 attempts  500 XP
+------------------------------------------------------------------+
  1) Play Round 1   0) Back to menu > 1
```
**Hint and lifeline**
```
  HINT: the secret number is EVEN.
  LIFELINE: the secret is between 15 and 26.
```
**Victory**
```
  ****************************************
  *     V I C T O R Y  -  CODE CRACKED   *
  ****************************************
  Secret number : 24
  Attempts used : 5 of 8
  Time taken    : 0 s
  --- Score breakdown ---
  Base points        :    100
  Accuracy bonus     :      0
  Speed bonus        :    100
  Remaining attempts :     75
  Streak bonus       :     20
  Lifeline penalty   :    -30
  TOTAL SCORE        :    265
  XP gained: +126   Total XP: 126
  Streak: 1

+==================================================================+
|  *** ACHIEVEMENT UNLOCKED: First Blood                        ***|
```
**Defeat (time limit, Time Attack)**
```
[Attempt 1/10] Known range: 1-50 | Extra hints: 2 | Lifeline: ready | Time left (snapshot): 15s
  Guess, or H=hint, L=lifeline, Q=quit > 5
  Time limit exceeded - that input arrived after the deadline.

  ****************************************
  *      D E F E A T  -  MISSION FAILED  *
  ****************************************
  Time ran out!
  The secret number was 17.
```
**Achievement**
```
+==================================================================+
|  *** ACHIEVEMENT UNLOCKED: Precision Expert                   ***|
|  Win a mission (range 1-100 or bigger) using at most half of your attempts.|
+==================================================================+
```
**Final championship winner**
```

  ==============================================
   *  *  *   C H A M P I O N   *  *  *
   Alice Ace has conquered NUMBER ARENA!
  ==============================================
  Championship titles : 1
  Total XP            : 1081 (Silver)
  Score earned this session across rounds: 1360
```
**Daily Challenge**
```
  DAILY BONUS: +150 XP!  Total XP: 1422 (Silver)
  You already completed today's challenge (20261119). Come back tomorrow!
```
**Game history**
```
+------------------------------------------------------------------+
| GAME HISTORY                                                     |
+------------------------------------------------------------------+
  #   Date             Mode          Level         Secret Attempts  Sec   Score  Result
  ---------------------------------------------------------------------------------------
  1   2026-11-19 10:02 Quick Play    Medium        24     5         0     265    WIN
  2   2026-11-19 10:02 Quick Play    Hard          398    0         0     0      LOSS
  3   2026-11-19 10:02 Championship  Beginner      7      5         0     370    WIN
  4   2026-11-19 10:02 Championship  Intermediate  120    7         0     0      LOSS
  5   2026-11-19 10:02 Championship  Intermediate  91     2         0     405    WIN
  6   2026-11-19 10:02 Championship  Expert        161    5         0     585    WIN
  7   2026-11-19 10:02 Daily         Daily         152    7         0     417    WIN
  8   2026-11-19 10:02 Time Attack   Easy          14     6         0     292    WIN
  9   2026-11-19 10:02 Time Attack   Easy          17     0         16    0      LOSS
```
**Leaderboard**
```
+------------------------------------------------------------------+
| LOCAL LEADERBOARD - TOP 10                                       |
+------------------------------------------------------------------+
  #   Player           ID       XP      Rank      Games  Won   Win%
  ------------------------------------------------------------------
  1   Alice Ace        NA-0001  1511    Silver    9      6     66.7%

  Ties: more wins first, then the lower (older) player ID.
  This is a LOCAL leaderboard stored on this computer only.
```

## 7. Test results (executed with a scripted player)
| # | Test | Expected | Result |
|---|---|---|---|
| 1 | Register "Alice Ace" | ID NA-0001, saved | PASS |
| 2 | Name "9bad" / duplicate "alice ace" | Rejected | PASS |
| 3 | Load existing by ID after restart | XP/streak restored | PASS |
| 4 | Menu input `abc`, `99`, empty | Error, re-prompt, no crash | PASS |
| 5 | Guess `101` (max 100) / `xyz` | "Out of range"/"Invalid", no attempt used | PASS |
| 6 | Wrong guess | HIGHER/LOWER, known range narrows | PASS |
| 7 | Attempt limit (7 x guess 1 in Round 2) | Defeat, +10 XP, round not cleared | PASS |
| 8 | Failed championship round | Progress kept, retry allowed | PASS |
| 9 | Championship R1 -> R2 -> R3 | Champion screen, title +1, Arena Champion | PASS |
| 10 | Score check (Medium, 5 attempts, lifeline, 0 s) | 100+0+100+75+20-30 = 265 | PASS |
| 11 | Time Attack, 15 s limit, input after 16 s | Defeat "Time ran out" | PASS |
| 12 | Daily Challenge twice | Second attempt blocked | PASS |
| 13 | Achievements unlock + persist | Saved with date | PASS |
| 14 | Leaderboard sorting / ties | XP, wins, ID | PASS (code path; single-player run) |
| 15 | Delete data folder / truncate `players.dat` | Recreated / `.bak` + warning | PASS |
| 16 | EOF mid-mission, 5000-char input lines | Clean exit, no hang | PASS |

## 8. Future enhancements (NOT implemented)
GUI, multiplayer, online leaderboard, database, sound, AI-generated clues, portable (text/JSON) save format.
