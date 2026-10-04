#include <stdio.h>
#include "fraction.h"

static long gcd(long a, long b) {
    if (a < 0) a = -a;
    if (b < 0) b = -b;
    while (b != 0) {
        long t = a % b;
        a = b;
        b = t;
    }
    return a;
}

Fraction fraction_make(long num, long den) {
    Fraction f = { num, den };
    long g = gcd(num, den);
    if (g != 0) {
        f.num /= g;
        f.den /= g;
    }
    if (f.den < 0) { /* keep the sign on the numerator */
        f.num = -f.num;
        f.den = -f.den;
    }
    return f;
}

Fraction fraction_integer(long n) {
    Fraction f = { n, 1 };
    return f;
}

int fraction_equal(Fraction a, Fraction b) {
    a = fraction_make(a.num, a.den);
    b = fraction_make(b.num, b.den);
    return a.num == b.num && a.den == b.den;
}

/* a/b + c/d = (ad + bc) / bd */
int fraction_add(Fraction x, Fraction y, Fraction *out) {
    long ad, bc, num, den;
    if (__builtin_mul_overflow(x.num, y.den, &ad) ||
        __builtin_mul_overflow(y.num, x.den, &bc) ||
        __builtin_add_overflow(ad, bc, &num) ||
        __builtin_mul_overflow(x.den, y.den, &den)) {
        return -1;
    }
    *out = fraction_make(num, den);
    return 0;
}

/* a/b * c/d = ac / bd, cancelling first so it overflows less often */
int fraction_multiply(Fraction x, Fraction y, Fraction *out) {
    long num, den;
    x = fraction_make(x.num, x.den);
    y = fraction_make(y.num, y.den);
    long g1 = gcd(x.num, y.den), g2 = gcd(y.num, x.den);
    if (g1 == 0) g1 = 1;
    if (g2 == 0) g2 = 1;
    if (__builtin_mul_overflow(x.num / g1, y.num / g2, &num) ||
        __builtin_mul_overflow(x.den / g2, y.den / g1, &den)) {
        return -1;
    }
    *out = fraction_make(num, den);
    return 0;
}

/* a/b / c/d = a/b * d/c */
int fraction_divide(Fraction x, Fraction y, Fraction *out) {
    Fraction flipped = { y.den, y.num };
    if (y.num == 0) {
        return -1;
    }
    return fraction_multiply(x, flipped, out);
}

int fraction_format(Fraction f, char *out, size_t size) {
    int n;
    f = fraction_make(f.num, f.den);
    if (f.den == 1) {
        n = snprintf(out, size, "%ld", f.num);
    } else {
        n = snprintf(out, size, "%ld/%ld", f.num, f.den);
    }
    return n < 0 || (size_t)n >= size ? -1 : n;
}
