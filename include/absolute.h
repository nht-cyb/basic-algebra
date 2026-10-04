#ifndef ABSOLUTE_H
#define ABSOLUTE_H

#include "fraction.h"
#include "inequality.h"
#include "linear.h"

#define ABSOLUTE_MAX_STEPS 40

/* The value of a numerical expression with absolute values, using the
   definition |x| = x if x ≥ 0 and -x if x < 0:
     "|-8 + 2 × 5|" is 2, "|4^2 − 4 × 2|" is 8, "||5||" is 5.
   Returns 0, or -1 if it has a variable or cannot be worked out. */
int absolute_value_evaluate(const char *text, Fraction *out);

typedef struct {
    int ok;            /* 0 if the equation could not be solved */
    const char *error; /* why, when ok is 0 */
    char variable;
    SolutionSet set;   /* the solutions: usually two numbers, maybe none */
    char text[96];     /* e.g. "x = -2 or x = 3", "no solution" */
    int step_count;
    LinearStep steps[ABSOLUTE_MAX_STEPS]; /* steps[0] is the equation as given */
} AbsoluteSolution;

/* Solves an equation with absolute values in one variable, recording
   the steps as the lessons do them.
   With one absolute value, it is moved to one side, as in
     4|2x - 1| - 8 = 12  ->  |2x - 1| = 5  ->  2x - 1 = 5 or 2x - 1 = -5,
   and |A| = k has no solution when k is negative.
   With more than one, such as |2x + 6| + |3x - 4| = 12, or with the
   variable outside the bars, it finds where each expression inside |...|
   is 0, solves the equation in each region between those numbers, and
   keeps the answers that are in their region.
   Each side can use what linear_parse understands plus |...|. */
int absolute_solve(const char *equation, AbsoluteSolution *out);

#endif
