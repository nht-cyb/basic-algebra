#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "system.h"

/* An equation written as a[0]·x + a[1]·y = c */
typedef struct {
    Fraction a[2];
    Fraction c;
} Standard;

typedef struct {
    SystemSolution *out;
    int too_big; /* set when a fraction does not fit in a long */
} Solver;

static Fraction sum(Solver *s, Fraction x, Fraction y) {
    Fraction r = { 0, 1 };
    if (fraction_add(x, y, &r) != 0) s->too_big = 1;
    return r;
}

static Fraction product(Solver *s, Fraction x, Fraction y) {
    Fraction r = { 0, 1 };
    if (fraction_multiply(x, y, &r) != 0) s->too_big = 1;
    return r;
}

static Fraction quotient(Solver *s, Fraction x, Fraction y) {
    Fraction r = { 0, 1 };
    if (fraction_divide(x, y, &r) != 0) s->too_big = 1;
    return r;
}

static Fraction negate(Fraction f) {
    return fraction_make(-f.num, f.den);
}

static int is_zero(Fraction f) { return f.num == 0; }

/* ---- Steps ---- */

static void add_text_step(Solver *s, const char *equation, const char *action) {
    SystemSolution *out = s->out;
    LinearStep *step;
    if (out->step_count == SYSTEM_MAX_STEPS) {
        return;
    }
    step = &out->steps[out->step_count++];
    snprintf(step->equation, sizeof step->equation, "%s", equation);
    snprintf(step->action, sizeof step->action, "%s", action);
}

/* "a·x + b·y = c" */
static void format_standard(const Standard *e, const char *names, char *out, size_t size) {
    LinearExpr left = { { e->a[0], e->a[1] }, { 0, 1 } };
    char l[62], r[48];
    linear_format(&left, names, 2, l, sizeof l);
    fraction_format(e->c, r, sizeof r);
    snprintf(out, size, "%s = %s", l, r);
}

static void add_standard_step(Solver *s, const Standard *e, const char *action) {
    char text[128];
    format_standard(e, s->out->names, text, sizeof text);
    add_text_step(s, text, action);
}

/* The next character that is not a space, reading "−" as "-" */
static char next_char(const char **s) {
    while (isspace((unsigned char)**s)) (*s)++;
    if (strncmp(*s, "−", strlen("−")) == 0) {
        *s += strlen("−");
        return '-';
    }
    return *(*s)++;
}

static int same_ignoring_spaces(const char *a, const char *b) {
    for (;;) {
        char ca = next_char(&a), cb = next_char(&b);
        if (ca != cb) return 0;
        if (ca == '\0') return 1;
    }
}

/* Solves "coef·name + constant = c" for name with linear_solve, adding
   its steps after a first step with the given action. */
static int solve_for(Solver *s, char name, Fraction coef, Fraction constant, Fraction c,
                     const char *action, Fraction *value) {
    LinearExpr left = { { coef, { 0, 1 } }, constant };
    LinearSolution one;
    char l[62], r[48], text[128];
    int shown;

    linear_format(&left, &name, 1, l, sizeof l);
    fraction_format(c, r, sizeof r);
    snprintf(text, sizeof text, "%s = %s", l, r);
    shown = 0;
    for (int i = 0; i < s->out->step_count; i++) {
        shown |= same_ignoring_spaces(s->out->steps[i].equation, text);
    }
    if (!shown) { /* don't repeat an equation that is already a step */
        add_text_step(s, text, action);
    }

    if (linear_solve(text, &one) != LINEAR_ONE_SOLUTION) {
        s->too_big = 1;
        return -1;
    }
    for (int i = 1; i < one.step_count; i++) {
        add_text_step(s, one.steps[i].equation, one.steps[i].action);
    }
    *value = one.value;
    return 0;
}

/* The line every solution is on: "y = 4x - 5", or "x = 3" without y */
static void describe_line(Solver *s, const Standard *e) {
    SystemSolution *out = s->out;
    LinearExpr right = { { { 0, 1 }, { 0, 1 } }, { 0, 1 } };
    char r[56];
    int solve_for_y = !is_zero(e->a[1]);
    int v = solve_for_y ? 1 : 0;

    if (is_zero(e->a[0]) && is_zero(e->a[1])) {
        snprintf(out->line, sizeof out->line, "%c and %c can be any numbers", out->names[0], out->names[1]);
        return;
    }
    if (solve_for_y) {
        right.coef[0] = quotient(s, negate(e->a[0]), e->a[1]);
    }
    right.constant = quotient(s, e->c, e->a[v]);
    linear_format(&right, out->names, 1, r, sizeof r);
    snprintf(out->line, sizeof out->line, "%c = %s", out->names[v], r);
}

/* ---- Reading the equations ---- */

