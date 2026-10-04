#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "absolute.h"

/* ---- Expressions: a·x + b + k₁|A₁| + k₂|A₂| + ... ---- */

#define MAX_TERMS 4

typedef struct {
    Fraction k;      /* the number in front of |...| */
    Fraction ia, ib; /* what is inside: ia·x + ib */
    char text[48];   /* how it was written, e.g. "1 - 3x" */
} AbsTerm;

typedef struct {
    Fraction a, b;
    int n;
    AbsTerm t[MAX_TERMS];
} AbsExpr;

typedef struct {
    const char *s;
    char variable;
    int depth; /* how many |...| we are inside */
    const char *error;
} Parser;

static void fail(Parser *p, const char *error) {
    if (p->error == NULL) p->error = error;
}

static Fraction add_f(Parser *p, Fraction x, Fraction y) {
    Fraction r = { 0, 1 };
    if (fraction_add(x, y, &r) != 0) fail(p, "the numbers are too big");
    return r;
}

static Fraction mul_f(Parser *p, Fraction x, Fraction y) {
    Fraction r = { 0, 1 };
    if (fraction_multiply(x, y, &r) != 0) fail(p, "the numbers are too big");
    return r;
}

static Fraction negate(Fraction f) { return fraction_make(-f.num, f.den); }
static int is_zero(Fraction f) { return f.num == 0; }

static AbsExpr constant(Fraction b) {
    AbsExpr x;
    memset(&x, 0, sizeof x);
    x.a = fraction_integer(0);
    x.b = b;
    return x;
}

static int is_constant(const AbsExpr *x) {
    return is_zero(x->a) && x->n == 0;
}

/* Adds k|A| to x, joining it with a term that has the same inside, or
   the opposite one, since |-A| = |A|: 6|2x + 3| - 2|2x + 3| = 4|2x + 3|. */
static void add_term(Parser *p, AbsExpr *x, const AbsTerm *t) {
    for (int i = 0; i < x->n; i++) {
        AbsTerm *u = &x->t[i];
        if ((fraction_equal(u->ia, t->ia) && fraction_equal(u->ib, t->ib)) ||
            (fraction_equal(u->ia, negate(t->ia)) && fraction_equal(u->ib, negate(t->ib)))) {
            u->k = add_f(p, u->k, t->k);
            return;
        }
    }
    if (x->n == MAX_TERMS) {
        fail(p, "there are too many absolute values");
        return;
    }
    x->t[x->n++] = *t;
}

static void drop_zero_terms(AbsExpr *x) {
    int n = 0;
    for (int i = 0; i < x->n; i++) {
        if (!is_zero(x->t[i].k)) x->t[n++] = x->t[i];
    }
    x->n = n;
}

static AbsExpr add(Parser *p, AbsExpr x, AbsExpr y) {
    x.a = add_f(p, x.a, y.a);
    x.b = add_f(p, x.b, y.b);
    for (int i = 0; i < y.n; i++) add_term(p, &x, &y.t[i]);
    drop_zero_terms(&x);
    return x;
}

static AbsExpr scale(Parser *p, AbsExpr x, Fraction c) {
    x.a = mul_f(p, x.a, c);
    x.b = mul_f(p, x.b, c);
    for (int i = 0; i < x.n; i++) x.t[i].k = mul_f(p, x.t[i].k, c);
    drop_zero_terms(&x);
    return x;
}

static AbsExpr multiply(Parser *p, AbsExpr x, AbsExpr y) {
    if (is_constant(&y)) return scale(p, x, y.b);
    if (is_constant(&x)) return scale(p, y, x.b);
    fail(p, "not linear: it multiplies variables or absolute values together");
    return x;
}

static AbsExpr divide(Parser *p, AbsExpr x, AbsExpr y) {
    Fraction inverse;
    if (!is_constant(&y)) {
        fail(p, "not linear: it divides by a variable or an absolute value");
        return x;
    }
    if (is_zero(y.b)) {
        fail(p, "it divides by 0");
        return x;
    }
    inverse = fraction_make(y.b.den, y.b.num);
    return scale(p, x, inverse);
}

