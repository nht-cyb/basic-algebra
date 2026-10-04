#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "linear.h"

/* ---- Each side of the equation, simplified to ax + b ---- */

typedef struct {
    Fraction a; /* coefficient of the variable */
    Fraction b; /* constant */
} Side;

typedef struct {
    const char *s;
    char variable; /* 0 until a letter is seen */
    const char *error;
} Parser;

static const Side zero_side = { { 0, 1 }, { 0, 1 } };

static Side constant(Fraction b) {
    Side x = { { 0, 1 }, b };
    return x;
}

static void fail(Parser *p, const char *error) {
    if (p->error == NULL) {
        p->error = error;
    }
}

static Fraction negate(Fraction f) {
    return fraction_make(-f.num, f.den);
}

static Side add(Parser *p, Side x, Side y) {
    if (fraction_add(x.a, y.a, &x.a) != 0 || fraction_add(x.b, y.b, &x.b) != 0) {
        fail(p, "the numbers are too big");
    }
    return x;
}

static Side subtract(Parser *p, Side x, Side y) {
    y.a = negate(y.a);
    y.b = negate(y.b);
    return add(p, x, y);
}

/* (ax + b)(cx + d) is only linear when a or c is 0 */
static Side multiply(Parser *p, Side x, Side y) {
    Side r = zero_side, t;
    if (x.a.num != 0 && y.a.num != 0) {
        fail(p, "not linear: the variable is multiplied by itself");
        return r;
    }
    if (x.a.num != 0) { /* swap so that y is the plain number */
        t = x;
        x = y;
        y = t;
    }
    /* b(cx + d) = bcx + bd: the distributive property */
    if (fraction_multiply(x.b, y.a, &r.a) != 0 || fraction_multiply(x.b, y.b, &r.b) != 0) {
        fail(p, "the numbers are too big");
    }
    return r;
}

static Side divide(Parser *p, Side x, Side y) {
    Side r = zero_side;
    if (y.a.num != 0) {
        fail(p, "not linear: it divides by the variable");
    } else if (y.b.num == 0) {
        fail(p, "it divides by 0");
    } else if (fraction_divide(x.a, y.b, &r.a) != 0 || fraction_divide(x.b, y.b, &r.b) != 0) {
        fail(p, "the numbers are too big");
    }
    return r;
}

/* ---- Reading the equation ---- */

static void skip_spaces(Parser *p) {
    while (isspace((unsigned char)*p->s)) p->s++;
}

/* Consumes one of the given symbols if it comes next. */
static int accept(Parser *p, const char *const *symbols) {
    skip_spaces(p);
    for (; *symbols != NULL; symbols++) {
        size_t n = strlen(*symbols);
        if (strncmp(p->s, *symbols, n) == 0) {
            p->s += n;
            return 1;
        }
    }
    return 0;
}

static const char *const plus_signs[] = { "+", NULL };
static const char *const minus_signs[] = { "-", "−", NULL };
static const char *const times_signs[] = { "*", "×", "·", NULL };
static const char *const divide_signs[] = { "/", "÷", NULL };
static const char *const open_bracket[] = { "(", NULL };
static const char *const close_bracket[] = { ")", NULL };

static Side parse_expr(Parser *p);

/* digits[.digits] as an exact fraction: 3.1 = 31/10 */
static Side parse_number(Parser *p) {
    Fraction value = { 0, 1 }, digit;
    long scale = 1;
    int after_point = 0;

    for (; isdigit((unsigned char)*p->s) || (*p->s == '.' && !after_point); p->s++) {
        if (*p->s == '.') {
            after_point = 1;
            continue;
        }
        digit = fraction_integer(*p->s - '0');
        if (fraction_multiply(value, fraction_integer(10), &value) != 0 ||
            fraction_add(value, digit, &value) != 0 ||
            (after_point && __builtin_mul_overflow(scale, 10, &scale))) {
            fail(p, "the numbers are too big");
        }
    }
    return constant(fraction_make(value.num, value.den * scale));
}

