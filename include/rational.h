#ifndef RATIONAL_H
#define RATIONAL_H

#include "fraction.h"

typedef enum {
    NUMBER_INVALID = -1, /* not understood, divides by 0, or not real */
    NUMBER_IRRATIONAL,
    NUMBER_RATIONAL
} NumberKind;

/* Says whether a written number is rational, i.e. can be written as a/b
   with a and b integers and b not 0. It understands:
     integers and fractions   2, 0, -8/2, 2/3
     terminating decimals     0.75, 0.150
     repeating decimals       0.(21) for 0.212121..., 15.(8451)
     radicals                 √4, √2, 3√125 (cube root), 5√325
     famous constants         pi, π, e, phi, ϕ, golden ratio
   A decimal written without parentheses ends where it is written, so
   "1.4142135" is the rational number 14142135/10000000.
   When the number is rational, *value is set to it as a fraction, or to
   0/0 if it does not fit in a long. value may be NULL. */
NumberKind classify_number(const char *text, Fraction *value);

#endif
