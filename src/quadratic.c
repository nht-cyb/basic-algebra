#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "quadratic.h"
#include "root.h"

/* ---- Fractions, noting when one does not fit in a long ---- */

static int too_big; /* reset at the start of each public function */

static Fraction add(Fraction x, Fraction y) {
    Fraction r = { 0, 1 };
    if (fraction_add(x, y, &r) != 0) too_big = 1;
    return r;
}

static Fraction mul(Fraction x, Fraction y) {
    Fraction r = { 0, 1 };
    if (fraction_multiply(x, y, &r) != 0) too_big = 1;
    return r;
}

static Fraction quo(Fraction x, Fraction y) {
    Fraction r = { 0, 1 };
    if (fraction_divide(x, y, &r) != 0) too_big = 1;
    return r;
}

static Fraction neg(Fraction f) { return fraction_make(-f.num, f.den); }
static Fraction whole(long n) { return fraction_integer(n); }
static int is_zero(Fraction f) { return f.num == 0; }
static int is_one(Fraction f) { return f.num == f.den; }

/* ---- Reading: polynomials up to x², as c[0] + c[1]x + c[2]x² ---- */

typedef struct {
    Fraction c[3];
} Poly;

typedef struct {
    const char *s;
    char variable;
    const char *error;
} Parser;

static const Poly zero_poly = { { { 0, 1 }, { 0, 1 }, { 0, 1 } } };

static void fail(Parser *p, const char *error) {
    if (p->error == NULL) p->error = error;
}

static Poly poly_add(Poly x, Poly y) {
    for (int i = 0; i < 3; i++) x.c[i] = add(x.c[i], y.c[i]);
    return x;
}

static Poly poly_negate(Poly x) {
    for (int i = 0; i < 3; i++) x.c[i] = neg(x.c[i]);
    return x;
}

static Poly poly_mul(Parser *p, Poly x, Poly y) {
    Poly r = zero_poly;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (is_zero(x.c[i]) || is_zero(y.c[j])) continue;
            if (i + j > 2) {
                fail(p, "not quadratic: the power of the variable is more than 2");
                return r;
            }
            r.c[i + j] = add(r.c[i + j], mul(x.c[i], y.c[j]));
        }
    }
    return r;
}

static Poly poly_div(Parser *p, Poly x, Poly y) {
    if (!is_zero(y.c[1]) || !is_zero(y.c[2])) {
        fail(p, "it divides by the variable");
        return zero_poly;
    }
    if (is_zero(y.c[0])) {
        fail(p, "it divides by 0");
        return zero_poly;
    }
    for (int i = 0; i < 3; i++) x.c[i] = quo(x.c[i], y.c[0]);
    return x;
}

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
static const char *const power_signs[] = { "^", NULL };
static const char *const squared_signs[] = { "²", NULL };
static const char *const open_bracket[] = { "(", NULL };
static const char *const close_bracket[] = { ")", NULL };

static Poly parse_expr(Parser *p);

static Poly parse_number(Parser *p) {
    Poly x = zero_poly;
    Fraction value = { 0, 1 };
    long scale = 1;
    int after_point = 0;

    for (; isdigit((unsigned char)*p->s) || (*p->s == '.' && !after_point); p->s++) {
        if (*p->s == '.') {
            after_point = 1;
            continue;
        }
        value = add(mul(value, whole(10)), whole(*p->s - '0'));
        if (after_point && __builtin_mul_overflow(scale, 10, &scale)) too_big = 1;
    }
    x.c[0] = fraction_make(value.num, value.den * scale);
    return x;
}

/* a number, the variable or a bracket */
static Poly parse_atom(Parser *p) {
    Poly x = zero_poly;

    if (accept(p, open_bracket)) {
        x = parse_expr(p);
        if (!accept(p, close_bracket)) fail(p, "a bracket is not closed");
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
        x.c[1] = whole(1);
        return x;
    }
    fail(p, *p->s == '\0' || *p->s == '=' ? "a number or variable is missing"
                                          : "it has a symbol that is not understood");
    return x;
}

/* an atom with an optional power: x^2, x², (x + 3)^2 */
static Poly parse_power(Parser *p) {
    Poly base = parse_atom(p), x;
    long n = 0;

    if (accept(p, squared_signs)) {
        n = 2;
    } else if (accept(p, power_signs)) {
        skip_spaces(p);
        if (!isdigit((unsigned char)*p->s)) {
            fail(p, "a power must be a whole number");
            return base;
        }
        while (isdigit((unsigned char)*p->s)) {
            n = n * 10 + (*p->s++ - '0');
            if (n > 64) {
                fail(p, "the power is too big");
                return base;
            }
        }
    } else {
        return base;
    }
    x = zero_poly;
    x.c[0] = whole(1);
    for (long i = 0; i < n && p->error == NULL; i++) x = poly_mul(p, x, base);
    return x;
}

