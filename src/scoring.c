/* scoring.c - Scoring and XP formulas.
 *
 * score = base + accuracy + speed + attemptsBonus + streakBonus - lifelinePenalty
 *   accuracy        = base/4 if no lifeline and no extra hints were used
 *   attemptsBonus   = 25 * (maxAttempts - attemptsUsed)
 *   speed           = Time Attack: 5 * (timeLimit - seconds)   clamped to 0..base
 *                     other modes: 2 * (60 - seconds)          clamped to 0..base
 *   streakBonus     = 20 * min(newStreak, 5)
 *   lifelinePenalty = max(20, 30% of base) when the lifeline was used
 * The total is clamped to 0..SCORE_MAX.  A lost mission scores 0.
 * XP: win = baseReward + score/10, loss = LOSS_XP.  XP is clamped to XP_MAX.
 */
#include "scoring.h"

static int clamp_int(long long v, int lo, int hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return (int)v;
}

ScoreBreakdown scoring_compute(const ScoreInput *in)
{
    ScoreBreakdown sb = {0, 0, 0, 0, 0, 0, 0};
    long long total;
    long long rawSpeed;
    int remaining = in->maxAttempts - in->attemptsUsed;

    sb.base = clamp_int(in->base, 0, SCORE_MAX);
    sb.accuracy = in->unaided ? sb.base / 4 : 0;
    sb.attemptsBonus = remaining > 0 ? remaining * 25 : 0;

    if (in->timeLimit > 0) rawSpeed = (long long)((in->timeLimit - in->seconds) * 5.0);
    else                   rawSpeed = (long long)((60.0 - in->seconds) * 2.0);
    sb.speed = clamp_int(rawSpeed, 0, sb.base);

    sb.streakBonus = 20 * clamp_int(in->newStreak, 0, 5);

    if (in->lifelineUsed) {
        sb.lifelinePenalty = sb.base * 30 / 100;
        if (sb.lifelinePenalty < 20) sb.lifelinePenalty = 20;
    }
    total = (long long)sb.base + sb.accuracy + sb.speed + sb.attemptsBonus + sb.streakBonus
            - sb.lifelinePenalty;
    sb.total = clamp_int(total, 0, SCORE_MAX);
    return sb;
}

int scoring_xp(int baseReward, int score, int won)
{
    if (!won) return LOSS_XP;
    return clamp_int((long long)baseReward + score / 10, 0, XP_MAX);
}

int scoring_add_xp(int current, int gain)
{
    return clamp_int((long long)current + gain, 0, XP_MAX);
}
