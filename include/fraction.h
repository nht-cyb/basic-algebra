#ifndef FRACTION_H
#define FRACTION_H

#include <stddef.h>

typedef struct {
    long num; /* numerator */
    long den; /* denominator, never 0 */
} Fraction;

/* num/den in lowest terms, with the sign on the numerator. */
Fraction fraction_make(long num, long den);
Fraction fraction_integer(long n);
int fraction_equal(Fraction a, Fraction b);

/* These return 0, or -1 if the result does not fit in a long
   (or, for divide, if y is 0). */
int fraction_add(Fraction x, Fraction y, Fraction *out);
int fraction_multiply(Fraction x, Fraction y, Fraction *out);
int fraction_divide(Fraction x, Fraction y, Fraction *out);

/* Writes "2/3", "-4" or "0". Returns the length, or -1 if it does not fit. */
int fraction_format(Fraction f, char *out, size_t size);

#endif