/* -x² is -(x²): the sign applies after the power */
static Poly parse_factor(Parser *p) {
    if (accept(p, minus_signs)) return poly_negate(parse_factor(p));
    if (accept(p, plus_signs)) return parse_factor(p);
    return parse_power(p);
}

static Poly parse_term(Parser *p) {
    Poly x = parse_factor(p);
    while (p->error == NULL) {
        if (accept(p, times_signs)) {
            x = poly_mul(p, x, parse_factor(p));
        } else if (accept(p, divide_signs)) {
            x = poly_div(p, x, parse_factor(p));
        } else if (skip_spaces(p), isalpha((unsigned char)*p->s) || *p->s == '(') {
            x = poly_mul(p, x, parse_factor(p));
        } else {
            break;
        }
    }
    return x;
}

static Poly parse_expr(Parser *p) {
    Poly x = parse_term(p);
    while (p->error == NULL) {
        if (accept(p, plus_signs)) {
            x = poly_add(x, parse_term(p));
        } else if (accept(p, minus_signs)) {
            x = poly_add(x, poly_negate(parse_term(p)));
        } else {
            break;
        }
    }
    return x;
}

const char *quadratic_read(const char *text, Quadratic *q) {
    Parser p = { text, 0, NULL };
    Poly left, right = zero_poly;

    too_big = 0;
    left = parse_expr(&p);
    skip_spaces(&p);
    if (p.error == NULL && *p.s == '=') {
        p.s++;
        right = parse_expr(&p);
        skip_spaces(&p);
    }
    if (p.error == NULL && *p.s != '\0') {
        fail(&p, *p.s == '=' ? "there is more than one = sign" : "it has a symbol that is not understood");
    }
    if (p.error == NULL && too_big) fail(&p, "the numbers are too big");
    if (p.error != NULL) return p.error;
    if (p.variable == 0) return "there is no variable to solve for";

    left = poly_add(left, poly_negate(right)); /* everything on the left: ... = 0 */
    if (too_big) return "the numbers are too big";
    if (is_zero(left.c[2])) return "not quadratic: there is no squared term, so it is linear";
    q->variable = p.variable;
    q->a = left.c[2];
    q->b = left.c[1];
    q->c = left.c[0];
    return NULL;
}

int quadratic_discriminant(const Quadratic *q, Fraction *out) {
    too_big = 0;
    *out = add(mul(q->b, q->b), neg(mul(whole(4), mul(q->a, q->c))));
    return too_big ? -1 : 0;
}

int quadratic_vertex(const Quadratic *q, Fraction *x, Fraction *y) {
    too_big = 0;
    *x = quo(neg(q->b), mul(whole(2), q->a));
    *y = add(add(mul(q->a, mul(*x, *x)), mul(q->b, *x)), q->c);
    if (too_big) return -1;
    return q->a.num > 0 ? 1 : 0;
}

/* ---- Writing ---- */

static void frac(Fraction f, char *out, size_t size) {
    fraction_format(f, out, size);
}

/* a fraction in brackets when it is negative or not whole: (-11), (2/5) */
static void frac_wrapped(Fraction f, char *out, size_t size) {
    char t[48];
    f = fraction_make(f.num, f.den);
    frac(f, t, sizeof t);
    if (f.num < 0 || f.den != 1) {
        snprintf(out, size, "(%s)", t);
    } else {
        snprintf(out, size, "%s", t);
    }
}

/* always in brackets: (4), (-3) */
static void frac_in_brackets(Fraction f, char *out, size_t size) {
    char t[48];
    frac(f, t, sizeof t);
    snprintf(out, size, "(%s)", t);
}

