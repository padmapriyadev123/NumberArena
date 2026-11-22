/* hints.c - Hint generators.  Hints narrow or characterise the secret but never reveal it. */
#include <stdlib.h>
#include "hints.h"

const char *hint_higher_lower(int secret, int guess)
{
    return secret > guess ? "HIGHER" : "LOWER";
}

const char *hint_hot_cold(int secret, int guess, int min, int max)
{
    int d = abs(secret - guess);
    int span = max - min + 1;
    int pct = span > 0 ? (int)((long long)d * 100 / span) : 100;
    if (pct <= 2)  return "BURNING HOT";
    if (pct <= 5)  return "HOT";
    if (pct <= 15) return "WARM";
    if (pct <= 30) return "COLD";
    return "FREEZING";
}

const char *hint_odd_even(int secret)
{
    return (secret % 2 == 0) ? "EVEN" : "ODD";
}

void hint_range(int secret, int lo, int hi, int *outLo, int *outHi)
{
    int mid = lo + (hi - lo) / 2;
    if (secret <= mid) { *outLo = lo;      *outHi = mid; }
    else               { *outLo = mid + 1; *outHi = hi;  }
}

/* Lifeline: a window of about a quarter of the remaining range that contains the
 * secret at a random position (so the secret is not always in the middle). */
void hint_lifeline(int secret, int lo, int hi, int *outLo, int *outHi)
{
    int size = hi - lo + 1;
    int w = size / 4;
    int smin, smax, start;
    if (w < 3) w = 3;
    if (size <= w) { *outLo = lo; *outHi = hi; return; }
    smin = secret - w + 1; if (smin < lo) smin = lo;
    smax = secret;         if (smax > hi - w + 1) smax = hi - w + 1;
    start = smin + rand() % (smax - smin + 1);
    *outLo = start;
    *outHi = start + w - 1;
}
