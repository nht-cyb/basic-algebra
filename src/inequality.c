#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include "inequality.h"

/* ---- Comparing fractions without overflow ---- */

static int compare(Fraction a, Fraction b) {
    __int128 left, right;
    a = fraction_make(a.num, a.den);
    b = fraction_make(b.num, b.den);
    left = (__int128)a.num * b.den;
    right = (__int128)b.num * a.den;
    return left < right ? -1 : left > right ? 1 : 0;
}

/* ---- Solution sets ---- */

SolutionSet solution_set_all(void) {
    SolutionSet s;
    memset(&s, 0, sizeof s);
    s.count = 1;
    return s;
}

SolutionSet solution_set_none(void) {
    SolutionSet s;
    memset(&s, 0, sizeof s);
    return s;
}

SolutionSet solution_set_from(Relation relation, Fraction value) {
    SolutionSet s = solution_set_all();
    Interval *p = &s.parts[0];

    switch (relation) {
    case RELATION_LESS:
    case RELATION_LESS_EQUAL:
        p->has_high = 1;
        p->high = value;
        p->high_closed = relation == RELATION_LESS_EQUAL;
        break;
    case RELATION_GREATER:
    case RELATION_GREATER_EQUAL:
        p->has_low = 1;
        p->low = value;
        p->low_closed = relation == RELATION_GREATER_EQUAL;
        break;
    case RELATION_NOT_EQUAL: /* x < value or x > value */
        s.count = 2;
        p->has_high = 1;
        p->high = value;
        s.parts[1].has_low = 1;
        s.parts[1].low = value;
        break;
    }
    return s;
}

/* a comes before b: no low end first, then the smaller low end */
static int starts_before(const Interval *a, const Interval *b) {
    int c;
    if (!a->has_low || !b->has_low) return !a->has_low && b->has_low;
    c = compare(a->low, b->low);
    return c < 0 || (c == 0 && a->low_closed && !b->low_closed);
}

/* Sorts the intervals and joins those that overlap or touch. */
static SolutionSet normalize(Interval *parts, int count) {
    SolutionSet s = solution_set_none();

    for (int i = 1; i < count; i++) { /* insertion sort */
        Interval t = parts[i];
        int j = i - 1;
        while (j >= 0 && starts_before(&t, &parts[j])) {
            parts[j + 1] = parts[j];
            j--;
        }
        parts[j + 1] = t;
    }
    for (int i = 0; i < count; i++) {
        Interval *last = s.count > 0 ? &s.parts[s.count - 1] : NULL;
        Interval *next = &parts[i];
        int c;

        if (last != NULL &&
            (!last->has_high || !next->has_low ||
             (c = compare(next->low, last->high)) < 0 ||
             (c == 0 && (last->high_closed || next->low_closed)))) {
            /* they overlap or touch: keep the later end */
            if (last->has_high) {
                if (!next->has_high) {
                    last->has_high = 0;
                } else if ((c = compare(next->high, last->high)) > 0) {
                    last->high = next->high;
                    last->high_closed = next->high_closed;
                } else if (c == 0) {
                    last->high_closed |= next->high_closed;
                }
            }
            continue;
        }
        if (s.count < INEQUALITY_MAX_PARTS) s.parts[s.count++] = *next;
    }
    return s;
}

static int intersect(const Interval *a, const Interval *b, Interval *out) {
    int c;
    *out = *a;
    if (b->has_low) {
        if (!a->has_low || (c = compare(b->low, a->low)) > 0) {
            out->has_low = 1;
            out->low = b->low;
            out->low_closed = b->low_closed;
        } else if (c == 0) {
            out->low_closed = a->low_closed && b->low_closed;
        }
    }
    if (b->has_high) {
        if (!a->has_high || (c = compare(b->high, a->high)) < 0) {
            out->has_high = 1;
            out->high = b->high;
            out->high_closed = b->high_closed;
        } else if (c == 0) {
            out->high_closed = a->high_closed && b->high_closed;
        }
    }
    if (out->has_low && out->has_high) {
        c = compare(out->low, out->high);
        return c < 0 || (c == 0 && out->low_closed && out->high_closed);
    }
    return 1;
}

