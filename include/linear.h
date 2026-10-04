#ifndef LINEAR_H
#define LINEAR_H

#include <stddef.h>
#include "fraction.h"

#define LINEAR_MAX_STEPS 8
#define LINEAR_MAX_VARIABLES 2

/* coef[0]·names[0] + coef[1]·names[1] + constant */
typedef struct {
    Fraction coef[LINEAR_MAX_VARIABLES];
    Fraction constant;
} LinearExpr;

typedef struct {
    int count;                          /* how many variables */
    char names[LINEAR_MAX_VARIABLES];   /* in the order they first appear */
    LinearExpr left, right;
} LinearEquation;

/* Reads an equation with up to two variables, simplifying each side,
   e.g. "6(x - 2) = 2(9 - 2y)" gives 6x - 12 = -4y + 18. It understands
   numbers (2, 3.1), letters as variables, + - * / × ÷, brackets, and a
   number or bracket written before a variable to multiply (2x, 2(x - 4),
   (2/5)x). Returns NULL, or a message saying why it cannot be read. */
const char *linear_parse(const char *text, LinearEquation *out);

/* Writes e as "4x - y - 5", "(2/5)x + 3", "x" or "0".
   Returns the length, or -1 if it does not fit. */
int linear_format(const LinearExpr *e, const char *names, int count, char *out, size_t size);

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
   It reads the equation with linear_parse, and it must have one variable.
   Each side is simplified to ax + b. Then, as in the lessons, the
   variable terms are moved to the left side, the numbers to the right
   side, and both sides are divided by the coefficient. The steps are
   recorded in out->steps. Returns out->kind. */
LinearKind linear_solve(const char *equation, LinearSolution *out);

#endif
