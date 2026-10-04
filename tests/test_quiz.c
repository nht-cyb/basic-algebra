/* The quiz from
   https://www.basic-mathematics.com/writing-an-algebraic-expression.html
   Like the quiz, an answer is right if it matches one of the accepted
   answers once spaces are removed. */

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "expression.h"

typedef struct {
    const char *phrase;
    const char *answers[5]; /* NULL-terminated */
} Question;

static const Question quiz[] = {
    { "Seven more than three times a number x.",
      { "3x+7", "3x + 7", "7+3x", "7 + 3x", NULL } },
    { "The difference of y and 14.",
      { "y-14", "y - 14", NULL } },
    { "The product of 5 and the sum of n and 6.",
      { "5(n+6)", "5*(n+6)", "5(n + 6)", "5*(n + 6)", NULL } },
    { "Eight less than the quotient of z and 4.",
      { "z/4-8", "z/4 - 8", "z/4 -8", "z/4- 8", NULL } },
    { "Twice the difference of a number p and 9.",
      { "2(p-9)", "2*(p-9)", "2(p - 9)", "2*(p - 9)", NULL } },
};

static int same_ignoring_spaces(const char *a, const char *b) {
    for (;;) {
        while (isspace((unsigned char)*a)) a++;
        while (isspace((unsigned char)*b)) b++;
        if (*a != *b) return 0;
        if (*a == '\0') return 1;
        a++;
        b++;
    }
}

int main(void) {
    size_t n = sizeof quiz / sizeof quiz[0];
    int score = 0;
    char text[256];

    for (size_t i = 0; i < n; i++) {
        const Question *q = &quiz[i];
        Expr *e = expr_parse(q->phrase);
        int correct = 0;

        if (e == NULL || expr_format(e, text, sizeof text) < 0) {
            strcpy(text, "(not understood)");
        } else {
            for (const char *const *a = q->answers; *a != NULL; a++) {
                if (same_ignoring_spaces(text, *a)) {
                    correct = 1;
                    break;
                }
            }
        }
        expr_free(e);

        printf("%s Question %zu: %s -> %s\n",
               correct ? "PASS" : "FAIL", i + 1, q->phrase, text);
        if (!correct) {
            printf("     expected: %s\n", q->answers[0]);
        }
        score += correct;
    }

    printf("Score: %d out of %zu\n", score, n);
    return score == (int)n ? 0 : 1;
}
