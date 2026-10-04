#ifndef QUADRATIC_H
#define QUADRATIC_H

#include "fraction.h"
#include "linear.h"

#define QUADRATIC_MAX_STEPS 12

/* a·x² + b·x + c, with a not 0 */
typedef struct {
    char variable;
    Fraction a, b, c;
} Quadratic;

/* Reads a quadratic equation such as "x^2 + 3x + 2 = 0", "(9 - w)w = 14"
   or "30 = -16t² + 28t" and moves everything to the left side, giving
   the standard form ax² + bx + c = 0. Text without an = sign, such as
   "-w^2 + 4.5w", is read as an expression. It understands what
   linear_parse does, plus powers written x^2 or x². Returns NULL, or a
   message saying why it cannot be read. */
const char *quadratic_read(const char *text, Quadratic *q);

/* b² - 4ac. Returns 0, or -1 if it does not fit. */
int quadratic_discriminant(const Quadratic *q, Fraction *out);

/* The vertex of the parabola y = ax² + bx + c, at x = -b/2a. Returns 1
   if it is a minimum (a > 0), 0 if it is a maximum, or -1 if it does
   not fit. */
int quadratic_vertex(const Quadratic *q, Fraction *x, Fraction *y);

typedef enum {
    QUADRATIC_FACTORING,              /* (x + 2)(x + 1) = 0 */
    QUADRATIC_COMPLETING_THE_SQUARE,  /* (x + 3)² = 1 */
    QUADRATIC_FORMULA                 /* x = (-b ± √(b² - 4ac)) / 2a */
} QuadraticMethod;

typedef enum {
    QUADRATIC_INVALID = -1, /* not understood, or the method cannot be used */
    QUADRATIC_TWO_REAL,     /* discriminant > 0 */
    QUADRATIC_ONE_REAL,     /* discriminant = 0: one repeated solution */
    QUADRATIC_TWO_COMPLEX   /* discriminant < 0: no real solution */
} QuadraticKind;

typedef struct {
    QuadraticKind kind;
    Quadratic q;
    Fraction discriminant;
    /* The solutions are x = p ± q·√r, times i when kind is
       QUADRATIC_TWO_COMPLEX. r is 1 when there is no square root left,
       and q is 0 when kind is QUADRATIC_ONE_REAL. */
    Fraction p, q_coef;
    long r;
    /* When the real solutions are fractions (r is 1), they are also in
       roots: roots[0] uses + and roots[1] uses -, like x₁ and x₂ in the
       lessons. Both are the same when kind is QUADRATIC_ONE_REAL. */
    int rational;
    Fraction roots[2];
    double approx[2];   /* roots[0] and roots[1] as decimals, when real */
    char text[112];     /* e.g. "x = 3 or x = -1/4", "x = -4 ± 3i" */
    const char *error;  /* why, when kind is QUADRATIC_INVALID */
    int step_count;
    LinearStep steps[QUADRATIC_MAX_STEPS]; /* steps[0] is the equation as given */
} QuadraticSolution;

/* Solves a quadratic equation with the chosen method, recording the
   steps as the lessons do them. Factoring only works when the solutions
   are fractions; otherwise it returns QUADRATIC_INVALID and says to use
   another method. Returns out->kind. */
QuadraticKind quadratic_solve(const char *equation, QuadraticMethod method, QuadraticSolution *out);

#endif
