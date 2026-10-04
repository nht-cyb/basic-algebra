#ifndef ROOT_H
#define ROOT_H

#include <stddef.h>

/* The nth root of x when it is a whole number: root_exact(125, 3, &r)
   gives r = 5. Returns 1 if exact, 0 if the root is not a whole number,
   and -1 if there is no real root (n < 1, or x < 0 with n even). */
int root_exact(long x, long n, long *root);

/* The principal square root of n using the square root algorithm:
   group the digits in pairs, then find each digit of the root in turn.
   Writes the root with the given number of decimals, e.g.
   square_root(2685, 2, 0, ...) writes "51.81". If round is nonzero the
   last decimal is rounded (51.82), otherwise digits are cut off, as the
   algorithm does by hand. Returns the length, or -1 if decimals is
   more than 25 or the text does not fit in size. */
int square_root(unsigned long n, int decimals, int round, char *out, size_t size);

/* The whole numbers around the square root: low <= sqrt(n) <= high,
   e.g. 4 < sqrt(17) < 5. For a perfect square, low == high. */
void square_root_between(unsigned long n, unsigned long *low, unsigned long *high);

/* Estimates sqrt(n) from the closest perfect square r*r:
   sqrt(n) is about r - (r*r - n) / (2r).
   sqrt(45): r = 7, 7 - 4/14 = 6.714. */
double square_root_estimate(unsigned long n);

#endif