/* c[2]x² + c[1]x + c[0], e.g. "x² - 9x + 14", "(1/50000)x² - (1/25)x + 38" */
static void format_poly(const Fraction c[3], char variable, char *out, size_t size) {
    char num[48];
    size_t len = 0;
    int first = 1;

    out[0] = '\0';
    for (int i = 2; i >= 0; i--) {
        Fraction f = fraction_make(c[i].num, c[i].den);
        int negative = f.num < 0;
        const char *power = i == 2 ? "²" : "";

        if (f.num == 0 && !(i == 0 && first)) continue;
        if (negative) f.num = -f.num;
        if (i > 0 && f.num == f.den) {
            num[0] = '\0';
        } else if (i > 0 && f.den != 1) {
            fraction_format(f, num + 1, sizeof num - 2);
            num[0] = '(';
            strcat(num, ")");
        } else {
            frac(f, num, sizeof num);
        }
        len += (size_t)snprintf(out + len, len < size ? size - len : 0, "%s%s",
                                first ? (negative ? "-" : "") : (negative ? " - " : " + "), num);
        if (i > 0 && len < size) {
            len += (size_t)snprintf(out + len, size - len, "%c%s", variable, power);
        }
        first = 0;
        if (len >= size) return;
    }
}

/* k·√r / d, times i if imaginary: "6", "5/3", "2√314", "3i", "i√2", "(5√2/3)" */
static void sqrt_text(long k, long r, long d, int imaginary, char *out, size_t size) {
    char t[64] = "";
    size_t len = 0;
    Fraction kd = fraction_make(k, d); /* √225 / 9 = 15/9 = 5/3 */

    k = kd.num;
    d = kd.den;
    if (!(k == 1 && (r > 1 || imaginary))) len += (size_t)snprintf(t + len, sizeof t - len, "%ld", k);
    if (imaginary) len += (size_t)snprintf(t + len, sizeof t - len, "i");
    if (r > 1) snprintf(t + len, sizeof t - len, "√%ld", r);
    if (d != 1 && (r > 1 || imaginary)) {
        snprintf(out, size, "(%s/%ld)", t, d);
    } else if (d != 1) {
        snprintf(out, size, "%s/%ld", t, d);
    } else {
        snprintf(out, size, "%s", t);
    }
}

/* "x = -4 ± 3i", "x = ±√314", "x = 3/2 ± (√5/2)" */
static void plus_minus_text(const QuadraticSolution *s, char *out, size_t size) {
    char p[48] = "", q[64];
    Fraction qc = fraction_make(s->q_coef.num, s->q_coef.den);

    sqrt_text(qc.num, s->r, qc.den, s->kind == QUADRATIC_TWO_COMPLEX, q, sizeof q);
    if (!is_zero(s->p)) {
        frac(s->p, p, sizeof p);
        snprintf(out, size, "%c = %s ± %s", s->q.variable, p, q);
    } else {
        snprintf(out, size, "%c = ±%s", s->q.variable, q);
    }
}

static void add_step(QuadraticSolution *s, const char *equation, const char *action) {
    LinearStep *step;
    if (s->step_count == QUADRATIC_MAX_STEPS) return;
    step = &s->steps[s->step_count++];
    snprintf(step->equation, sizeof step->equation, "%s", equation);
    snprintf(step->action, sizeof step->action, "%s", action);
}

/* the next character that is not a space or ^, reading ² as 2 and − as - */
static char next_char(const char **s) {
    while (isspace((unsigned char)**s) || **s == '^') (*s)++;
    if (strncmp(*s, "²", strlen("²")) == 0) {
        *s += strlen("²");
        return '2';
    }
    if (strncmp(*s, "−", strlen("−")) == 0) {
        *s += strlen("−");
        return '-';
    }
    return *(*s)++;
}

static int same_text(const char *a, const char *b) {
    for (;;) {
        char ca = next_char(&a), cb = next_char(&b);
        if (ca != cb) return 0;
        if (ca == '\0') return 1;
    }
}

/* ---- The solutions: x = p ± q√r ---- */