/* ---- Reading ---- */

static void skip_spaces(Parser *p) {
    while (isspace((unsigned char)*p->s)) p->s++;
}

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
static const char *const bar[] = { "|", NULL };
static const char *const power_sign[] = { "^", NULL };

static AbsExpr parse_expr(Parser *p);

static AbsExpr parse_number(Parser *p) {
    Fraction value = { 0, 1 };
    long scale_by = 1;
    int after_point = 0;

    for (; isdigit((unsigned char)*p->s) || (*p->s == '.' && !after_point); p->s++) {
        if (*p->s == '.') {
            after_point = 1;
            continue;
        }
        value = add_f(p, mul_f(p, value, fraction_integer(10)), fraction_integer(*p->s - '0'));
        if (after_point && __builtin_mul_overflow(scale_by, 10, &scale_by)) fail(p, "the numbers are too big");
    }
    return constant(fraction_make(value.num, value.den * scale_by));
}

/* "1 − 3x" as written, with − as - */
static void copy_text(const char *start, const char *end, char *out, size_t size) {
    size_t len = 0;
    while (start < end && isspace((unsigned char)*start)) start++;
    while (end > start && isspace((unsigned char)end[-1])) end--;
    while (start < end && len + 1 < size) {
        if (strncmp(start, "−", strlen("−")) == 0) {
            out[len++] = '-';
            start += strlen("−");
        } else {
            out[len++] = *start++;
        }
    }
    out[len] = '\0';
}

