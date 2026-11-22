/* hints.h - Intelligent hint system. No hint ever prints the secret itself. */
#ifndef HINTS_H
#define HINTS_H
const char *hint_higher_lower(int secret, int guess);          /* "HIGHER" or "LOWER" */
const char *hint_hot_cold(int secret, int guess, int min, int max);
const char *hint_odd_even(int secret);
void hint_range(int secret, int lo, int hi, int *outLo, int *outHi);     /* half containing secret */
void hint_lifeline(int secret, int lo, int hi, int *outLo, int *outHi);  /* ~25% window containing secret */
#endif