static void find_solutions(QuadraticSolution *s) {
    Quadratic *q = &s->q;
    Fraction two_a = mul(whole(2), q->a), d = s->discriminant, sq;
    long n, outside, inside;
    double root_d;

    s->p = quo(neg(q->b), two_a);
    s->r = 1;
    if (is_zero(d)) {
        s->kind = QUADRATIC_ONE_REAL;
        s->q_coef = whole(0);
        s->rational = 1;
        s->roots[0] = s->roots[1] = s->p;
        s->approx[0] = s->approx[1] = (double)s->p.num / (double)s->p.den;
        snprintf(s->text, sizeof s->text, "%c = ", q->variable);
        frac(s->p, s->text + 4, sizeof s->text - 4);
        return;
    }

    /* √(n/m) = √(n·m)/m = outside·√inside / m */
    s->kind = d.num > 0 ? QUADRATIC_TWO_REAL : QUADRATIC_TWO_COMPLEX;
    if (__builtin_mul_overflow(d.num < 0 ? -d.num : d.num, d.den, &n)) {
        too_big = 1;
        return;
    }
    root_simplify(n, &outside, &inside);
    s->r = inside;
    sq = fraction_make(outside, d.den);                  /* √|Δ| = sq·√r */
    s->q_coef = quo(sq, two_a.num < 0 ? neg(two_a) : two_a);

    if (s->kind == QUADRATIC_TWO_REAL) {
        root_d = sqrt((double)d.num / (double)d.den);
        s->approx[0] = (-(double)q->b.num / q->b.den + root_d) / ((double)two_a.num / two_a.den);
        s->approx[1] = (-(double)q->b.num / q->b.den - root_d) / ((double)two_a.num / two_a.den);
        if (inside == 1) {
            char r0[48], r1[48];
            s->rational = 1;
            s->roots[0] = quo(add(neg(q->b), sq), two_a);
            s->roots[1] = quo(add(neg(q->b), neg(sq)), two_a);
            frac(s->roots[0], r0, sizeof r0);
            frac(s->roots[1], r1, sizeof r1);
            snprintf(s->text, sizeof s->text, "%c = %s or %c = %s", q->variable, r0, q->variable, r1);
            return;
        }
    }
    plus_minus_text(s, s->text, sizeof s->text);
}

/* ---- The three methods ---- */

static void solve_by_formula(QuadraticSolution *s) {
    Quadratic *q = &s->q;
    char a[48], b[48], c[48], d[48], minus_b[48], two_a[48], root[64], text[256];
    Fraction da = s->discriminant;
    long n = 0, outside = 1, inside = 1;

    frac(q->a, a, sizeof a);
    frac(q->b, b, sizeof b);
    frac(q->c, c, sizeof c);
    snprintf(text, sizeof text, "a = %s, b = %s, c = %s", a, b, c);
    add_step(s, text, "Identify a, b and c");

    /* Δ = (-11)² - 4(4)(-3) = 169 */
    frac_wrapped(q->b, b, sizeof b);
    frac_in_brackets(q->a, a, sizeof a);
    frac_in_brackets(q->c, c, sizeof c);
    frac(da, d, sizeof d);
    snprintf(text, sizeof text, "Δ = %s² - 4%s%s = %s", b, a, c, d);
    add_step(s, text, "Find the discriminant b² - 4ac");

    frac(neg(q->b), minus_b, sizeof minus_b);
    frac_wrapped(mul(whole(2), q->a), two_a, sizeof two_a);
    frac_wrapped(da, d, sizeof d);
    snprintf(text, sizeof text, "%c = (%s ± √%s) / %s", q->variable, minus_b, d, two_a);
    add_step(s, text, "Use the quadratic formula (-b ± √Δ) / 2a");

    if (!is_zero(da) && !__builtin_mul_overflow(da.num < 0 ? -da.num : da.num, da.den, &n)) {
        root_simplify(n, &outside, &inside);
        if (outside != 1 || inside != n) { /* √36 = 6, √1256 = 2√314, √(-36) = 6i */
            char action[192];
            sqrt_text(outside, inside, da.den, da.num < 0, root, sizeof root);
            snprintf(text, sizeof text, "%c = (%s ± %s) / %s", q->variable, minus_b, root, two_a);
            snprintf(action, sizeof action, "√%s = %s", d, root);
            add_step(s, text, action);
        }
    }
    add_step(s, s->text, s->kind == QUADRATIC_ONE_REAL ? "Δ = 0, so there is one solution"
                         : s->rational ? "Work out x₁ with + and x₂ with -" : "Simplify");
}

