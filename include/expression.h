#ifndef EXPRESSION_H
#define EXPRESSION_H

#include <stddef.h>

typedef enum {
    EXPR_NUMBER,
    EXPR_VARIABLE,
    EXPR_ADD,      /* left + right */
    EXPR_SUBTRACT, /* left - right */
    EXPR_MULTIPLY, /* left * right */
    EXPR_DIVIDE    /* left / right */
} ExprKind;

typedef struct Expr {
    ExprKind kind;
    long value;          /* EXPR_NUMBER */
    char name;           /* EXPR_VARIABLE, e.g. 'x' */
    struct Expr *left;   /* operators only */
    struct Expr *right;
} Expr;

/* Building blocks. The operator functions take ownership of both
   operands and free them if they fail. All return NULL on failure. */
Expr *expr_number(long value);
Expr *expr_variable(char name);
Expr *expr_add(Expr *left, Expr *right);
Expr *expr_subtract(Expr *left, Expr *right);
Expr *expr_multiply(Expr *left, Expr *right);
Expr *expr_divide(Expr *left, Expr *right);
void expr_free(Expr *e);

/* Writes e as algebra, e.g. "3x + 7", "5(n + 6)", "z/4 - 8".
   Returns the length written, or -1 if it does not fit in size. */
int expr_format(const Expr *e, char *out, size_t size);

/* Turns a verbal phrase into an expression, using the key words from
   basic-mathematics.com:
     "6 more than n"              -> n + 6
     "two less than m"            -> m - 2
     "the difference of x and 9"  -> x - 9
     "the product of x and 10"    -> 10x
     "the quotient of n and 5"    -> n/5
     "the ratio of y to 8"        -> y/8
     "twice a number"             -> 2x
   "a number" is x and "another number" is y, unless a letter follows,
   as in "a number p". Returns NULL if the phrase is not understood. */
Expr *expr_parse(const char *phrase);

#endif