SolutionSet solution_set_and(const SolutionSet *a, const SolutionSet *b) {
    Interval parts[INEQUALITY_MAX_PARTS * INEQUALITY_MAX_PARTS];
    int count = 0;
    for (int i = 0; i < a->count; i++) {
        for (int j = 0; j < b->count; j++) {
            if (intersect(&a->parts[i], &b->parts[j], &parts[count])) count++;
        }
    }
    return normalize(parts, count);
}

SolutionSet solution_set_or(const SolutionSet *a, const SolutionSet *b) {
    Interval parts[2 * INEQUALITY_MAX_PARTS];
    int count = 0;
    for (int i = 0; i < a->count; i++) parts[count++] = a->parts[i];
    for (int i = 0; i < b->count; i++) parts[count++] = b->parts[i];
    return normalize(parts, count);
}

int solution_set_contains(const SolutionSet *s, Fraction x) {
    for (int i = 0; i < s->count; i++) {
        const Interval *p = &s->parts[i];
        int lo = !p->has_low || compare(x, p->low) > 0 || (p->low_closed && compare(x, p->low) == 0);
        int hi = !p->has_high || compare(x, p->high) < 0 || (p->high_closed && compare(x, p->high) == 0);
        if (lo && hi) return 1;
    }
    return 0;
}

/* ---- Writing solution sets ---- */

static const char *relation_symbol(Relation r) {
    static const char *symbols[] = { "<", "≤", ">", "≥", "≠" };
    return symbols[r];
}

/* x > 3 becomes 3 < x: what the relation is when the sides swap */
static Relation reversed(Relation r) {
    switch (r) {
    case RELATION_LESS: return RELATION_GREATER;
    case RELATION_LESS_EQUAL: return RELATION_GREATER_EQUAL;
    case RELATION_GREATER: return RELATION_LESS;
    case RELATION_GREATER_EQUAL: return RELATION_LESS_EQUAL;
    default: return r;
    }
}

typedef struct {
    char *out;
    size_t size, len;
    int overflow;
} Writer;

static void put(Writer *w, const char *s) {
    size_t n = strlen(s);
    if (w->len + n >= w->size) {
        w->overflow = 1;
        return;
    }
    memcpy(w->out + w->len, s, n + 1);
    w->len += n;
}

static void put_fraction(Writer *w, Fraction f) {
    char t[48];
    fraction_format(f, t, sizeof t);
    put(w, t);
}

int solution_set_format(const SolutionSet *s, char variable, char *out, size_t size) {
    Writer w = { out, size, 0, 0 };
    char v[2] = { variable, '\0' };

    if (size == 0) return -1;
    out[0] = '\0';
    if (s->count == 0) {
        put(&w, "no solution");
    } else if (s->count == 1 && !s->parts[0].has_low && !s->parts[0].has_high) {
        put(&w, "all real numbers");
    } else if (s->count == 2 && !s->parts[0].has_low && !s->parts[1].has_high &&
               !s->parts[0].high_closed && !s->parts[1].low_closed &&
               compare(s->parts[0].high, s->parts[1].low) == 0) {
        put(&w, v);
        put(&w, " ≠ ");
        put_fraction(&w, s->parts[0].high);
    } else {
        for (int i = 0; i < s->count; i++) {
            const Interval *p = &s->parts[i];
            if (i > 0) put(&w, " or ");
            if (p->has_low && p->has_high && compare(p->low, p->high) == 0) {
                put(&w, v);
                put(&w, " = ");
                put_fraction(&w, p->low);
                continue;
            }
            if (p->has_low && p->has_high) { /* 2 ≤ x < 4 */
                put_fraction(&w, p->low);
                put(&w, p->low_closed ? " ≤ " : " < ");
                put(&w, v);
            } else if (p->has_low) {         /* x ≥ 3 */
                put(&w, v);
                put(&w, p->low_closed ? " ≥ " : " > ");
                put_fraction(&w, p->low);
                continue;
            } else {
                put(&w, v);
            }
            put(&w, p->high_closed ? " ≤ " : " < ");
            put_fraction(&w, p->high);
        }
    }
    return w.overflow ? -1 : (int)w.len;
}