/* a number, the variable, a bracket, or a sign before one of these */
static Side parse_factor(Parser *p) {
    Side x = zero_side;

    if (accept(p, minus_signs)) {
        return subtract(p, zero_side, parse_factor(p));
    }
    if (accept(p, plus_signs)) {
        return parse_factor(p);
    }
    if (accept(p, open_bracket)) {
        x = parse_expr(p);
        if (!accept(p, close_bracket)) {
            fail(p, "a bracket is not closed");
        }
        return x;
    }
    skip_spaces(p);
    if (isdigit((unsigned char)*p->s) || (*p->s == '.' && isdigit((unsigned char)p->s[1]))) {
        return parse_number(p);
    }
    if (isalpha((unsigned char)*p->s)) {
        if (p->variable == 0) {
            p->variable = *p->s;
        } else if (p->variable != *p->s) {
            fail(p, "there is more than one variable");
        }
        p->s++;
        x.a = fraction_integer(1);
        return x;
    }
    fail(p, *p->s == '\0' || *p->s == '=' ? "a number or variable is missing"
                                          : "it has a symbol that is not understood");
    return x;
}

/* factors joined by × or ÷, or written side by side: 2x, 2(x - 4) */
static Side parse_term(Parser *p) {
    Side x = parse_factor(p);
    while (p->error == NULL) {
        if (accept(p, times_signs)) {
            x = multiply(p, x, parse_factor(p));
        } else if (accept(p, divide_signs)) {
            x = divide(p, x, parse_factor(p));
        } else if (skip_spaces(p), isalpha((unsigned char)*p->s) || *p->s == '(') {
            x = multiply(p, x, parse_factor(p));
        } else {
            break;
        }
    }
    return x;
}

/* terms joined by + or - */
static Side parse_expr(Parser *p) {
    Side x = parse_term(p);
    while (p->error == NULL) {
        if (accept(p, plus_signs)) {
            x = add(p, x, parse_term(p));
        } else if (accept(p, minus_signs)) {
            x = subtract(p, x, parse_term(p));
        } else {
            break;
        }
    }
    return x;
}

/* ---- Writing the steps ---- */

static int is_zero(Fraction f) { return f.num == 0; }
static int is_one(Fraction f) { return f.num == f.den; }

/* "x", "-x", "4x", "(2/5)x", "-(2/5)x" */
static void format_term(Fraction a, char variable, char *out, size_t size) {
    char num[48];
    a = fraction_make(a.num, a.den);
    if (a.num == a.den) {
        snprintf(out, size, "%c", variable);
    } else if (a.num == -a.den) {
        snprintf(out, size, "-%c", variable);
    } else if (a.den == 1) {
        snprintf(out, size, "%ld%c", a.num, variable);
    } else {
        fraction_format(fraction_make(a.num < 0 ? -a.num : a.num, a.den), num, sizeof num);
        snprintf(out, size, "%s(%s)%c", a.num < 0 ? "-" : "", num, variable);
    }
}

/* "4x - 12", "x", "20", "0" */
static void format_side(Side x, char variable, char *out, size_t size) {
    char term[64], num[48];
    if (is_zero(x.a)) {
        fraction_format(x.b, out, size);
        return;
    }
    format_term(x.a, variable, term, sizeof term);
    if (is_zero(x.b)) {
        snprintf(out, size, "%s", term);
        return;
    }
    fraction_format(x.b.num < 0 ? negate(x.b) : x.b, num, sizeof num);
    snprintf(out, size, "%s %c %s", term, x.b.num < 0 ? '-' : '+', num);
}

static void add_step(LinearSolution *out, Side left, Side right, const char *action) {
    LinearStep *step;
    char l[62], r[62]; /* so "l = r" always fits in step->equation */
    if (out->step_count == LINEAR_MAX_STEPS) {
        return;
    }
    step = &out->steps[out->step_count++];
    format_side(left, out->variable, l, sizeof l);
    format_side(right, out->variable, r, sizeof r);
    snprintf(step->equation, sizeof step->equation, "%s = %s", l, r);
    snprintf(step->action, sizeof step->action, "%s", action);
}

static int same_ignoring_spaces(const char *a, const char *b) {
    for (;;) {
        while (isspace((unsigned char)*a)) a++;
        while (isspace((unsigned char)*b)) b++;
        if (*a != *b) return 0;
        if (*a == '\0') return 1;
        a++;
        b++;
    }
}

/* ---- Solving ---- */

