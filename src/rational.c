#include <ctype.h>
#include <string.h>
#include "rational.h"
#include "root.h"

static const char *irrational_constants[] = {
    "pi", "π", "e", "phi", "ϕ", "φ", "golden ratio"
};

static const char *skip_spaces(const char *s) {
    while (isspace((unsigned char)*s)) s++;
    return s;
}

/* Reads all the digits, into *value. Returns how many were read, or -1
   if the number does not fit in a long. */
static int read_digits(const char **s, long *value) {
    int count = 0, fits = 1;
    *value = 0;
    while (isdigit((unsigned char)**s)) {
        if (__builtin_mul_overflow(*value, 10, value) ||
            __builtin_add_overflow(*value, **s - '0', value)) {
            fits = 0;
        }
        (*s)++;
        count++;
    }
    return fits ? count : -1;
}

/* 10^n, or -1 if it does not fit */
static long power_of_ten(int n) {
    long p = 1;
    while (n-- > 0) {
        if (__builtin_mul_overflow(p, 10, &p)) return -1;
    }
    return p;
}

/* Reads [-]whole[.fixed][(repeat)] as a fraction:
     whole + fixed / 10^k + repeat / (10^k * (10^r - 1))
   where k and r are the number of digits in fixed and repeat, so
   0.(21) = 21/99 = 7/33. Returns 1 if read, 0 if not a decimal, and -1
   if it is a decimal that does not fit in a long. */
static int read_decimal(const char **s, Fraction *out) {
    const char *p = *s;
    long whole, fixed = 0, repeat = 0, ten_k = 1, ten_r;
    int negative = 0, k = 0, r, fits = 1;
    Fraction value, part;

    if (*p == '-') {
        negative = 1;
        p++;
    }
    if (!isdigit((unsigned char)*p)) {
        return 0;
    }
    if (read_digits(&p, &whole) < 0) fits = 0;
    value = fraction_integer(whole);

    if (*p == '.' && (isdigit((unsigned char)p[1]) || p[1] == '(')) {
        p++;
        k = read_digits(&p, &fixed);
        ten_k = power_of_ten(k);
        if (k < 0 || ten_k < 0) {
            fits = 0;
        } else if (fits && fraction_add(value, fraction_make(fixed, ten_k), &value) != 0) {
            fits = 0;
        }

        if (*p == '(') {
            p++;
            r = read_digits(&p, &repeat);
            if (r == 0 || *p != ')') {
                return 0;
            }
            p++;
            ten_r = power_of_ten(r);
            if (r < 0 || ten_r < 0 || !fits ||
                fraction_divide(fraction_make(repeat, ten_r - 1), fraction_integer(ten_k), &part) != 0 ||
                fraction_add(value, part, &value) != 0) {
                fits = 0;
            }
        }
    }

    *s = p;
    if (!fits) {
        return -1;
    }
    *out = negative ? fraction_make(-value.num, value.den) : value;
    return 1;
}

/* [index]√radicand or sqrt(radicand) */
static NumberKind classify_radical(const char *s, Fraction *value) {
    long index = 2, radicand, root;
    int negative = 0, r;

    if (isdigit((unsigned char)*s)) {
        if (read_digits(&s, &index) < 0) return NUMBER_INVALID;
    }
    if (strncmp(s, "√", strlen("√")) == 0) {
        s += strlen("√");
    } else if (index == 2 && strncmp(s, "sqrt(", 5) == 0) {
        s += 5;
    } else {
        return NUMBER_INVALID;
    }
    s = skip_spaces(s);
    if (*s == '-') {
        negative = 1;
        s++;
    }
    if (read_digits(&s, &radicand) <= 0) {
        return NUMBER_INVALID;
    }
    if (*s == ')') s++;
    if (*skip_spaces(s) != '\0') {
        return NUMBER_INVALID;
    }

    /* The root of a perfect square (or cube, ...) is a whole number.
       Any other root of a whole number is irrational. */
    r = root_exact(negative ? -radicand : radicand, index, &root);
    if (r < 0) {
        return NUMBER_INVALID;
    }
    if (r == 0) {
        return NUMBER_IRRATIONAL;
    }
    *value = fraction_integer(root);
    return NUMBER_RATIONAL;
}

NumberKind classify_number(const char *text, Fraction *value) {
    Fraction ignored, top, bottom;
    const char *s = skip_spaces(text);
    size_t len;
    int r;

    if (value == NULL) {
        value = &ignored;
    }

    len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) len--;
    for (size_t i = 0; i < sizeof irrational_constants / sizeof irrational_constants[0]; i++) {
        if (strlen(irrational_constants[i]) == len && strncmp(s, irrational_constants[i], len) == 0) {
            return NUMBER_IRRATIONAL;
        }
    }

    if (strstr(s, "√") != NULL || strncmp(s, "sqrt(", 5) == 0) {
        return classify_radical(s, value);
    }

    /* a or a/b, where a and b are decimals */
    r = read_decimal(&s, &top);
    if (r == 0) {
        return NUMBER_INVALID;
    }
    s = skip_spaces(s);
    if (*s == '/') {
        int r2;
        s = skip_spaces(s + 1);
        r2 = read_decimal(&s, &bottom);
        if (r2 == 0 || (r2 == 1 && bottom.num == 0)) {
            return NUMBER_INVALID;
        }
        if (r < 0 || r2 < 0 || fraction_divide(top, bottom, &top) != 0) {
            r = -1;
        }
    }
    if (*skip_spaces(s) != '\0') {
        return NUMBER_INVALID;
    }
    if (r < 0) {
        value->num = 0;
        value->den = 0;
    } else {
        *value = top;
    }
    return NUMBER_RATIONAL;
}
