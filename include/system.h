#ifndef SYSTEM_H
#define SYSTEM_H

#include "fraction.h"
#include "linear.h"

#define SYSTEM_MAX_STEPS 16

typedef enum {
    SYSTEM_INVALID = -1,   /* not understood, not linear, or too big */
    SYSTEM_ONE_SOLUTION,   /* the lines cross at one point */
    SYSTEM_NO_SOLUTION,    /* parallel lines: same slope, different intercept */
    SYSTEM_INFINITELY_MANY /* the same line twice */
} SystemKind;

typedef struct {
    SystemKind kind;
    char names[2];      /* the two variables, in the order they first appear */
    Fraction values[2]; /* the solution, when kind is SYSTEM_ONE_SOLUTION */
    char line[64];      /* when kind is SYSTEM_INFINITELY_MANY, the line every
                           solution is on, e.g. "y = 4x - 5" */
    const char *error;  /* why, when kind is SYSTEM_INVALID */
    int step_count;
    LinearStep steps[SYSTEM_MAX_STEPS]; /* steps[0] and [1] are the equations as given */
} SystemSolution;

/* Solves a system of two linear equations in two variables using the
   elimination method:
     1. Multiply one or both equations so a variable has opposite
        coefficients, then add the equations to eliminate it.
     2. Solve the equation that is left for the other variable.
     3. Substitute that value into one of the equations to find the
        variable that was eliminated.
   For example "x + y = 20" and "x - y = 10" give x = 15 and y = 5.
   If eliminating a variable also eliminates the other one, the system
   has no solution (4 = 8) or infinitely many solutions (0 = 0).
   Each equation is read with linear_parse. Returns out->kind. */
SystemKind system_solve(const char *first, const char *second, SystemSolution *out);

#endif