static LinearKind invalid(LinearSolution *out, const char *error) {
    out->kind = LINEAR_INVALID;
    out->error = error;
    return out->kind;
}

LinearKind linear_solve(const char *equation, LinearSolution *out) {
    Parser p = { equation, 0, NULL };
    Side left, right = zero_side, t;
    char action[96], term[64], num[48];
    Fraction a;
    size_t len;

    memset(out, 0, sizeof *out);

    left = parse_expr(&p);
    skip_spaces(&p);
    if (p.error == NULL && *p.s != '=') {
        fail(&p, *p.s == '\0' ? "there is no = sign" : "it has a symbol that is not understood");
    }
    if (p.error == NULL) {
        p.s++;
        right = parse_expr(&p);
        skip_spaces(&p);
        if (p.error == NULL && *p.s != '\0') {
            fail(&p, *p.s == '=' ? "there is more than one = sign" : "it has a symbol that is not understood");
        }
    }
    if (p.error != NULL) {
        return invalid(out, p.error);
    }
    if (p.variable == 0) {
        return invalid(out, "there is no variable to solve for");
    }
    out->variable = p.variable;

    /* the equation as given */
    while (isspace((unsigned char)*equation)) equation++;
    len = strlen(equation);
    while (len > 0 && isspace((unsigned char)equation[len - 1])) len--;
    snprintf(out->steps[0].equation, sizeof out->steps[0].equation, "%.*s", (int)len, equation);
    out->step_count = 1;

    /* 6(x - 2) = 2(9 - 2x) becomes 6x - 12 = -4x + 18 */
    add_step(out, left, right, strchr(equation, '(') ? "Use the distributive property" : "Write each side as ax + b");
    if (same_ignoring_spaces(out->steps[1].equation, out->steps[0].equation)) {
        out->step_count = 1;
    }

    /* 60 = 2n + 4 becomes 2n + 4 = 60, so the variable is on the left */
    if (is_zero(left.a) && !is_zero(right.a)) {
        t = left;
        left = right;
        right = t;
        add_step(out, left, right, "Swap the two sides");
    }

    /* 9x - 12 = 5x + 8: subtract 5x from each side */
    if (!is_zero(right.a)) {
        format_term(right.a.num < 0 ? negate(right.a) : right.a, out->variable, term, sizeof term);
        snprintf(action, sizeof action, "%s %s %s each side",
                 right.a.num < 0 ? "Add" : "Subtract", term, right.a.num < 0 ? "to" : "from");
        if (fraction_add(left.a, negate(right.a), &left.a) != 0) {
            return invalid(out, "the numbers are too big");
        }
        right.a = fraction_integer(0);
        add_step(out, left, right, action);
    }

    if (is_zero(left.a)) { /* the variable cancelled out */
        out->kind = fraction_equal(left.b, right.b) ? LINEAR_ALL_NUMBERS : LINEAR_NO_SOLUTION;
        return out->kind;
    }

    /* 4x - 12 = 8: add 12 to each side */
    if (!is_zero(left.b)) {
        fraction_format(left.b.num < 0 ? negate(left.b) : left.b, num, sizeof num);
        snprintf(action, sizeof action, "%s %s %s each side",
                 left.b.num < 0 ? "Add" : "Subtract", num, left.b.num < 0 ? "to" : "from");
        if (fraction_add(right.b, negate(left.b), &right.b) != 0) {
            return invalid(out, "the numbers are too big");
        }
        left.b = fraction_integer(0);
        add_step(out, left, right, action);
    }

    /* 4x = 20: divide each side by 4. (2/5)x = 10: multiply each side
       by the reciprocal, 5/2. */
    a = left.a;
    if (!is_one(a)) {
        if (a.den == 1) {
            snprintf(action, sizeof action, "Divide each side by %ld", a.num);
        } else {
            fraction_format(fraction_make(a.den, a.num), num, sizeof num);
            snprintf(action, sizeof action, "Multiply each side by %s", num);
        }
        if (fraction_divide(right.b, a, &right.b) != 0) {
            return invalid(out, "the numbers are too big");
        }
        left.a = fraction_integer(1);
        add_step(out, left, right, action);
    }

    out->kind = LINEAR_ONE_SOLUTION;
    out->value = right.b;
    return out->kind;
}
