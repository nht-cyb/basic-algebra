#ifndef INEQUALITY_H
#define INEQUALITY_H

#include <stddef.h>
#include "fraction.h"
#include "linear.h"

#define INEQUALITY_MAX_PARTS 8
#define INEQUALITY_MAX_STEPS 24

typedef enum {
    RELATION_LESS,          /* <  */
    RELATION_LESS_EQUAL,    /* ≤  */
    RELATION_GREATER,       /* >  */
    RELATION_GREATER_EQUAL, /* ≥  */
    RELATION_NOT_EQUAL      /* ≠  */
} Relation;

/* One stretch of the number line. A missing end goes on forever:
   x > 2 is { has_low, low = 2, low_closed = 0, no high }. */
typedef struct {
    int has_low, has_high;
    Fraction low, high;
    int low_closed, high_closed; /* ● (included) or ○ (not included) */
} Interval;

/* The numbers that make an inequality true: up to INEQUALITY_MAX_PARTS
   intervals, sorted and not touching. No parts means no solution. */
typedef struct {
    int count;
    Interval parts[INEQUALITY_MAX_PARTS];
} SolutionSet;

SolutionSet solution_set_all(void);
SolutionSet solution_set_none(void);
/* The numbers x with "x relation value", e.g. x ≥ 3 */
SolutionSet solution_set_from(Relation relation, Fraction value);
/* "and" keeps the numbers in both; "or" keeps the numbers in either */
SolutionSet solution_set_and(const SolutionSet *a, const SolutionSet *b);
SolutionSet solution_set_or(const SolutionSet *a, const SolutionSet *b);
int solution_set_contains(const SolutionSet *s, Fraction x);

/* Writes "x < 6", "2 ≤ x < 4", "x < -3 or x > 2", "x ≠ 2",
   "all real numbers" or "no solution". Returns the length, or -1. */
int solution_set_format(const SolutionSet *s, char variable, char *out, size_t size);
/* Writes interval notation: "(-∞, 6)", "[2, 4)", "(-∞, 2) ∪ (2, ∞)", "{1}", "∅". */
int solution_set_interval_notation(const SolutionSet *s, char *out, size_t size);

/* Draws the solution set on a number line, as in the lessons: an open
   circle ○ when the number is not included (< or >), a closed circle ●
   when it is (≤ or ≥), and a thick line ━ over the solutions, with an
   arrow when they go on forever. For x ≥ 3:
      ────────────────────●━━━━━━━━━━━━━━━━━━━▶
     -2  -1   0   1   2   3   4   5   6   7   8
   The two lines are separated by a newline. Returns the length, or -1. */
int solution_set_graph(const SolutionSet *s, char *out, size_t size);

typedef struct {
    int ok;             /* 0 if the inequality could not be solved */
    const char *error;  /* why, when ok is 0 */
    char variable;
    SolutionSet set;
    char text[96];      /* the solution, written as solution_set_format does */
    int step_count;
    LinearStep steps[INEQUALITY_MAX_STEPS]; /* steps[0] is the inequality as given */
} InequalitySolution;

/* Solves an inequality in one variable, recording the steps. It
   understands:
     linear inequalities          5x - 2 ≥ 13, 2 + 3(5 - x) ≥ 38
     compound inequalities        x ≥ 2 and x < 4, 8x + 4 ≤ 20 or 3x - 2 > 1,
                                  -8 < x < 8
     absolute value inequalities  |x - 4| < 7, |3x + 3| > 15
   The relations are <, >, ≤ (or <=), ≥ (or >=) and ≠ (or !=). Each side
   is read as linear_parse reads it. As in the lessons, multiplying or
   dividing both sides by a negative number reverses the inequality. */
int inequality_solve(const char *inequality, InequalitySolution *out);

/* A linear inequality in two variables, as y relation m·x + b, or
   x relation c when the boundary is a vertical line. */
typedef struct {
    char x, y;          /* the variables; y is the one on the vertical axis */
    Relation relation;
    int vertical;       /* 1 for x relation c */
    Fraction m, b, c;
} PlaneInequality;

/* Reads an inequality such as "y > (2/3)x + 1" or "2x + 3y ≤ 6" and
   rewrites it as y relation mx + b, reversing the relation if it divides
   by a negative number. The variable named y (or else the later one in
   the alphabet) goes on the vertical axis. Returns NULL, or a message
   saying why it cannot be read. */
const char *plane_inequality_read(const char *text, PlaneInequality *p);

/* Whether the point (x, y) makes the inequality true: the test point
   method from the lesson. */
int plane_inequality_check(const PlaneInequality *p, Fraction x, Fraction y);

/* Describes how to graph it, e.g. "Draw y = (2/3)x + 1 as a dashed
   line (it is not included) and shade above it". Returns the length,
   or -1. */
int plane_inequality_describe(const PlaneInequality *p, char *out, size_t size);

/* Draws the graph from -10 to 10 on each axis: ░ for the shaded
   solutions, • on a solid boundary line, · on a dashed one, and the
   axes. The rows are separated by newlines. Returns the length, or -1. */
int plane_inequality_graph(const PlaneInequality *p, char *out, size_t size);

#endif