int solution_set_interval_notation(const SolutionSet *s, char *out, size_t size) {
    Writer w = { out, size, 0, 0 };

    if (size == 0) return -1;
    out[0] = '\0';
    if (s->count == 0) put(&w, "∅");
    for (int i = 0; i < s->count; i++) {
        const Interval *p = &s->parts[i];
        if (i > 0) put(&w, " ∪ ");
        if (p->has_low && p->has_high && compare(p->low, p->high) == 0) { /* x = 1 is {1} */
            put(&w, "{");
            put_fraction(&w, p->low);
            put(&w, "}");
            continue;
        }
        if (p->has_low) {
            put(&w, p->low_closed ? "[" : "(");
            put_fraction(&w, p->low);
        } else {
            put(&w, "(-∞");
        }
        put(&w, ", ");
        if (p->has_high) {
            put_fraction(&w, p->high);
            put(&w, p->high_closed ? "]" : ")");
        } else {
            put(&w, "∞)");
        }
    }
    return w.overflow ? -1 : (int)w.len;
}

/* ---- Finding the relation symbols ---- */

typedef struct {
    const char *text;
    Relation relation;
} Symbol;

static const Symbol symbols[] = {
    { "<=", RELATION_LESS_EQUAL }, { ">=", RELATION_GREATER_EQUAL }, { "!=", RELATION_NOT_EQUAL },
    { "≤", RELATION_LESS_EQUAL }, { "≥", RELATION_GREATER_EQUAL }, { "≠", RELATION_NOT_EQUAL },
    { "<", RELATION_LESS }, { ">", RELATION_GREATER },
};

/* The relation symbol at s, if there is one: sets *relation and returns
   its length in bytes, or returns 0. */
static size_t symbol_at(const char *s, Relation *relation) {
    for (size_t i = 0; i < sizeof symbols / sizeof symbols[0]; i++) {
        size_t n = strlen(symbols[i].text);
        if (strncmp(s, symbols[i].text, n) == 0) {
            *relation = symbols[i].relation;
            return n;
        }
    }
    return 0;
}

/* Finds up to max relation symbols; returns how many there are. */
static int find_symbols(const char *s, const char **where, size_t *length, Relation *relation, int max) {
    int count = 0;
    while (*s != '\0') {
        Relation r;
        size_t n = symbol_at(s, &r);
        if (n > 0) {
            if (count < max) {
                where[count] = s;
                length[count] = n;
                relation[count] = r;
            }
            count++;
            s += n;
        } else {
            s++;
        }
    }
    return count;
}

/* An = sign that is not part of <=, >= or != */
static int has_equals_sign(const char *s) {
    for (const char *p = s; *p != '\0'; p++) {
        if (*p == '=' && (p == s || (p[-1] != '<' && p[-1] != '>' && p[-1] != '!'))) return 1;
    }
    return 0;
}

/* ---- Solving ---- */

typedef struct {
    InequalitySolution *out;
    const char *error;
} Solver;

static void fail(Solver *s, const char *error) {
    if (s->error == NULL) s->error = error;
}

static void add_step(Solver *s, const char *equation, const char *action) {
    InequalitySolution *out = s->out;
    LinearStep *step;
    if (out->step_count == INEQUALITY_MAX_STEPS) return;
    step = &out->steps[out->step_count++];
    snprintf(step->equation, sizeof step->equation, "%s", equation);
    snprintf(step->action, sizeof step->action, "%s", action);
}

static void trim(const char *text, size_t len, char *out, size_t size) {
    while (len > 0 && isspace((unsigned char)*text)) { text++; len--; }
    while (len > 0 && isspace((unsigned char)text[len - 1])) len--;
    snprintf(out, size, "%.*s", (int)len, text);
}

/* The next character that is not a space, reading the two ways of
   writing each symbol as the same: >= and ≥, − and - */
static int next_symbol(const char **s) {
    static const struct { const char *text; int code; } same[] = {
        { "<=", 1 }, { "≤", 1 }, { ">=", 2 }, { "≥", 2 }, { "!=", 3 }, { "≠", 3 }, { "−", '-' },
    };
    while (isspace((unsigned char)**s)) (*s)++;
    for (size_t i = 0; i < sizeof same / sizeof same[0]; i++) {
        size_t n = strlen(same[i].text);
        if (strncmp(*s, same[i].text, n) == 0) {
            *s += n;
            return same[i].code;
        }
    }
    return (unsigned char)*(*s)++;
}