static void solve_by_completing_the_square(QuadraticSolution *s) {
    Quadratic *q = &s->q;
    Fraction c[3], h, k, b_coef = quo(q->b, q->a), c_coef = quo(q->c, q->a);
    char text[256], action[192], left[64], num[48], num2[48], root[64];
    char v = q->variable;
    long n, outside, inside;

    /* 3x² + 8x - 3 = 0 becomes x² + (8/3)x - 1 = 0 */
    c[2] = whole(1);
    c[1] = b_coef;
    c[0] = c_coef;
    if (!is_one(q->a)) {
        format_poly(c, v, left, sizeof left);
        snprintf(text, sizeof text, "%s = 0", left);
        frac(q->a, num, sizeof num);
        snprintf(action, sizeof action, "Divide each side by %s", num);
        add_step(s, text, action);
    }

    /* x² + 6x = -8 */
    k = neg(c_coef);
    c[0] = whole(0);
    format_poly(c, v, left, sizeof left);
    if (!is_zero(c_coef)) {
        frac(k, num, sizeof num);
        snprintf(text, sizeof text, "%s = %s", left, num);
        frac(c_coef.num < 0 ? k : c_coef, num2, sizeof num2);
        snprintf(action, sizeof action, "%s %s %s each side", c_coef.num < 0 ? "Add" : "Subtract",
                 num2, c_coef.num < 0 ? "to" : "from");
        add_step(s, text, action);
    }

    /* add (b/2)² to each side: (x + 3)² = 1 */
    h = quo(b_coef, whole(2));
    if (!is_zero(b_coef)) {
        Fraction h2 = mul(h, h);
        k = add(k, h2);
        c[0] = h2;
        format_poly(c, v, left, sizeof left);
        frac(k, num, sizeof num);
        snprintf(text, sizeof text, "%s = %s", left, num);
        frac_wrapped(b_coef, num, sizeof num);
        frac(h2, num2, sizeof num2);
        snprintf(action, sizeof action, "Add (%s/2)² = %s to each side", num, num2);
        add_step(s, text, action);

        frac(h.num < 0 ? neg(h) : h, num2, sizeof num2);
        snprintf(left, sizeof left, "(%c %c %s)²", v, h.num < 0 ? '-' : '+', num2);
        frac(k, num, sizeof num);
        snprintf(text, sizeof text, "%s = %s", left, num);
        add_step(s, text, "Write the left side as a square");
        snprintf(left, sizeof left, "%c %c %s", v, h.num < 0 ? '-' : '+', num2);
    } else {
        snprintf(left, sizeof left, "%c", v);
    }

    /* x + 3 = ±1 */
    if (is_zero(k)) {
        snprintf(text, sizeof text, "%s = 0", left);
    } else if (__builtin_mul_overflow(k.num < 0 ? -k.num : k.num, k.den, &n)) {
        too_big = 1;
        return;
    } else {
        root_simplify(n, &outside, &inside);
        sqrt_text(outside, inside, k.den, k.num < 0, root, sizeof root);
        snprintf(text, sizeof text, "%s = ±%s", left, root);
    }
    add_step(s, text, "Take the square root of each side");

    if (!is_zero(h)) {
        frac(h.num < 0 ? neg(h) : h, num, sizeof num);
        snprintf(action, sizeof action, "%s %s %s each side", h.num < 0 ? "Add" : "Subtract",
                 num, h.num < 0 ? "to" : "from");
        add_step(s, s->text, action);
    }
}

/* the factor that is 0 at x = root: (2x + 5) for -5/2, x for 0 */
static void factor_text(Fraction root, char v, int brackets, char *out, size_t size) {
    char coef[32] = "";
    root = fraction_make(root.num, root.den);
    if (root.den != 1) snprintf(coef, sizeof coef, "%ld", root.den);
    if (root.num == 0) {
        snprintf(out, size, "%s%c", coef, v);
    } else {
        snprintf(out, size, "%s%s%c %c %ld%s", brackets ? "(" : "", coef, v,
                 root.num < 0 ? '+' : '-', root.num < 0 ? -root.num : root.num, brackets ? ")" : "");
    }
}