static const char *read_equation(Solver *s, const char *text, Standard *e) {
    SystemSolution *out = s->out;
    LinearEquation eq;
    const char *error = linear_parse(text, &eq);
    int v;

    if (error != NULL) {
        return error;
    }
    e->a[0] = e->a[1] = fraction_integer(0);
    /* ax + by + k = dx + ey + m becomes (a - d)x + (b - e)y = m - k */
    e->c = sum(s, eq.right.constant, negate(eq.left.constant));
    for (int i = 0; i < eq.count; i++) {
        for (v = 0; v < 2 && out->names[v] != 0 && out->names[v] != eq.names[i]; v++)
            ;
        if (v == 2) {
            return "there are more than two variables";
        }
        out->names[v] = eq.names[i];
        e->a[v] = sum(s, eq.left.coef[i], negate(eq.right.coef[i]));
    }
    return NULL;
}

static void add_given_step(Solver *s, const char *text, const char *action) {
    size_t len;
    char trimmed[128];
    while (isspace((unsigned char)*text)) text++;
    len = strlen(text);
    while (len > 0 && isspace((unsigned char)text[len - 1])) len--;
    snprintf(trimmed, sizeof trimmed, "%.*s", (int)len, text);
    add_text_step(s, trimmed, action);
}

/* ---- Solving ---- */

static SystemKind finish(Solver *s, SystemKind kind) {
    if (s->too_big) {
        s->out->kind = SYSTEM_INVALID;
        s->out->error = "the numbers are too big";
    } else {
        s->out->kind = kind;
    }
    return s->out->kind;
}

static SystemKind invalid(SystemSolution *out, const char *error) {
    out->kind = SYSTEM_INVALID;
    out->error = error;
    return out->kind;
}

static int is_integer(Fraction f) {
    return fraction_make(f.num, f.den).den == 1;
}

/* Finds k1 and k2 so that k1·e1 + k2·e2 eliminates a variable,
   preferring y as the lessons do. Returns the variable eliminated. */
static int choose_elimination(Solver *s, const Standard *e1, const Standard *e2,
                              Fraction *k1, Fraction *k2) {
    Fraction one = fraction_integer(1), r;
    int v;

    for (v = 1; v >= 0; v--) { /* opposite: x + y and x - y */
        if (fraction_equal(e1->a[v], negate(e2->a[v]))) {
            *k1 = one;
            *k2 = one;
            return v;
        }
    }
    for (v = 1; v >= 0; v--) { /* the same: 2x + y and 2x + y */
        if (fraction_equal(e1->a[v], e2->a[v])) {
            *k1 = one;
            *k2 = negate(one);
            return v;
        }
    }
    for (v = 1; v >= 0; v--) { /* y and -2y: multiply one equation by a whole number */
        r = quotient(s, negate(e2->a[v]), e1->a[v]);
        if (is_integer(r)) {
            *k1 = r;
            *k2 = one;
            return v;
        }
        r = quotient(s, negate(e1->a[v]), e2->a[v]);
        if (is_integer(r)) {
            *k1 = one;
            *k2 = r;
            return v;
        }
    }
    /* 2y and 3y: multiply equation 1 by 3 and equation 2 by -2 */
    *k1 = e2->a[1];
    *k2 = negate(e1->a[1]);
    if (k1->num < 0) {
        *k1 = negate(*k1);
        *k2 = negate(*k2);
    }
    return 1;
}

static Standard scale(Solver *s, const Standard *e, Fraction k) {
    Standard r;
    r.a[0] = product(s, e->a[0], k);
    r.a[1] = product(s, e->a[1], k);
    r.c = product(s, e->c, k);
    return r;
}

/* The value of names[u] is known: find the other variable from an
   equation that has it, or check the equations if neither has it. */
static SystemKind find_other(Solver *s, const Standard *e, int u, Fraction value) {
    SystemSolution *out = s->out;
    int w = 1 - u, k;
    char action[96], num[48];

    out->values[u] = value;
    for (k = 0; k < 2 && is_zero(e[k].a[w]); k++)
        ;
    if (k == 2) {
        /* names[w] is in neither equation: any value works, if both
           equations hold */
        for (k = 0; k < 2; k++) {
            if (!fraction_equal(product(s, e[k].a[u], value), e[k].c)) {
                return finish(s, SYSTEM_NO_SOLUTION);
            }
        }
        fraction_format(value, num, sizeof num);
        snprintf(out->line, sizeof out->line, "%c = %s", out->names[u], num);
        return finish(s, SYSTEM_INFINITELY_MANY);
    }

    fraction_format(value, num, sizeof num);
    snprintf(action, sizeof action, "Substitute %c = %s into equation %d", out->names[u], num, k + 1);
    if (solve_for(s, out->names[w], e[k].a[w], product(s, e[k].a[u], value), e[k].c,
                  action, &out->values[w]) != 0) {
        return finish(s, SYSTEM_INVALID);
    }
    return finish(s, SYSTEM_ONE_SOLUTION);
}