static int same_ignoring_spaces(const char *a, const char *b) {
    for (;;) {
        int ca = next_symbol(&a), cb = next_symbol(&b);
        if (ca != cb) return 0;
        if (ca == '\0') return 1;
    }
}

static Fraction negate(Fraction f) { return fraction_make(-f.num, f.den); }
static int is_zero(Fraction f) { return f.num == 0; }

/* "4x - 12 > 8" */
static void add_inequality_step(Solver *s, LinearExpr left, Relation r, LinearExpr right, const char *action) {
    char l[60], rt[60], text[128];
    linear_format(&left, &s->out->variable, 1, l, sizeof l);
    linear_format(&right, &s->out->variable, 1, rt, sizeof rt);
    snprintf(text, sizeof text, "%s %s %s", l, relation_symbol(r), rt);
    add_step(s, text, action);
}

static int holds(Fraction a, Relation r, Fraction b) {
    int c = compare(a, b);
    switch (r) {
    case RELATION_LESS: return c < 0;
    case RELATION_LESS_EQUAL: return c <= 0;
    case RELATION_GREATER: return c > 0;
    case RELATION_GREATER_EQUAL: return c >= 0;
    default: return c != 0;
    }
}

/* One linear inequality such as 2 + 3(5 - x) ≥ 38. Its text is already
   a step; this adds the steps that solve it. */
static SolutionSet solve_linear(Solver *s, const char *text) {
    const char *where[2];
    size_t length[2];
    Relation r, rels[2];
    char copy[256], action[128], num[48];
    LinearEquation eq;
    LinearExpr left, right, t;
    Fraction a;
    const char *error;
    int n = find_symbols(text, where, length, rels, 2);

    if (n == 0) {
        fail(s, has_equals_sign(text) ? "it is an equation, not an inequality" : "there is no <, >, ≤, ≥ or ≠");
        return solution_set_none();
    }
    if (n > 1 || has_equals_sign(text)) {
        fail(s, "it has more than one relation symbol");
        return solution_set_none();
    }
    r = rels[0];

    /* read it as an equation with linear_parse */
    snprintf(copy, sizeof copy, "%.*s = %s", (int)(where[0] - text), text, where[0] + length[0]);
    error = linear_parse(copy, &eq);
    if (error != NULL) {
        fail(s, error);
        return solution_set_none();
    }
    if (eq.count > 1) {
        fail(s, "there is more than one variable");
        return solution_set_none();
    }
    if (eq.count == 1) {
        if (s->out->variable != 0 && s->out->variable != eq.names[0]) {
            fail(s, "the parts use different variables");
            return solution_set_none();
        }
        s->out->variable = eq.names[0];
    } else if (s->out->variable == 0) {
        s->out->variable = 'x';
    }
    left = eq.left;
    right = eq.right;

    add_inequality_step(s, left, r, right, strchr(text, '(') ? "Use the distributive property" : "Write each side as ax + b");
    if (same_ignoring_spaces(s->out->steps[s->out->step_count - 1].equation, text)) {
        s->out->step_count--;
    }

    /* 9 > x + 3 becomes x + 3 < 9 */
    if (is_zero(left.coef[0]) && !is_zero(right.coef[0])) {
        t = left;
        left = right;
        right = t;
        r = reversed(r);
        add_inequality_step(s, left, r, right, "Swap the two sides and turn the inequality around");
    }

    /* subtract the variable term on the right from each side */
    if (!is_zero(right.coef[0])) {
        LinearExpr term = { { right.coef[0].num < 0 ? negate(right.coef[0]) : right.coef[0] }, { 0, 1 } };
        char term_text[48];
        linear_format(&term, &s->out->variable, 1, term_text, sizeof term_text);
        snprintf(action, sizeof action, "%s %s %s each side", right.coef[0].num < 0 ? "Add" : "Subtract",
                 term_text, right.coef[0].num < 0 ? "to" : "from");
        if (fraction_add(left.coef[0], negate(right.coef[0]), &left.coef[0]) != 0) {
            fail(s, "the numbers are too big");
            return solution_set_none();
        }
        right.coef[0] = fraction_integer(0);
        add_inequality_step(s, left, r, right, action);
    }

    /* no variable left: 0 > -7 is always true, 0 > 9 never */
    if (is_zero(left.coef[0])) {
        int ok = holds(left.constant, r, right.constant);
        add_step(s, ok ? "all real numbers" : "no solution",
                 ok ? "This is always true" : "This is never true");
        return ok ? solution_set_all() : solution_set_none();
    }

    /* move the number on the left to the right */
    if (!is_zero(left.constant)) {
        a = left.constant;
        fraction_format(a.num < 0 ? negate(a) : a, num, sizeof num);
        snprintf(action, sizeof action, "%s %s %s each side", a.num < 0 ? "Add" : "Subtract",
                 num, a.num < 0 ? "to" : "from");
        if (fraction_add(right.constant, negate(a), &right.constant) != 0) {
            fail(s, "the numbers are too big");
            return solution_set_none();
        }
        left.constant = fraction_integer(0);
        add_inequality_step(s, left, r, right, action);
    }

    /* divide by the coefficient, reversing the inequality if it is negative */
    a = fraction_make(left.coef[0].num, left.coef[0].den);
    if (!(a.num == 1 && a.den == 1)) {
        const char *reverse = a.num < 0 ? " and reverse the inequality" : "";
        if (a.den == 1) {
            snprintf(action, sizeof action, "Divide each side by %ld%s", a.num, reverse);
        } else {
            fraction_format(fraction_make(a.den, a.num), num, sizeof num);
            snprintf(action, sizeof action, "Multiply each side by %s%s", num, reverse);
        }
        if (fraction_divide(right.constant, a, &right.constant) != 0) {
            fail(s, "the numbers are too big");
            return solution_set_none();
        }
        left.coef[0] = fraction_integer(1);
        if (a.num < 0) r = reversed(r);
        add_inequality_step(s, left, r, right, action);
    }
    return solution_set_from(r, right.constant);
}

