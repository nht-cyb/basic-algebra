#ifndef EXPONENT_H
#define EXPONENT_H

#include <stddef.h>
#include "fraction.h"

/* base^exponent, e.g. 4^11 or (2/3)^3 */
typedef struct {
    Fraction base;
    long exponent;
} Power;

Power power_make(Fraction base, long exponent);

/* The value of p, using the properties of exponents:
     x^0 = 1                   (x not 0)
     x^-n = 1 / x^n
     (x/y)^n = x^n / y^n
   (-2)^6 = 64 and (-2)^7 = -128. For -2^6, which is -(2^6), negate the
   value of 2^6. Returns 0, or -1 for 0^0, 0^-n or if it does not fit. */
int power_value(Power p, Fraction *out);

/* The laws of exponents. Each returns 0, or -1 if the bases differ or
   the new exponent does not fit. */
int power_multiply(Power a, Power b, Power *out); /* x^n × x^m = x^(n + m) */
int power_divide(Power a, Power b, Power *out);   /* x^n ÷ x^m = x^(n - m) */
int power_of_power(Power a, long m, Power *out);  /* (x^n)^m = x^(n × m) */

/* x^(1/n), the nth root of x, when it is a fraction: 27^(1/3) = 3.
   Returns 1 if found, 0 if the root is irrational, -1 if not real. */
int power_root(Fraction x, long n, Fraction *out);

/* Writes "4^11", "3^-6", "(-2)^6" or "(2/3)^3".
   Returns the length, or -1 if it does not fit. */
int power_format(Power p, char *out, size_t size);

#endif
