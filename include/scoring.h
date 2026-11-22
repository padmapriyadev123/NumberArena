/* scoring.h - Score and XP formulas (see README for the documented formula). */
#ifndef SCORING_H
#define SCORING_H

#define SCORE_MAX 1000000
#define XP_MAX    99999999
#define LOSS_XP   10

typedef struct {
    int base;            /* base mission points */
    int maxAttempts;
    int attemptsUsed;
    int unaided;         /* 1 = no lifeline and no extra hints */
    double seconds;      /* elapsed time */
    int timeLimit;       /* 0 = none */
    int lifelineUsed;
    int newStreak;       /* streak INCLUDING this win */
} ScoreInput;

typedef struct {
    int base, accuracy, speed, attemptsBonus, streakBonus, lifelinePenalty, total;
} ScoreBreakdown;

ScoreBreakdown scoring_compute(const ScoreInput *in);
int scoring_xp(int baseReward, int score, int won);
int scoring_add_xp(int current, int gain);   /* saturating add */
#endif
