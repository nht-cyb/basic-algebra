#include <limits.h>
#include <stdio.h>
#include "exponent.h"
#include "root.h"

Power power_make(Fraction base, long exponent) {
    Power p = { fraction_make(base.num, base.den), exponent };
    return p;
}

/* b^n for n >= 0 by repeated squaring, or -1 if it does not fit */
static int whole_power(long b, long n, long *out) {
    long result = 1;
    while (n > 0) {
        if (n & 1) {
            if (__builtin_mul_overflow(result, b, &result)) return -1;
        }
        n >>= 1;
        if (n > 0 && __builtin_mul_overflow(b, b, &b)) return -1;
    }
    *out = result;
    return 0;
}

int power_value(Power p, Fraction *out) {
    Fraction base = fraction_make(p.base.num, p.base.den);
    long n = p.exponent, num, den;

    if (base.num == 0 && n <= 0) {
        return -1;
    }
    if (n < 0) { /* x^-n = 1 / x^n: flip the base */
        if (n == LONG_MIN) return -1;
        base = fraction_make(base.den, base.num);
        n = -n;
    }
    /* (x/y)^n = x^n / y^n, and x^0 = 1 */
    if (whole_power(base.num, n, &num) != 0 || whole_power(base.den, n, &den) != 0) {
        return -1;
    }
    *out = fraction_make(num, den);
    return 0;
}

int power_multiply(Power a, Power b, Power *out) {
    long n;
    if (!fraction_equal(a.base, b.base) || __builtin_add_overflow(a.exponent, b.exponent, &n)) {
        return -1;
    }
    *out = power_make(a.base, n);
    return 0;
}

int power_divide(Power a, Power b, Power *out) {
    long n;
    if (!fraction_equal(a.base, b.base) || __builtin_sub_overflow(a.exponent, b.exponent, &n)) {
        return -1;
    }
    *out = power_make(a.base, n);
    return 0;
}

int power_of_power(Power a, long m, Power *out) {
    long n;
    if (__builtin_mul_overflow(a.exponent, m, &n)) {
        return -1;
    }
    *out = power_make(a.base, n);
    return 0;
}

int power_root(Fraction x, long n, Fraction *out) {
    long num, den;
    int r;

    x = fraction_make(x.num, x.den);
    r = root_exact(x.num, n, &num);
    if (r != 1) {
        return r;
    }
    r = root_exact(x.den, n, &den);
    if (r != 1) {
        return r;
    }
    *out = fraction_make(num, den);
    return 1;
}

int power_format(Power p, char *out, size_t size) {
    char base[48];
    int n;
    Fraction b = fraction_make(p.base.num, p.base.den);

    if (fraction_format(b, base, sizeof base) < 0) {
        return -1;
    }
    if (b.num < 0 || b.den != 1) { /* (-2)^6 is not -2^6 */
        n = snprintf(out, size, "(%s)^%ld", base, p.exponent);
    } else {
        n = snprintf(out, size, "%s^%ld", base, p.exponent);
    }
    return n < 0 || (size_t)n >= size ? -1 : n;
}