static SolutionSet combine(Solver *s, const SolutionSet *a, const SolutionSet *b, int use_and) {
    SolutionSet result = use_and ? solution_set_and(a, b) : solution_set_or(a, b);
    char text[96], first[48], second[48], action[160];

    solution_set_format(a, s->out->variable, first, sizeof first);
    solution_set_format(b, s->out->variable, second, sizeof second);
    solution_set_format(&result, s->out->variable, text, sizeof text);
    snprintf(action, sizeof action, "%s %s %s: keep the numbers in %s", first, use_and ? "and" : "or",
             second, use_and ? "both" : "either");
    add_step(s, text, action);
    return result;
}

static SolutionSet solve_part(Solver *s, const char *text, const char *action);

/* |x - 4| < 7: x - 4 < 7 and -(x - 4) < 7, as in the lesson */
static SolutionSet solve_absolute(Solver *s, const char *text) {
    const char *bar1 = strchr(text, '|'), *bar2 = bar1 ? strchr(bar1 + 1, '|') : NULL;
    const char *where[2];
    size_t length[2];
    Relation rels[2];
    char inside[100], right[100], case1[256], case2[256], both[520], action[512];
    int simple = 1;
    SolutionSet a, b;
    int use_and;

    for (const char *p = text; p < bar1; p++) {
        if (!isspace((unsigned char)*p)) bar2 = NULL; /* something before |...| */
    }
    if (bar2 == NULL || find_symbols(bar2, where, length, rels, 2) != 1 ||
        strchr(bar2 + 1, '|') != NULL || find_symbols(bar1, where, length, rels, 2) != 1) {
        fail(s, "write an absolute value inequality as |expression| < number");
        return solution_set_none();
    }
    for (const char *p = bar2 + 1; p < where[0]; p++) {
        if (!isspace((unsigned char)*p)) {
            fail(s, "write an absolute value inequality as |expression| < number");
            return solution_set_none();
        }
    }
    if (rels[0] == RELATION_NOT_EQUAL) {
        fail(s, "≠ with an absolute value is not supported");
        return solution_set_none();
    }
    trim(bar1 + 1, (size_t)(bar2 - bar1 - 1), inside, sizeof inside);
    trim(where[0] + length[0], strlen(where[0] + length[0]), right, sizeof right);
    if (strchr(right, '|') != NULL) {
        fail(s, "write an absolute value inequality as |expression| < number");
        return solution_set_none();
    }

    /* |A| < B means A < B and -A < B; |A| > B means A > B or -A > B */
    use_and = rels[0] == RELATION_LESS || rels[0] == RELATION_LESS_EQUAL;
    snprintf(case1, sizeof case1, "%s %s %s", inside, relation_symbol(rels[0]), right);
    for (const char *c = inside; *c != '\0'; c++) { /* -x needs no brackets, -(x - 4) does */
        if (!isalnum((unsigned char)*c) && *c != '.') simple = 0;
    }
    snprintf(case2, sizeof case2, simple ? "-%s %s %s" : "-(%s) %s %s", inside, relation_symbol(rels[0]), right);
    snprintf(both, sizeof both, "%s %s %s", case1, use_and ? "and" : "or", case2);
    snprintf(action, sizeof action, simple ? "|%s| is %s when %s is positive and -%s when it is negative"
                                           : "|%s| is %s when %s is positive and -(%s) when it is negative",
             inside, inside, inside, inside);
    add_step(s, both, action);

    a = solve_part(s, case1, "Case 1: solve the first inequality");
    b = solve_part(s, case2, "Case 2: solve the second inequality");
    return combine(s, &a, &b, use_and);
}

