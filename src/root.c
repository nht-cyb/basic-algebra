#include <math.h>
#include "root.h"

typedef unsigned __int128 u128;

/* base^n, or -1 if it does not fit in a long */
static int checked_power(long base, long n, long *out) {
    long result = 1;
    for (long i = 0; i < n; i++) {
        if (__builtin_mul_overflow(result, base, &result)) {
            return -1;
        }
    }
    *out = result;
    return 0;
}

int root_exact(long x, long n, long *root) {
    long magnitude, guess, p;
    int negative = x < 0;

    if (n < 1 || (negative && n % 2 == 0)) {
        return -1;
    }
    if (x == 0 || x == 1 || n == 1) {
        *root = x;
        return 1;
    }
    magnitude = negative ? -x : x;
    if (magnitude < 0) { /* LONG_MIN */
        return 0;
    }
    /* pow() is only approximate, so check the whole numbers around it */
    guess = (long)llround(pow((double)magnitude, 1.0 / (double)n));
    for (long r = guess > 2 ? guess - 1 : 2; r <= guess + 1; r++) { /* 1 was handled above */
        if (checked_power(r, n, &p) == 0 && p == magnitude) {
            *root = negative ? -r : r;
            return 1;
        }
    }
    return 0;
}

void root_simplify(long n, long *outside, long *inside) {
    long out = 1, r;

    if (root_exact(n, 2, &r) == 1) { /* a perfect square, even a big one */
        *outside = r;
        *inside = 1;
        return;
    }
    for (long p = 2; p <= 1000000 && p * p <= n; p++) {
        while (n % (p * p) == 0) {
            n /= p * p;
            out *= p;
        }
    }
    *outside = out;
    *inside = n;
}

/* Writes the digits of v, with a decimal point before the last
   `decimals` digits. */
static int write_fixed(u128 v, int decimals, char *out, size_t size) {
    char digits[48];
    int count = 0;
    size_t len = 0;

    do {
        digits[count++] = (char)('0' + (int)(v % 10));
        v /= 10;
    } while (v > 0 || count <= decimals); /* at least one digit before the point */

    for (int i = count - 1; i >= 0; i--) {
        if (len + 2 >= size) {
            return -1;
        }
        out[len++] = digits[i];
        if (i == decimals && decimals > 0) {
            out[len++] = '.';
        }
    }
    out[len] = '\0';
    return (int)len;
}

int square_root(unsigned long n, int decimals, int round, char *out, size_t size) {
    int pairs[16]; /* the digits of n in pairs, from the right */
    int count = 0;
    int extra = round ? 1 : 0; /* one more digit to round with */
    u128 root = 0, remainder = 0;

    if (decimals < 0 || decimals > 25 || size == 0) {
        return -1;
    }

    do {
        pairs[count++] = (int)(n % 100);
        n /= 100;
    } while (n > 0);

    /* Each step: bring down the next pair (00 after the decimal point),
       then find the largest digit z with (20 * root + z) * z <= remainder.
       20 * root is "double the number on top" with a digit place for z. */
    for (int step = 0; step < count + decimals + extra; step++) {
        int pair = step < count ? pairs[count - 1 - step] : 0;
        u128 z = 9;

        remainder = remainder * 100 + (u128)pair;
        while ((20 * root + z) * z > remainder) {
            z--;
        }
        remainder -= (20 * root + z) * z;
        root = root * 10 + z;
    }

    if (round) {
        root = (root + 5) / 10;
    }
    return write_fixed(root, decimals, out, size);
}

void square_root_between(unsigned long n, unsigned long *low, unsigned long *high) {
    unsigned long r = (unsigned long)sqrtl((long double)n);

    /* sqrtl() can be off by one for large n */
    while ((u128)r * r > n) r--;
    while ((u128)(r + 1) * (r + 1) <= n) r++;

    *low = r;
    *high = (u128)r * r == n ? r : r + 1;
}

double square_root_estimate(unsigned long n) {
    unsigned long low, high;
    double r;

    square_root_between(n, &low, &high);
    /* pick the closer perfect square: 45 is closer to 49 than to 36 */
    if ((u128)high * high - n < n - (u128)low * low) {
        r = (double)high;
    } else {
        r = (double)low;
    }
    return r - (r * r - (double)n) / (2 * r);
}