SystemKind system_solve(const char *first, const char *second, SystemSolution *out) {
    Solver s = { out, 0 };
    Standard e[2], m1, m2;
    Fraction k1, k2, coef, value, one = fraction_integer(1);
    const char *error;
    char action[96], num[48], text[128];
    int u, v, k, subtract;

    memset(out, 0, sizeof *out);
    if ((error = read_equation(&s, first, &e[0])) != NULL ||
        (error = read_equation(&s, second, &e[1])) != NULL) {
        return invalid(out, error);
    }
    if (out->names[1] == 0) {
        return invalid(out, "a system needs two variables");
    }
    if (out->names[0] > out->names[1]) { /* x before y, as in y = 2x + 1 */
        char t = out->names[0];
        out->names[0] = out->names[1];
        out->names[1] = t;
        for (k = 0; k < 2; k++) {
            Fraction f = e[k].a[0];
            e[k].a[0] = e[k].a[1];
            e[k].a[1] = f;
        }
    }

    add_given_step(&s, first, "Equation 1");
    add_given_step(&s, second, "Equation 2");
    for (k = 0; k < 2; k++) {
        format_standard(&e[k], out->names, text, sizeof text);
        if (!same_ignoring_spaces(text, out->steps[k].equation)) {
            snprintf(action, sizeof action, "Write equation %d as a%c + b%c = c", k + 1, out->names[0], out->names[1]);
            add_text_step(&s, text, action);
        }
    }

    /* An equation with no variables left is always true (0 = 0) or
       never true (0 = 5). */
    for (k = 0; k < 2; k++) {
        if (is_zero(e[k].a[0]) && is_zero(e[k].a[1])) {
            if (!is_zero(e[k].c)) {
                return finish(&s, SYSTEM_NO_SOLUTION);
            }
            if (is_zero(e[1 - k].a[0]) && is_zero(e[1 - k].a[1]) && !is_zero(e[1 - k].c)) {
                return finish(&s, SYSTEM_NO_SOLUTION);
            }
            describe_line(&s, &e[1 - k]);
            return finish(&s, SYSTEM_INFINITELY_MANY);
        }
    }

    /* An equation with one variable can be solved straight away. */
    for (k = 0; k < 2; k++) {
        for (u = 0; u < 2; u++) {
            if (is_zero(e[k].a[1 - u])) {
                if (fraction_equal(e[k].a[u], one)) { /* already x = 3 */
                    return find_other(&s, e, u, e[k].c);
                }
                snprintf(action, sizeof action, "Solve equation %d for %c", k + 1, out->names[u]);
                if (solve_for(&s, out->names[u], e[k].a[u], fraction_integer(0), e[k].c, action, &value) != 0) {
                    return finish(&s, SYSTEM_INVALID);
                }
                return find_other(&s, e, u, value);
            }
        }
    }

    /* Elimination */
    v = choose_elimination(&s, &e[0], &e[1], &k1, &k2);
    u = 1 - v;
    m1 = scale(&s, &e[0], k1);
    m2 = scale(&s, &e[1], k2);
    subtract = fraction_equal(k1, one) && fraction_equal(k2, negate(one));
    if (!subtract && !fraction_equal(k1, one)) {
        fraction_format(k1, num, sizeof num);
        snprintf(action, sizeof action, "Multiply equation 1 by %s", num);
        add_standard_step(&s, &m1, action);
    }
    if (!subtract && !fraction_equal(k2, one)) {
        fraction_format(k2, num, sizeof num);
        snprintf(action, sizeof action, "Multiply equation 2 by %s", num);
        add_standard_step(&s, &m2, action);
    }
    if (subtract) {
        snprintf(action, sizeof action, "Subtract equation 2 from equation 1 to eliminate %c", out->names[v]);
    } else {
        snprintf(action, sizeof action, "Add the two equations to eliminate %c", out->names[v]);
    }

    coef = sum(&s, m1.a[u], m2.a[u]);
    value = sum(&s, m1.c, m2.c);
    if (is_zero(coef)) {
        /* both variables were eliminated: 0 = 0 or 0 = 4 */
        fraction_format(value, num, sizeof num);
        snprintf(text, sizeof text, "0 = %s", num);
        add_text_step(&s, text, action);
        if (!is_zero(value)) {
            return finish(&s, SYSTEM_NO_SOLUTION);
        }
        describe_line(&s, &e[0]);
        return finish(&s, SYSTEM_INFINITELY_MANY);
    }
    if (solve_for(&s, out->names[u], coef, fraction_integer(0), value, action, &value) != 0) {
        return finish(&s, SYSTEM_INVALID);
    }
    return find_other(&s, e, u, value);
}