/* One part: an absolute value inequality, -8 < x < 8, or a linear one */
static SolutionSet solve_part(Solver *s, const char *text, const char *action) {
    const char *where[3];
    size_t length[3];
    Relation rels[3];
    char trimmed[256], first[256], second[256];
    SolutionSet a, b;

    trim(text, strlen(text), trimmed, sizeof trimmed);
    if (action != NULL) add_step(s, trimmed, action);
    if (strchr(trimmed, '|') != NULL) {
        return solve_absolute(s, trimmed);
    }
    if (find_symbols(trimmed, where, length, rels, 3) == 2) {
        /* -8 < x < 8 is -8 < x and x < 8 */
        snprintf(first, sizeof first, "%.*s", (int)(where[1] - trimmed), trimmed);
        snprintf(second, sizeof second, "%s", where[0] + length[0]);
        a = solve_part(s, first, "Solve the left part");
        b = solve_part(s, second, "Solve the right part");
        return combine(s, &a, &b, 1);
    }
    return solve_linear(s, trimmed);
}

/* Finds " and " or " or " outside |...|; returns its position or NULL. */
static const char *find_word(const char *s, const char *word, size_t *length) {
    size_t n = strlen(word);
    for (const char *p = s; *p != '\0'; p++) {
        if (p > s && isspace((unsigned char)p[-1]) && strncasecmp(p, word, n) == 0 &&
            (p[n] == '\0' || isspace((unsigned char)p[n]))) {
            *length = n;
            return p;
        }
    }
    return NULL;
}

int inequality_solve(const char *inequality, InequalitySolution *out) {
    Solver s = { out, NULL };
    char text[256], part[256];
    const char *and_at, *or_at, *at, *rest;
    size_t and_len = 0, or_len = 0, len;
    SolutionSet result, next;
    int use_and, number = 1;
    char action[64];

    memset(out, 0, sizeof *out);
    trim(inequality, strlen(inequality), text, sizeof text);
    add_step(&s, text, "");

    and_at = find_word(text, "and", &and_len);
    or_at = find_word(text, "or", &or_len);
    if (and_at != NULL && or_at != NULL) {
        out->error = "it mixes and with or, so it is not clear which goes first";
        return 0;
    }

    if (and_at == NULL && or_at == NULL) {
        result = solve_part(&s, text, NULL);
    } else {
        /* x ≥ 2 and x < 4: solve each part, then combine them */
        use_and = and_at != NULL;
        at = use_and ? and_at : or_at;
        len = use_and ? and_len : or_len;
        snprintf(part, sizeof part, "%.*s", (int)(at - text), text);
        result = solve_part(&s, part, "Solve the first inequality");
        rest = at + len;
        while (s.error == NULL) {
            const char *next_at = find_word(rest, use_and ? "and" : "or", &len);
            number++;
            snprintf(part, sizeof part, "%.*s", next_at ? (int)(next_at - rest) : (int)strlen(rest), rest);
            snprintf(action, sizeof action, "Solve inequality %d", number);
            next = solve_part(&s, part, number == 2 ? "Solve the second inequality" : action);
            result = combine(&s, &result, &next, use_and);
            if (next_at == NULL) break;
            rest = next_at + len;
        }
    }

    if (s.error != NULL) {
        out->error = s.error;
        return 0;
    }
    out->ok = 1;
    out->set = result;
    if (out->variable == 0) out->variable = 'x';
    solution_set_format(&result, out->variable, out->text, sizeof out->text);
    return 1;
}

