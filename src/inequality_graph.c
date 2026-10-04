#include <math.h>
#include <stdio.h>
#include <string.h>
#include "inequality.h"

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

static double value_of(Fraction f) {
    return (double)f.num / (double)f.den;
}

/* ---- The number line ---- */

#define CELLS 4 /* characters between two tick marks */

static int in_set(const SolutionSet *s, double x) {
    for (int i = 0; i < s->count; i++) {
        const Interval *p = &s->parts[i];
        if ((!p->has_low || x > value_of(p->low)) && (!p->has_high || x < value_of(p->high))) return 1;
    }
    return 0;
}

int solution_set_graph(const SolutionSet *s, char *out, size_t size) {
    Writer w = { out, size, 0, 0 };
    double lo = 0, hi = 0;
    long step = 1, first, last, ticks, width;
    int have = 0;
    char marker[1024] = { 0 }, labels[1100];
    size_t label_end = 0;

    if (size == 0) return -1;
    out[0] = '\0';

    /* the window: the ends of the intervals, with room around them */
    for (int i = 0; i < s->count; i++) {
        const Interval *p = &s->parts[i];
        if (p->has_low) {
            double v = value_of(p->low);
            lo = have ? fmin(lo, v) : v;
            hi = have ? fmax(hi, v) : v;
            have = 1;
        }
        if (p->has_high) {
            double v = value_of(p->high);
            lo = have ? fmin(lo, v) : v;
            hi = have ? fmax(hi, v) : v;
            have = 1;
        }
    }
    if (!have) {
        lo = hi = 0;
    }
    lo = floor(lo) - 3;
    hi = ceil(hi) + 3;
    if (hi - lo < 10) {
        double extra = 10 - (hi - lo);
        lo -= floor(extra / 2);
        hi += extra - floor(extra / 2);
    }
    /* at most 20 ticks: 1, 2, 5, 10, 20, 50, ... apart */
    for (long power = 1; (hi - lo) / step > 20 && power < 1000000000000000L; power *= 10) {
        static const long multiples[] = { 1, 2, 5 };
        for (int k = 0; k < 3 && (hi - lo) / step > 20; k++) {
            step = multiples[k] * power;
        }
    }
    first = (long)floor(lo / step);
    last = (long)ceil(hi / step);
    ticks = last - first;
    width = ticks * CELLS + 1;

    /* ○ or ● at each end of an interval */
    for (int i = 0; i < s->count; i++) {
        const Interval *p = &s->parts[i];
        for (int end = 0; end < 2; end++) {
            int has = end == 0 ? p->has_low : p->has_high;
            Fraction f = end == 0 ? p->low : p->high;
            long col;
            if (!has) continue;
            col = lround((value_of(f) / step - first) * CELLS);
            if (col >= 0 && col < width) {
                marker[col] = solution_set_contains(s, f) ? 2 : 1;
            }
        }
    }

    put(&w, " "); /* a margin, so the label under the first tick can start with - */
    for (long c = 0; c < width; c++) {
        double x = (first + (double)c / CELLS) * step;
        if (marker[c] == 2) {
            put(&w, "●");
        } else if (marker[c] == 1) {
            put(&w, "○");
        } else if (c == 0 && s->count > 0 && !s->parts[0].has_low) {
            put(&w, "◀");
        } else if (c == width - 1 && s->count > 0 && !s->parts[s->count - 1].has_high) {
            put(&w, "▶");
        } else {
            put(&w, in_set(s, x) ? "━" : "─");
        }
    }
    put(&w, "\n");

    /* the numbers under the tick marks, centred, skipping any that would touch */
    memset(labels, ' ', sizeof labels);
    for (long k = 0; k <= ticks; k++) {
        char num[24];
        long col = k * CELLS + 1, start; /* + 1 for the margin */
        int n = snprintf(num, sizeof num, "%ld", (first + k) * step);
        start = col - n / 2;
        if (start < 0) start = 0;
        if (k > 0 && (size_t)start <= label_end) continue;
        if (start + n >= (long)sizeof labels - 1) break;
        memcpy(labels + start, num, (size_t)n);
        label_end = (size_t)(start + n);
    }
    labels[label_end] = '\0';
    put(&w, labels);
    return w.overflow ? -1 : (int)w.len;
}

/* ---- The coordinate plane ---- */

#define PLANE 10 /* from -10 to 10 on each axis */

static int holds(double a, Relation r, double b) {
    switch (r) {
    case RELATION_LESS: return a < b;
    case RELATION_LESS_EQUAL: return a <= b;
    case RELATION_GREATER: return a > b;
    case RELATION_GREATER_EQUAL: return a >= b;
    default: return a != b;
    }
}

/* whether the boundary line passes through the square around (x, y) */
static int on_line(const PlaneInequality *p, int x, int y) {
    double y1, y2;
    if (p->vertical) {
        double c = value_of(p->c);
        return c >= x - 0.5 && c < x + 0.5;
    }
    /* through the middle of the square, or across it (just touching a
       corner does not count, so a line of slope -1 stays one cell thick) */
    if (fabs(value_of(p->m) * x + value_of(p->b) - y) <= 0.5) return 1;
    y1 = value_of(p->m) * (x - 0.5) + value_of(p->b);
    y2 = value_of(p->m) * (x + 0.5) + value_of(p->b);
    return fmax(y1, y2) > y - 0.5 && fmin(y1, y2) < y + 0.5;
}

static int shaded(const PlaneInequality *p, int x, int y) {
    if (p->vertical) return holds(x, p->relation, value_of(p->c));
    return holds(y, p->relation, value_of(p->m) * x + value_of(p->b));
}

int plane_inequality_graph(const PlaneInequality *p, char *out, size_t size) {
    Writer w = { out, size, 0, 0 };
    int strict = p->relation == RELATION_LESS || p->relation == RELATION_GREATER;

    if (size == 0) return -1;
    out[0] = '\0';
    for (int y = PLANE; y >= -PLANE; y--) {
        for (int x = -PLANE; x <= PLANE; x++) {
            int line = on_line(p, x, y);
            if (line) {
                put(&w, strict ? "·" : "•");
            } else if (x == 0 && y == 0) {
                put(&w, "┼");
            } else if (x == 0) {
                put(&w, "│");
            } else if (y == 0) {
                put(&w, "─");
            } else {
                put(&w, shaded(p, x, y) ? "░" : " ");
            }
            if (x < PLANE) { /* the gap before the next column */
                if (y == 0) {
                    put(&w, "─");
                } else if (!line && !on_line(p, x + 1, y) && x != 0 && x + 1 != 0 &&
                           shaded(p, x, y) && shaded(p, x + 1, y)) {
                    put(&w, "░");
                } else {
                    put(&w, " ");
                }
            }
        }
        if (y > -PLANE) put(&w, "\n");
    }
    return w.overflow ? -1 : (int)w.len;
}
