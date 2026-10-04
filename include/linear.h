#ifndef LINEAR_H
#define LINEAR_H

#include "fraction.h"

#define LINEAR_MAX_STEPS 8

typedef enum {
    LINEAR_INVALID = -1, /* not understood, not linear, or too big */
    LINEAR_ONE_SOLUTION, /* x = value */
    LINEAR_NO_SOLUTION,  /* e.g. x + 1 = x + 2 */
    LINEAR_ALL_NUMBERS   /* e.g. 2(x + 1) = 2x + 2 */
} LinearKind;

typedef struct {
    char equation[128]; /* e.g. "4x = 20" */
    char action[96];    /* how we got there, e.g. "Add 12 to each side" */
} LinearStep;

typedef struct {
    LinearKind kind;
    char variable;  /* the letter solved for */
    Fraction value; /* the solution, when kind is LINEAR_ONE_SOLUTION */
    const char *error; /* why, when kind is LINEAR_INVALID */
    int step_count;
    LinearStep steps[LINEAR_MAX_STEPS]; /* steps[0] is the equation as given */
} LinearSolution;

/* Solves a linear equation in one variable, written as text, e.g.
     "9x - 12 = 5x + 8"            -> x = 5
     "6(x - 2) = 2(9 - 2x)"        -> x = 3
     "(2/5)x + 4 = 14"             -> x = 25
     "3.1x + 1.2 = 7.4"            -> x = 2
     "60 = 4 + n × 2"              -> n = 28
   It understands numbers (2, 3.1), one letter as the variable, + - * /
   × ÷, brackets, and a number or bracket written before the variable
   to multiply (2x, 2(x - 4), (2/5)x).
   Each side is simplified to ax + b. Then, as in the lessons, the
   variable terms are moved to the left side, the numbers to the right
   side, and both sides are divided by the coefficient. The steps are
   recorded in out->steps. Returns out->kind. */
LinearKind linear_solve(const char *equation, LinearSolution *out);

#endif