static void solve_by_factoring(QuadraticSolution *s) {
    Quadratic *q = &s->q;
    Fraction scale, a, b, c, lead, first, second, t, k;
    char text[256], action[128], num[48], f1[48], f2[48], prefix[32] = "";
    Fraction cs[3];
    long lcm = 1, g;
    char v = q->variable;

    if (!s->rational) {
        s->kind = QUADRATIC_INVALID;
        s->error = s->discriminant.num < 0
                       ? "it cannot be factored: it has no real solutions"
                       : "it cannot be factored with whole numbers: use the quadratic formula";
        return;
    }

    /* whole-number coefficients with a positive x² term:
       -w² + 9w - 14 = 0 becomes w² - 9w + 14 = 0 */
    Fraction coefs[3] = { q->a, q->b, q->c };
    for (int i = 0; i < 3; i++) {
        long den = fraction_make(coefs[i].num, coefs[i].den).den, x = lcm, y = den;
        while (y != 0) { long r = x % y; x = y; y = r; }
        g = x;
        if (__builtin_mul_overflow(lcm / g, den, &lcm)) { too_big = 1; return; }
    }
    scale = whole(q->a.num < 0 ? -lcm : lcm);
    a = mul(q->a, scale);
    b = mul(q->b, scale);
    c = mul(q->c, scale);
    if (!is_one(scale)) {
        cs[2] = a; cs[1] = b; cs[0] = c;
        format_poly(cs, v, text, sizeof text - 4);
        strcat(text, " = 0");
        frac(scale, num, sizeof num);
        snprintf(action, sizeof action, "Multiply each side by %s", num);
        add_step(s, text, action);
    }

    /* the factors come from the solutions: x = -2 gives (x + 2) */
    first = s->roots[0];
    second = s->roots[1];
    if ((double)first.num / first.den > (double)second.num / second.den) { t = first; first = second; second = t; }
    if (first.num == 0 && second.num != 0) { t = first; first = second; second = t; }
    if (second.num == 0) { t = first; first = second; second = t; } /* x first: x(x + 3) */
    lead = mul(whole(fraction_make(first.num, first.den).den), whole(fraction_make(second.num, second.den).den));
    k = quo(a, lead);
    if (k.num == -1 && k.den == 1) snprintf(prefix, sizeof prefix, "-");
    else if (!is_one(k)) frac(k, prefix, sizeof prefix);

    factor_text(first, v, 1, f1, sizeof f1);
    factor_text(second, v, 1, f2, sizeof f2);
    if (s->kind == QUADRATIC_ONE_REAL) {
        snprintf(text, sizeof text, "%s%s² = 0", prefix, f1);
    } else {
        snprintf(text, sizeof text, "%s%s%s = 0", prefix, f1, f2);
    }
    if (is_one(a) && b.den == 1 && c.den == 1) {
        /* x² + 3x + 2: 2 and 1 multiply to 2 and add to 3 */
        snprintf(action, sizeof action, "%ld and %ld multiply to %ld and add to %ld",
                 -first.num, -second.num, c.num, b.num);
    } else {
        snprintf(action, sizeof action, "Factor the left side");
    }
    add_step(s, text, action);

    factor_text(first, v, 0, f1, sizeof f1);
    factor_text(second, v, 0, f2, sizeof f2);
    if (s->kind == QUADRATIC_ONE_REAL) {
        snprintf(text, sizeof text, "%s = 0", f1);
        add_step(s, text, "The factor must be 0");
    } else {
        snprintf(text, sizeof text, "%s = 0 or %s = 0", f1, f2);
        add_step(s, text, "A product is 0 only when one of its factors is 0");
    }

    frac(first, f1, sizeof f1);
    frac(second, f2, sizeof f2);
    if (s->kind == QUADRATIC_ONE_REAL) {
        snprintf(text, sizeof text, "%c = %s", v, f1);
    } else {
        snprintf(text, sizeof text, "%c = %s or %c = %s", v, f1, v, f2);
    }
    add_step(s, text, "Solve each one");
}

QuadraticKind quadratic_solve(const char *equation, QuadraticMethod method, QuadraticSolution *s) {
    const char *error;
    char text[256], left[96];
    Fraction cs[3];
    size_t len;

    memset(s, 0, sizeof *s);
    error = quadratic_read(equation, &s->q);
    if (error == NULL && quadratic_discriminant(&s->q, &s->discriminant) != 0) {
        error = "the numbers are too big";
    }
    if (error != NULL) {
        s->kind = QUADRATIC_INVALID;
        s->error = error;
        return s->kind;
    }

    too_big = 0;
    while (isspace((unsigned char)*equation)) equation++;
    len = strlen(equation);
    while (len > 0 && isspace((unsigned char)equation[len - 1])) len--;
    snprintf(text, sizeof text, "%.*s", (int)len, equation);
    add_step(s, text, "");

    cs[2] = s->q.a; cs[1] = s->q.b; cs[0] = s->q.c;
    format_poly(cs, s->q.variable, left, sizeof left);
    snprintf(text, sizeof text, "%s = 0", left);
    if (!same_text(text, s->steps[0].equation)) {
        add_step(s, text, "Write in standard form ax² + bx + c = 0");
    }

    find_solutions(s);
    if (!too_big) {
        switch (method) {
        case QUADRATIC_FACTORING: solve_by_factoring(s); break;
        case QUADRATIC_COMPLETING_THE_SQUARE: solve_by_completing_the_square(s); break;
        default: solve_by_formula(s); break;
        }
    }
    if (too_big) {
        s->kind = QUADRATIC_INVALID;
        s->error = "the numbers are too big";
    }
    return s->kind;
}