/* ---- Inequalities in two variables ---- */

const char *plane_inequality_read(const char *text, PlaneInequality *p) {
    const char *where[2];
    size_t length[2];
    Relation rels[2];
    char copy[256];
    LinearEquation eq;
    const char *error;
    Fraction a, b, c;
    int xi, yi;

    memset(p, 0, sizeof *p);
    if (find_symbols(text, where, length, rels, 2) != 1 || has_equals_sign(text)) {
        return "write it with one of <, >, ≤, ≥, for example y > 2x + 1";
    }
    if (rels[0] == RELATION_NOT_EQUAL) {
        return "≠ is not supported for graphing in two variables";
    }
    snprintf(copy, sizeof copy, "%.*s = %s", (int)(where[0] - text), text, where[0] + length[0]);
    if ((error = linear_parse(copy, &eq)) != NULL) return error;
    if (eq.count != 2) return "it needs two variables, such as x and y";

    /* y is the vertical axis: the one named y, or else the later letter */
    yi = eq.names[0] == 'y' || (eq.names[1] != 'y' && eq.names[0] > eq.names[1]) ? 0 : 1;
    xi = 1 - yi;
    p->x = eq.names[xi];
    p->y = eq.names[yi];

    /* a·x + b·y relation c */
    if (fraction_add(eq.left.coef[xi], negate(eq.right.coef[xi]), &a) != 0 ||
        fraction_add(eq.left.coef[yi], negate(eq.right.coef[yi]), &b) != 0 ||
        fraction_add(eq.right.constant, negate(eq.left.constant), &c) != 0) {
        return "the numbers are too big";
    }
    p->relation = rels[0];
    if (is_zero(b)) {
        if (is_zero(a)) return "the variables cancel out";
        p->vertical = 1;
        if (fraction_divide(c, a, &p->c) != 0) return "the numbers are too big";
        if (a.num < 0) p->relation = reversed(p->relation);
        return NULL;
    }
    /* y relation (-a/b)x + c/b, reversed when b is negative */
    if (fraction_divide(negate(a), b, &p->m) != 0 || fraction_divide(c, b, &p->b) != 0) {
        return "the numbers are too big";
    }
    if (b.num < 0) p->relation = reversed(p->relation);
    return NULL;
}

int plane_inequality_check(const PlaneInequality *p, Fraction x, Fraction y) {
    Fraction mx, value;
    if (p->vertical) return holds(x, p->relation, p->c);
    if (fraction_multiply(p->m, x, &mx) != 0 || fraction_add(mx, p->b, &value) != 0) return 0;
    return holds(y, p->relation, value);
}

int plane_inequality_describe(const PlaneInequality *p, char *out, size_t size) {
    LinearExpr line = { { p->m, { 0, 1 } }, p->b };
    char boundary[64], num[48];
    int strict = p->relation == RELATION_LESS || p->relation == RELATION_GREATER;
    int greater = p->relation == RELATION_GREATER || p->relation == RELATION_GREATER_EQUAL;
    int n;

    if (p->vertical) {
        fraction_format(p->c, num, sizeof num);
        n = snprintf(out, size, "Draw %c = %s as a %s line (%s) and shade to the %s of it",
                     p->x, num, strict ? "dashed" : "solid",
                     strict ? "it is not included" : "it is included", greater ? "right" : "left");
    } else {
        linear_format(&line, &p->x, 1, boundary, sizeof boundary);
        n = snprintf(out, size, "Draw %c = %s as a %s line (%s) and shade %s it",
                     p->y, boundary, strict ? "dashed" : "solid",
                     strict ? "it is not included" : "it is included", greater ? "above" : "below");
    }
    return n < 0 || (size_t)n >= size ? -1 : n;
}
