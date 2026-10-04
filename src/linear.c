#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "linear.h"

/* ---- Each side of the equation, simplified to ax + by + c ---- */

typedef struct {
    const char *s;
    int count;                        /* variables seen so far */
    char names[LINEAR_MAX_VARIABLES];
    const char *error;
} Parser;

static const LinearExpr zero_expr = { { { 0, 1 }, { 0, 1 } }, { 0, 1 } };

static LinearExpr constant(Fraction c) {
    LinearExpr x = zero_expr;
    x.constant = c;
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

static int has_variable(const LinearExpr *x) {
    for (int i = 0; i < LINEAR_MAX_VARIABLES; i++) {
        if (x->coef[i].num != 0) return 1;
    }
    return 0;
}

static LinearExpr add(Parser *p, LinearExpr x, LinearExpr y) {
    int failed = fraction_add(x.constant, y.constant, &x.constant) != 0;
    for (int i = 0; i < LINEAR_MAX_VARIABLES; i++) {
        failed |= fraction_add(x.coef[i], y.coef[i], &x.coef[i]) != 0;
    }
    if (failed) {
        fail(p, "the numbers are too big");
    }
    return x;
}

static LinearExpr subtract(Parser *p, LinearExpr x, LinearExpr y) {
    y.constant = negate(y.constant);
    for (int i = 0; i < LINEAR_MAX_VARIABLES; i++) {
        y.coef[i] = negate(y.coef[i]);
    }
    return add(p, x, y);
}

/* A product is only linear when one side is a plain number. */
static LinearExpr multiply(Parser *p, LinearExpr x, LinearExpr y) {
    LinearExpr r = zero_expr, t;
    int failed;

    if (has_variable(&x) && has_variable(&y)) {
        fail(p, "not linear: it multiplies variables together");
        return r;
    }
    if (has_variable(&x)) { /* swap so that x is the plain number */
        t = x;
        x = y;
        y = t;
    }
    /* c(ax + by + d) = cax + cby + cd: the distributive property */
    failed = fraction_multiply(x.constant, y.constant, &r.constant) != 0;
    for (int i = 0; i < LINEAR_MAX_VARIABLES; i++) {
        failed |= fraction_multiply(x.constant, y.coef[i], &r.coef[i]) != 0;
    }
    if (failed) {
        fail(p, "the numbers are too big");
    }
    return r;
}

static LinearExpr divide(Parser *p, LinearExpr x, LinearExpr y) {
    LinearExpr r = zero_expr;
    int failed;

    if (has_variable(&y)) {
        fail(p, "not linear: it divides by a variable");
        return r;
    }
    if (y.constant.num == 0) {
        fail(p, "it divides by 0");
        return r;
    }
    failed = fraction_divide(x.constant, y.constant, &r.constant) != 0;
    for (int i = 0; i < LINEAR_MAX_VARIABLES; i++) {
        failed |= fraction_divide(x.coef[i], y.constant, &r.coef[i]) != 0;
    }
    if (failed) {
        fail(p, "the numbers are too big");
    }
    return r;
}

/* ---- Reading an equation ---- */

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

static LinearExpr parse_expr(Parser *p);

/* digits[.digits] as an exact fraction: 3.1 = 31/10 */
static LinearExpr parse_number(Parser *p) {
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

static LinearExpr parse_variable(Parser *p) {
    LinearExpr x = zero_expr;
    char name = *p->s++;
    int i = 0;

    while (i < p->count && p->names[i] != name) i++;
    if (i == p->count) {
        if (p->count == LINEAR_MAX_VARIABLES) {
            fail(p, "there are more than two variables");
            return x;
        }
        p->names[p->count++] = name;
    }
    x.coef[i] = fraction_integer(1);
    return x;
}

/* a number, a variable, a bracket, or a sign before one of these */
static LinearExpr parse_factor(Parser *p) {
    LinearExpr x;

    if (accept(p, minus_signs)) {
        return subtract(p, zero_expr, parse_factor(p));
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
        return parse_variable(p);
    }
    fail(p, *p->s == '\0' || *p->s == '=' ? "a number or variable is missing"
                                          : "it has a symbol that is not understood");
    return zero_expr;
}

/* factors joined by × or ÷, or written side by side: 2x, 2(x - 4) */
static LinearExpr parse_term(Parser *p) {
    LinearExpr x = parse_factor(p);
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
static LinearExpr parse_expr(Parser *p) {
    LinearExpr x = parse_term(p);
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

const char *linear_parse(const char *text, LinearEquation *out) {
    Parser p = { text, 0, { 0 }, NULL };

    memset(out, 0, sizeof *out);
    out->left = parse_expr(&p);
    skip_spaces(&p);
    if (p.error == NULL && *p.s != '=') {
        fail(&p, *p.s == '\0' ? "there is no = sign" : "it has a symbol that is not understood");
    }
    if (p.error == NULL) {
        p.s++;
        out->right = parse_expr(&p);
        skip_spaces(&p);
        if (p.error == NULL && *p.s != '\0') {
            fail(&p, *p.s == '=' ? "there is more than one = sign" : "it has a symbol that is not understood");
        }
    }
    out->count = p.count;
    memcpy(out->names, p.names, sizeof out->names);
    return p.error;
}

/* ---- Writing equations ---- */

/* "x", "4x", "(2/5)x", without the sign */
static void format_term(Fraction a, char variable, char *out, size_t size) {
    char num[48];
    if (a.num == a.den) {
        snprintf(out, size, "%c", variable);
    } else if (a.den == 1) {
        snprintf(out, size, "%ld%c", a.num, variable);
    } else {
        fraction_format(a, num, sizeof num);
        snprintf(out, size, "(%s)%c", num, variable);
    }
}

int linear_format(const LinearExpr *e, const char *names, int count, char *out, size_t size) {
    char part[64];
    size_t len = 0;
    int first = 1, n;

    if (size == 0) return -1;
    out[0] = '\0';
    for (int i = 0; i <= count; i++) {
        Fraction f = fraction_make(i < count ? e->coef[i].num : e->constant.num,
                                   i < count ? e->coef[i].den : e->constant.den);
        int negative = f.num < 0;

        if (f.num == 0 && !(i == count && first)) {
            continue; /* skip zero terms, but write "0" if nothing else */
        }
        if (negative) f.num = -f.num;
        if (i < count) {
            format_term(f, names[i], part, sizeof part);
        } else {
            fraction_format(f, part, sizeof part);
        }
        n = snprintf(out + len, size - len, "%s%s",
                     first ? (negative ? "-" : "") : (negative ? " - " : " + "), part);
        if (n < 0 || (size_t)n >= size - len) return -1;
        len += (size_t)n;
        first = 0;
    }
    return (int)len;
}

static void add_step(LinearSolution *out, LinearExpr left, LinearExpr right, const char *action) {
    LinearStep *step;
    char l[62], r[62]; /* so "l = r" always fits in step->equation */
    if (out->step_count == LINEAR_MAX_STEPS) {
        return;
    }
    step = &out->steps[out->step_count++];
    linear_format(&left, &out->variable, 1, l, sizeof l);
    linear_format(&right, &out->variable, 1, r, sizeof r);
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

static int is_zero(Fraction f) { return f.num == 0; }
static int is_one(Fraction f) { return f.num == f.den; }

static LinearKind invalid(LinearSolution *out, const char *error) {
    out->kind = LINEAR_INVALID;
    out->error = error;
    return out->kind;
}

LinearKind linear_solve(const char *equation, LinearSolution *out) {
    LinearEquation eq;
    LinearExpr left, right, t;
    char action[96], term[64], num[48];
    const char *error;
    Fraction a;
    size_t len;

    error = linear_parse(equation, &eq);
    memset(out, 0, sizeof *out);
    if (error != NULL) {
        return invalid(out, error);
    }
    if (eq.count == 0) {
        return invalid(out, "there is no variable to solve for");
    }
    if (eq.count > 1) {
        return invalid(out, "there is more than one variable");
    }
    out->variable = eq.names[0];
    left = eq.left;
    right = eq.right;

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
    if (is_zero(left.coef[0]) && !is_zero(right.coef[0])) {
        t = left;
        left = right;
        right = t;
        add_step(out, left, right, "Swap the two sides");
    }

    /* 9x - 12 = 5x + 8: subtract 5x from each side */
    if (!is_zero(right.coef[0])) {
        a = right.coef[0];
        format_term(a.num < 0 ? negate(a) : a, out->variable, term, sizeof term);
        snprintf(action, sizeof action, "%s %s %s each side",
                 a.num < 0 ? "Add" : "Subtract", term, a.num < 0 ? "to" : "from");
        if (fraction_add(left.coef[0], negate(a), &left.coef[0]) != 0) {
            return invalid(out, "the numbers are too big");
        }
        right.coef[0] = fraction_integer(0);
        add_step(out, left, right, action);
    }

    if (is_zero(left.coef[0])) { /* the variable cancelled out */
        out->kind = fraction_equal(left.constant, right.constant) ? LINEAR_ALL_NUMBERS : LINEAR_NO_SOLUTION;
        return out->kind;
    }

    /* 4x - 12 = 8: add 12 to each side */
    if (!is_zero(left.constant)) {
        a = left.constant;
        fraction_format(a.num < 0 ? negate(a) : a, num, sizeof num);
        snprintf(action, sizeof action, "%s %s %s each side",
                 a.num < 0 ? "Add" : "Subtract", num, a.num < 0 ? "to" : "from");
        if (fraction_add(right.constant, negate(a), &right.constant) != 0) {
            return invalid(out, "the numbers are too big");
        }
        left.constant = fraction_integer(0);
        add_step(out, left, right, action);
    }

    /* 4x = 20: divide each side by 4. (2/5)x = 10: multiply each side
       by the reciprocal, 5/2. */
    a = left.coef[0];
    if (!is_one(a)) {
        if (a.den == 1) {
            snprintf(action, sizeof action, "Divide each side by %ld", a.num);
        } else {
            fraction_format(fraction_make(a.den, a.num), num, sizeof num);
            snprintf(action, sizeof action, "Multiply each side by %s", num);
        }
        if (fraction_divide(right.constant, a, &right.constant) != 0) {
            return invalid(out, "the numbers are too big");
        }
        left.coef[0] = fraction_integer(1);
        add_step(out, left, right, action);
    }

    out->kind = LINEAR_ONE_SOLUTION;
    out->value = right.constant;
    return out->kind;
}