/* a number, the variable, a bracket or |...| */
static AbsExpr parse_atom(Parser *p) {
    AbsExpr x = constant(fraction_integer(0));

    if (accept(p, open_bracket)) {
        x = parse_expr(p);
        if (!accept(p, close_bracket)) fail(p, "a bracket is not closed");
        return x;
    }
    if (accept(p, bar)) {
        const char *start = p->s;
        AbsTerm t;
        p->depth++;
        x = parse_expr(p);
        p->depth--;
        if (!accept(p, bar)) {
            fail(p, "an absolute value bar | is not closed");
            return x;
        }
        if (x.n > 0) {
            fail(p, "an absolute value with a variable inside another one is not supported");
            return x;
        }
        if (is_zero(x.a)) { /* |-8 + 10| = 2: just a number */
            return constant(x.b.num < 0 ? negate(x.b) : x.b);
        }
        memset(&t, 0, sizeof t);
        t.k = fraction_integer(1);
        t.ia = x.a;
        t.ib = x.b;
        copy_text(start, p->s - 1, t.text, sizeof t.text);
        x = constant(fraction_integer(0));
        x.n = 1;
        x.t[0] = t;
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

/* 4^2: a power of a number */
static AbsExpr parse_power(Parser *p) {
    AbsExpr x = parse_atom(p), r;
    long n = 0;

    if (!accept(p, power_sign)) return x;
    skip_spaces(p);
    while (isdigit((unsigned char)*p->s) && n <= 64) n = n * 10 + (*p->s++ - '0');
    if (!is_constant(&x)) {
        fail(p, "not linear: it has a power of the variable");
        return x;
    }
    if (n > 64) {
        fail(p, "the power is too big");
        return x;
    }
    r = constant(fraction_integer(1));
    for (long i = 0; i < n; i++) r.b = mul_f(p, r.b, x.b);
    return r;
}

static AbsExpr parse_factor(Parser *p) {
    if (accept(p, minus_signs)) return scale(p, parse_factor(p), fraction_integer(-1));
    if (accept(p, plus_signs)) return parse_factor(p);
    return parse_power(p);
}

/* factors joined by × or ÷, or side by side: 4|2x - 1|, 2(x + 1).
   Inside |...|, a | ends the absolute value instead of starting one. */
static AbsExpr parse_term(Parser *p) {
    AbsExpr x = parse_factor(p);
    while (p->error == NULL) {
        if (accept(p, times_signs)) {
            x = multiply(p, x, parse_factor(p));
        } else if (accept(p, divide_signs)) {
            x = divide(p, x, parse_factor(p));
        } else if (skip_spaces(p), isalpha((unsigned char)*p->s) || *p->s == '(' ||
                   (*p->s == '|' && p->depth == 0)) {
            x = multiply(p, x, parse_factor(p));
        } else {
            break;
        }
    }
    return x;
}

static AbsExpr parse_expr(Parser *p) {
    AbsExpr x = parse_term(p);
    while (p->error == NULL) {
        if (accept(p, plus_signs)) {
            x = add(p, x, parse_term(p));
        } else if (accept(p, minus_signs)) {
            x = add(p, x, scale(p, parse_term(p), fraction_integer(-1)));
        } else {
            break;
        }
    }
    return x;
}

int absolute_value_evaluate(const char *text, Fraction *out) {
    Parser p = { text, 0, 0, NULL };
    AbsExpr x = parse_expr(&p);
    skip_spaces(&p);
    if (p.error != NULL || *p.s != '\0' || !is_constant(&x)) return -1;
    *out = x.b;
    return 0;
}

/* ---- Writing ---- */

typedef struct {
    char *out;
    size_t size, len;
} Writer;

static void put(Writer *w, const char *s) {
    int n = snprintf(w->out + w->len, w->size - w->len, "%s", s);
    if (n > 0) w->len = w->len + (size_t)n < w->size ? w->len + (size_t)n : w->size - 1;
}

/* "4|2x - 1|", "-|x|", "(1/2)|1 - 3x|" with " + " or " - " in front
   unless it comes first */
static void put_signed(Writer *w, Fraction k, const char *body, int first) {
    char num[48];
    k = fraction_make(k.num, k.den);
    if (first) {
        if (k.num < 0) put(w, "-");
    } else {
        put(w, k.num < 0 ? " - " : " + ");
    }
    if (k.num < 0) k.num = -k.num;
    if (k.num == k.den && body[0] != '\0') {
        num[0] = '\0';
    } else if (k.den != 1 && body[0] != '\0') {
        num[0] = '(';
        fraction_format(k, num + 1, sizeof num - 2);
        strcat(num, ")");
    } else {
        fraction_format(k, num, sizeof num);
    }
    put(w, num);
    put(w, body);
}

/* the left side k₁|A₁| + ... + ax + b, and "= c" */
static void format_equation(const AbsExpr *x, char variable, Fraction right, char *out, size_t size) {
    Writer w = { out, size, 0 };
    char body[64], num[48];
    int first = 1;

    out[0] = '\0';
    for (int i = 0; i < x->n; i++) {
        snprintf(body, sizeof body, "|%s|", x->t[i].text);
        put_signed(&w, x->t[i].k, body, first);
        first = 0;
    }
    if (!is_zero(x->a)) {
        snprintf(body, sizeof body, "%c", variable);
        put_signed(&w, x->a, body, first);
        first = 0;
    }
    if (!is_zero(x->b) || first) put_signed(&w, x->b, "", first);
    fraction_format(right, num, sizeof num);
    put(&w, " = ");
    put(&w, num);
}

/* ---- Solving ---- */

typedef struct {
    AbsoluteSolution *out;
    const char *error;
} Solver;

static void add_step(Solver *s, const char *equation, const char *action) {
    LinearStep *step;
    if (s->out->step_count == ABSOLUTE_MAX_STEPS) return;
    step = &s->out->steps[s->out->step_count++];
    snprintf(step->equation, sizeof step->equation, "%s", equation);
    snprintf(step->action, sizeof step->action, "%s", action);
}

static int same_ignoring_spaces(const char *a, const char *b) {
    for (;;) {
        while (isspace((unsigned char)*a)) a++;
        while (isspace((unsigned char)*b)) b++;
        if (strncmp(a, "−", strlen("−")) == 0 && *b == '-') { a += strlen("−"); b++; continue; }
        if (strncmp(b, "−", strlen("−")) == 0 && *a == '-') { b += strlen("−"); a++; continue; }
        if (*a != *b) return 0;
        if (*a == '\0') return 1;
        a++;
        b++;
    }
}

static SolutionSet point(Fraction v) {
    SolutionSet lo = solution_set_from(RELATION_GREATER_EQUAL, v);
    SolutionSet hi = solution_set_from(RELATION_LESS_EQUAL, v);
    return solution_set_and(&lo, &hi);
}

/* Solves a linear equation with linear_solve, adding its steps after a
   first step with the given action. Returns its kind. */
static LinearKind solve_linear(Solver *s, const char *text, const char *action, Fraction *value) {
    LinearSolution one;
    add_step(s, text, action);
    linear_solve(text, &one);
    if (one.kind == LINEAR_INVALID) {
        if (s->error == NULL) s->error = one.error;
        return one.kind;
    }
    for (int i = 1; i < one.step_count; i++) add_step(s, one.steps[i].equation, one.steps[i].action);
    *value = one.value;
    return one.kind;
}

/* One absolute value: k|A| + c = d, as in the lessons */
static SolutionSet solve_one(Solver *s, AbsExpr x, Fraction right) {
    AbsoluteSolution *out = s->out;
    AbsTerm *t = &x.t[0];
    char text[256], action[320], num[48];
    Fraction k = t->k, v;
    SolutionSet set, other;

    /* k|A| = d - c */
    if (!is_zero(x.b)) {
        fraction_format(x.b.num < 0 ? negate(x.b) : x.b, num, sizeof num);
        snprintf(action, sizeof action, "%s %s %s each side", x.b.num < 0 ? "Add" : "Subtract",
                 num, x.b.num < 0 ? "to" : "from");
        if (fraction_add(right, negate(x.b), &right) != 0) {
            s->error = "the numbers are too big";
            return solution_set_none();
        }
        x.b = fraction_integer(0);
        format_equation(&x, out->variable, right, text, sizeof text);
        add_step(s, text, action);
    }

    /* |A| = (d - c) / k */
    if (!(k.num == k.den)) {
        fraction_format(k, num, sizeof num);
        snprintf(action, sizeof action, "Divide each side by %s", num);
        if (fraction_divide(right, k, &right) != 0) {
            s->error = "the numbers are too big";
            return solution_set_none();
        }
        x.t[0].k = fraction_integer(1);
        format_equation(&x, out->variable, right, text, sizeof text);
        add_step(s, text, action);
    }

    fraction_format(right, num, sizeof num);
    if (right.num < 0) {
        add_step(s, "no solution", "An absolute value is never negative");
        return solution_set_none();
    }
    if (right.num == 0) {
        snprintf(text, sizeof text, "%s = 0", t->text);
        if (solve_linear(s, text, "Only 0 has absolute value 0", &v) != LINEAR_ONE_SOLUTION) {
            return solution_set_none();
        }
        return point(v);
    }

    /* |A| = 5 means A = 5 or A = -5 */
    snprintf(text, sizeof text, "%s = %s or %s = -%s", t->text, num, t->text, num);
    snprintf(action, sizeof action, "|%s| = %s means %s = %s or %s = -%s", t->text, num, t->text, num, t->text, num);
    add_step(s, text, action);

    snprintf(text, sizeof text, "%s = %s", t->text, num);
    if (solve_linear(s, text, "Solve the first equation", &v) != LINEAR_ONE_SOLUTION) return solution_set_none();
    set = point(v);
    snprintf(text, sizeof text, "%s = -%s", t->text, num);
    if (solve_linear(s, text, "Solve the second equation", &v) != LINEAR_ONE_SOLUTION) return solution_set_none();
    other = point(v);
    return solution_set_or(&set, &other);
}

static int compare(Fraction a, Fraction b) {
    __int128 l, r;
    a = fraction_make(a.num, a.den);
    b = fraction_make(b.num, b.den);
    l = (__int128)a.num * b.den;
    r = (__int128)b.num * a.den;
    return l < r ? -1 : l > r ? 1 : 0;
}

/* Several absolute values, or the variable outside them: solve in each
   region between the numbers where an expression inside |...| is 0. */
static SolutionSet solve_by_regions(Solver *s, const AbsExpr *x, Fraction right) {
    AbsoluteSolution *out = s->out;
    Fraction zeros[MAX_TERMS], t;
    int nz = 0;
    char text[256], action[256], num[48], num2[48];
    SolutionSet result = solution_set_none();
    Writer w;

    /* where is each inside 0? */
    w = (Writer){ action, sizeof action, 0 };
    action[0] = '\0';
    for (int i = 0; i < x->n; i++) {
        Fraction z;
        int seen = 0;
        if (fraction_divide(negate(x->t[i].ib), x->t[i].ia, &z) != 0) {
            s->error = "the numbers are too big";
            return result;
        }
        for (int j = 0; j < nz; j++) seen |= compare(zeros[j], z) == 0;
        if (!seen) zeros[nz++] = z;
        fraction_format(z, num, sizeof num);
        snprintf(text, sizeof text, "%s%s = 0 at %c = %s", i > 0 ? ", " : "", x->t[i].text, out->variable, num);
        put(&w, text);
    }
    for (int i = 1; i < nz; i++) { /* sort */
        for (int j = i; j > 0 && compare(zeros[j], zeros[j - 1]) < 0; j--) {
            t = zeros[j]; zeros[j] = zeros[j - 1]; zeros[j - 1] = t;
        }
    }
    add_step(s, action, "Find where each expression inside |...| is 0");

    /* each region: x < z₁, z₁ ≤ x < z₂, ..., x ≥ zₙ */
    for (int r = 0; r <= nz && s->error == NULL; r++) {
        SolutionSet region = solution_set_all(), bound;
        Fraction sample, value;
        char region_text[96];
        LinearKind kind;
        Writer e = { text, sizeof text, 0 };
        Writer why;
        int first = 1;

        if (r > 0) {
            bound = solution_set_from(RELATION_GREATER_EQUAL, zeros[r - 1]);
            region = solution_set_and(&region, &bound);
        }
        if (r < nz) {
            bound = solution_set_from(RELATION_LESS, zeros[r]);
            region = solution_set_and(&region, &bound);
        }
        solution_set_format(&region, out->variable, region_text, sizeof region_text);

        /* a number in the region tells the sign of each inside */
        if (nz == 0) sample = fraction_integer(0);
        else if (r == 0) fraction_add(zeros[0], fraction_integer(-1), &sample);
        else if (r == nz) fraction_add(zeros[nz - 1], fraction_integer(1), &sample);
        else {
            fraction_add(zeros[r - 1], zeros[r], &sample);
            sample = fraction_make(sample.num, sample.den * 2);
        }

        /* write the equation without bars: |A| is A or -(A) here */
        text[0] = '\0';
        action[0] = '\0';
        why = (Writer){ action, sizeof action, 0 };
        snprintf(action, sizeof action, "When %s, ", region_text);
        why.len = strlen(action);
        for (int i = 0; i < x->n; i++) {
            const AbsTerm *u = &x->t[i];
            Fraction at, k = u->k;
            char body[64];
            int negative;
            fraction_multiply(u->ia, sample, &at);
            fraction_add(at, u->ib, &at);
            negative = at.num < 0;
            if (negative) k = negate(k);
            snprintf(body, sizeof body, "(%s)", u->text);
            put_signed(&e, k, body, first);
            first = 0;
            snprintf(body, sizeof body, "%s%s %s 0", i > 0 ? " and " : "", u->text, negative ? "<" : "≥");
            put(&why, body);
        }
        if (!is_zero(x->a)) {
            char body[4] = { out->variable, '\0' };
            put_signed(&e, x->a, body, first);
            first = 0;
        }
        if (!is_zero(x->b)) put_signed(&e, x->b, "", first);
        fraction_format(right, num, sizeof num);
        put(&e, " = ");
        put(&e, num);

        kind = solve_linear(s, text, action, &value);
        if (kind == LINEAR_INVALID) break;
        if (kind == LINEAR_ALL_NUMBERS) {
            snprintf(text, sizeof text, "every %c with %s", out->variable, region_text);
            add_step(s, text, "The equation is true everywhere in this region");
            result = solution_set_or(&result, &region);
        } else if (kind == LINEAR_ONE_SOLUTION) {
            int inside_region = solution_set_contains(&region, value);
            fraction_format(value, num2, sizeof num2);
            snprintf(text, sizeof text, "%c = %s is %sin %s", out->variable, num2,
                     inside_region ? "" : "not ", region_text);
            add_step(s, text, inside_region ? "So it is a solution" : "So it is not a solution here");
            if (inside_region) {
                SolutionSet p = point(value);
                result = solution_set_or(&result, &p);
            }
        }
    }
    return result;
}

int absolute_solve(const char *equation, AbsoluteSolution *out) {
    Parser p = { equation, 0, 0, NULL };
    Solver s = { out, NULL };
    AbsExpr left, right, x;
    Fraction c;
    char text[256], given[256];
    size_t len;
    int as_given;

    memset(out, 0, sizeof *out);
    left = parse_expr(&p);
    skip_spaces(&p);
    if (p.error == NULL && *p.s != '=') fail(&p, *p.s == '\0' ? "there is no = sign" : "it has a symbol that is not understood");
    if (p.error == NULL) {
        p.s++;
        right = parse_expr(&p);
        skip_spaces(&p);
        if (p.error == NULL && *p.s != '\0') {
            fail(&p, *p.s == '=' ? "there is more than one = sign" : "it has a symbol that is not understood");
        }
    }
    if (p.error != NULL) {
        out->error = p.error;
        return 0;
    }
    out->variable = p.variable ? p.variable : 'x';

    while (isspace((unsigned char)*equation)) equation++;
    len = strlen(equation);
    while (len > 0 && isspace((unsigned char)equation[len - 1])) len--;
    snprintf(given, sizeof given, "%.*s", (int)len, equation);
    add_step(&s, given, "");

    /* absolute values and x on the left, numbers on the right */
    x = add(&p, left, scale(&p, right, fraction_integer(-1)));
    c = negate(x.b);
    x.b = fraction_integer(0);
    as_given = left.n == 1 && right.n == 0 && is_zero(x.a);
    if (as_given) {
        x.b = left.b; /* keep 4|2x - 1| - 8 = 12 as it is, like the lesson */
        c = right.b;
    }
    if (p.error != NULL) {
        out->error = p.error;
        return 0;
    }
    format_equation(&x, out->variable, c, text, sizeof text);
    if (!as_given && !same_ignoring_spaces(text, given)) {
        add_step(&s, text, x.n > 0 ? "Collect the absolute values on the left and the numbers on the right"
                                   : "Collect the terms");
    }

    if (x.n == 0) {
        /* no absolute values left: a linear equation, or a statement */
        Fraction v;
        if (is_zero(x.a)) {
            int ok = fraction_equal(fraction_integer(0), c);
            add_step(&s, ok ? "all real numbers" : "no solution", ok ? "This is always true" : "This is never true");
            out->set = ok ? solution_set_all() : solution_set_none();
        } else {
            LinearKind kind = solve_linear(&s, text, "Solve it", &v);
            out->set = kind == LINEAR_ONE_SOLUTION ? point(v) : solution_set_none();
        }
    } else if (x.n == 1 && is_zero(x.a)) {
        out->set = solve_one(&s, x, c);
    } else {
        out->set = solve_by_regions(&s, &x, c);
    }

    if (s.error != NULL) {
        out->error = s.error;
        return 0;
    }
    out->ok = 1;
    solution_set_format(&out->set, out->variable, out->text, sizeof out->text);
    if (x.n > 0 && !(x.n == 1 && is_zero(x.a) && out->set.count == 0)) {
        add_step(&s, out->text, "The solutions");
    }
    return 1;
}
