/* Examples from
   https://www.basic-mathematics.com/definition-of-absolute-value.html
   https://www.basic-mathematics.com/properties-of-absolute-value.html
   https://www.basic-mathematics.com/solving-absolute-value-equations.html
   https://www.basic-mathematics.com/solve-multi-step-absolute-value-equations.html
   https://www.basic-mathematics.com/solve-tough-absolute-value-equations.html
   https://www.basic-mathematics.com/absolute-value-word-problems.html */

#include <string.h>
#include "absolute.h"
#include "check.h"

static void check_value(const char *expression, long expected) {
    Fraction v = { 0, 1 };
    char got[48] = "(error)";
    int ok = absolute_value_evaluate(expression, &v) == 0;
    if (ok) fraction_format(v, got, sizeof got);
    CHECK(expression, ok && fraction_equal(v, fraction_integer(expected)), got);
}

/* two expressions from a property should have the same value */
static void check_equal(const char *left, const char *right) {
    Fraction a, b;
    char label[128], got[112] = "(error)", x[48], y[48];
    int ok = absolute_value_evaluate(left, &a) == 0 && absolute_value_evaluate(right, &b) == 0;
    if (ok) {
        fraction_format(a, x, sizeof x);
        fraction_format(b, y, sizeof y);
        snprintf(got, sizeof got, "%s and %s", x, y);
        ok = fraction_equal(a, b);
    }
    snprintf(label, sizeof label, "%s = %s", left, right);
    CHECK(label, ok, got);
}

/* left ≤ right, for the inequalities among the properties */
static void check_at_most(const char *left, const char *right) {
    Fraction a, b, d;
    char label[128], got[112] = "(error)", x[48], y[48];
    int ok = absolute_value_evaluate(left, &a) == 0 && absolute_value_evaluate(right, &b) == 0;
    if (ok) {
        fraction_format(a, x, sizeof x);
        fraction_format(b, y, sizeof y);
        snprintf(got, sizeof got, "%s ≤ %s", x, y);
        fraction_add(b, fraction_make(-a.num, a.den), &d);
        ok = d.num >= 0;
    }
    snprintf(label, sizeof label, "%s ≤ %s", left, right);
    CHECK(label, ok, got);
}

static void check_solution(const char *equation, const char *expected) {
    AbsoluteSolution s;
    int ok = absolute_solve(equation, &s) && strcmp(s.text, expected) == 0;
    CHECK(equation, ok, s.ok ? s.text : s.error);
}

/* The steps after the given equation should match the lesson. */
static void check_steps(const char *equation, const char *const *expected) {
    AbsoluteSolution s;
    char label[160], got[600] = "";
    int i = 0, ok = 1;

    absolute_solve(equation, &s);
    for (; expected[i] != NULL; i++) {
        if (i + 1 >= s.step_count || strcmp(s.steps[i + 1].equation, expected[i]) != 0) ok = 0;
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

static void check_inequality(const char *inequality, const char *expected) {
    InequalitySolution s;
    int ok = inequality_solve(inequality, &s) && strcmp(s.text, expected) == 0;
    CHECK(inequality, ok, s.ok ? s.text : s.error);
}

int main(void) {
    printf("== Definition of absolute value ==\n");
    check_value("|6|", 6);
    check_value("|-8|", 8);
    check_value("|0|", 0);
    check_value("|-8 + 2 × 5|", 2);
    check_value("|4^2 − 4 × 2|", 8);
    check_value("|-5 + 5 × 2 − 15|", 10);

    printf("\n== Properties of absolute value ==\n");
    check_value("|3|", 3);
    check_value("|-2|", 2);
    check_equal("|5|", "|-5|");
    check_value("||5||", 5);
    check_equal("|2 - 5|", "|5 - 2|");
    check_equal("|-5 × 6|", "|-5| × |6|");
    check_equal("|-24 / 3|", "|-24| / |3|");
    check_value("|-4|", 4);
    check_at_most("|-4 + -5|", "|-4| + |-5|");
    check_at_most("|-4 + 5|", "|4| + |-5|");
    check_at_most("|5 - 1|", "|5| + |1|");
    check_at_most("|5 - -1|", "|5| + |-1|");
    check_at_most("|9 - 8|", "|9 - 2| + |2 - 8|");
    check_at_most("|9 - -8|", "|9 - 2| + |2 - -8|");
    check_value("|12 - 12|", 0);
    check_equal("|-5| × |-5|", "5 × 5");
    check_at_most("|8| - |4|", "|8 - 4|");
    check_at_most("|8| - |-4|", "|8 - -4|");
    check_inequality("|x| ≥ 2", "x ≤ -2 or x ≥ 2");
    check_inequality("|x| ≤ 2", "-2 ≤ x ≤ 2");

    printf("\n== Solving absolute value equations ==\n");
    check_solution("|x| = 4", "x = -4 or x = 4");
    check_solution("|x − 5| = 2", "x = 3 or x = 7");
    check_solution("|3x + 3| = 15", "x = -6 or x = 4");

    printf("\n== Multi-step absolute value equations ==\n");
    check_solution("4|2x - 1| - 8 = 12", "x = -2 or x = 3");
    STEPS("4|2x - 1| - 8 = 12", "4|2x - 1| = 20", "|2x - 1| = 5", "2x - 1 = 5 or 2x - 1 = -5",
          "2x - 1 = 5", "2x = 6", "x = 3", "2x - 1 = -5", "2x = -4", "x = -2", "x = -2 or x = 3");
    check_solution("0.5|1 - 3x| + 1 = 11", "x = -19/3 or x = 7");

    printf("\n== Tough absolute value equations ==\n");
    check_solution("6|2x + 3| - 7 = 2|2x + 3| + 1", "x = -5/2 or x = -1/2");
    check_solution("|2x + 6| + - 3 + |3x - 4| = 9", "x = -2 or x = 2");
    STEPS("|2x + 6| + - 3 + |3x - 4| = 9",
          "|2x + 6| + |3x - 4| = 12", "2x + 6 = 0 at x = -3, 3x - 4 = 0 at x = 4/3",
          "-(2x + 6) - (3x - 4) = 12", "-5x - 2 = 12", "-5x = 14", "x = -14/5", "x = -14/5 is not in x < -3",
          "(2x + 6) - (3x - 4) = 12", "-x + 10 = 12", "-x = 2", "x = -2", "x = -2 is in -3 ≤ x < 4/3",
          "(2x + 6) + (3x - 4) = 12", "5x + 2 = 12", "5x = 10", "x = 2", "x = 2 is in x ≥ 4/3",
          "x = -2 or x = 2");

    printf("\n== Absolute value word problems ==\n");
    check_solution("|x - 50| = 15", "x = 35 or x = 65");
    check_inequality("|x - 25000| ≤ 1250", "23750 ≤ x ≤ 26250");
    check_solution("|x - 1| = 3", "x = -2 or x = 4");
    check_inequality("|x - 40| < 5", "35 < x < 45");

    return CHECK_DONE();
}
