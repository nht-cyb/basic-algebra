/* Examples from
   https://www.basic-mathematics.com/linear-equations.html
   https://www.basic-mathematics.com/solve-one-step-equations.html
   https://www.basic-mathematics.com/solving-multiplication-equations.html
   https://www.basic-mathematics.com/solving-two-step-equations.html
   https://www.basic-mathematics.com/solving-an-equation-with-a-variable-on-both-sides.html
   https://www.basic-mathematics.com/solve-equations-using-the-distributive-property.html */

#include <string.h>
#include "check.h"
#include "linear.h"

static void check_solution(const char *equation, char variable, long num, long den) {
    LinearSolution s;
    char got[64] = "(not solved)";
    int ok = linear_solve(equation, &s) == LINEAR_ONE_SOLUTION;

    if (ok) {
        char value[48];
        fraction_format(s.value, value, sizeof value);
        snprintf(got, sizeof got, "%c = %s", s.variable, value);
    } else if (s.kind == LINEAR_INVALID) {
        snprintf(got, sizeof got, "(invalid: %s)", s.error);
    }
    CHECK(equation, ok && s.variable == variable && fraction_equal(s.value, fraction_make(num, den)), got);
}

/* The equations after the first one should match the lesson's steps,
   given as a NULL-terminated list. */
static void check_steps(const char *equation, const char *const *expected) {
    LinearSolution s;
    char label[160], got[128] = "";
    int i = 0, ok = 1;

    linear_solve(equation, &s);
    for (; expected[i] != NULL; i++) {
        if (i + 1 >= s.step_count || strcmp(s.steps[i + 1].equation, expected[i]) != 0) {
            ok = 0;
        }
    }
    ok = ok && s.step_count == i + 1;
    for (int j = 1; j < s.step_count; j++) {
        if (j > 1) strncat(got, ", ", sizeof got - strlen(got) - 1);
        strncat(got, s.steps[j].equation, sizeof got - strlen(got) - 1);
    }
    snprintf(label, sizeof label, "steps of %s", equation);
    CHECK(label, ok, got);
}

#define STEPS(equation, ...)                                        \
    do {                                                            \
        const char *const expected_[] = { __VA_ARGS__, NULL };      \
        check_steps(equation, expected_);                           \
    } while (0)

int main(void) {
    printf("== Linear equations ==\n");
    check_solution("2 + x = 5", 'x', 3, 1);
    check_solution("60 = 4 + n × 2", 'n', 28, 1);

    printf("\n== One-step equations ==\n");
    check_solution("x - 2 = 8", 'x', 10, 1);
    check_solution("x - 4 = -6", 'x', -2, 1);
    check_solution("x + 3 = 8", 'x', 5, 1);
    check_solution("x + 5 = -10", 'x', -15, 1);

    printf("\n== Multiplication equations ==\n");
    check_solution("6x = 18", 'x', 3, 1);
    check_solution("2x = 10", 'x', 5, 1);
    check_solution("(2/5)x = 10", 'x', 25, 1);
    STEPS("(2/5)x = 10", "x = 25");

    printf("\n== Two-step equations ==\n");
    check_solution("2x - 2 = 6", 'x', 4, 1);
    STEPS("2x - 2 = 6", "2x = 8", "x = 4");
    check_solution("6x + 2 = 20", 'x', 3, 1);
    STEPS("6x + 2 = 20", "6x = 18", "x = 3");
    check_solution("(2/5)x + 4 = 14", 'x', 25, 1);
    STEPS("(2/5)x + 4 = 14", "(2/5)x = 10", "x = 25");
    check_solution("3.1x + 1.2 = 7.4", 'x', 2, 1);

    printf("\n== Variable on both sides ==\n");
    check_solution("9x - 12 = 5x + 8", 'x', 5, 1);
    STEPS("9x - 12 = 5x + 8", "4x - 12 = 8", "4x = 20", "x = 5");
    check_solution("3y + 11 = 31 - 7y", 'y', 2, 1);
    STEPS("3y + 11 = 31 - 7y", "3y + 11 = -7y + 31", "10y + 11 = 31", "10y = 20", "y = 2");

    printf("\n== Distributive property ==\n");
    check_solution("2(x - 4) = 10", 'x', 9, 1);
    STEPS("2(x - 4) = 10", "2x - 8 = 10", "2x = 18", "x = 9");
    check_solution("6(x - 2) = 2(9 - 2x)", 'x', 3, 1);
    STEPS("6(x - 2) = 2(9 - 2x)", "6x - 12 = -4x + 18", "10x - 12 = 18", "10x = 30", "x = 3");
    check_solution("-2x + 4(x + 3) = 2(-3x + -6)", 'x', -3, 1);
    STEPS("-2x + 4(x + 3) = 2(-3x + -6)", "2x + 12 = -6x - 12", "8x + 12 = -12", "8x = -24", "x = -3");

    return CHECK_DONE();
}
