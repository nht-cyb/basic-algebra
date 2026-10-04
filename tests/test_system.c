/* Examples from
   https://www.basic-mathematics.com/system-of-linear-equations.html
   https://www.basic-mathematics.com/solutions-of-systems-of-linear-equations.html
   https://www.basic-mathematics.com/substitution-method.html
   https://www.basic-mathematics.com/elimination-method.html
   https://www.basic-mathematics.com/solve-a-system-of-linear-equations-by-using-a-table.html
   https://www.basic-mathematics.com/solve-a-system-without-a-unique-solution.html */

#include <string.h>
#include "check.h"
#include "system.h"

static const char *kind_name(SystemKind kind) {
    switch (kind) {
    case SYSTEM_ONE_SOLUTION: return "one solution";
    case SYSTEM_NO_SOLUTION: return "no solution";
    case SYSTEM_INFINITELY_MANY: return "infinitely many";
    default: return "invalid";
    }
}

static Fraction value_of(const SystemSolution *s, char name) {
    Fraction none = { 0, 0 };
    for (int i = 0; i < 2; i++) {
        if (s->names[i] == name) return s->values[i];
    }
    return none;
}

static void check_one(const char *first, const char *second,
                      char n1, long v1, char n2, long v2) {
    SystemSolution s;
    char label[128], got[112] = "", a[48], b[48];
    int ok = system_solve(first, second, &s) == SYSTEM_ONE_SOLUTION;

    snprintf(label, sizeof label, "%s and %s", first, second);
    if (ok) {
        fraction_format(value_of(&s, n1), a, sizeof a);
        fraction_format(value_of(&s, n2), b, sizeof b);
        snprintf(got, sizeof got, "%c = %s, %c = %s", n1, a, n2, b);
        ok = fraction_equal(value_of(&s, n1), fraction_integer(v1)) &&
             fraction_equal(value_of(&s, n2), fraction_integer(v2));
    } else {
        snprintf(got, sizeof got, "%s%s%s", kind_name(s.kind), s.error ? ": " : "", s.error ? s.error : "");
    }
    CHECK(label, ok, got);
}

/* line is only checked for infinitely many solutions */
static void check_kind(const char *first, const char *second, SystemKind expected, const char *line) {
    SystemSolution s;
    char label[128], got[96];
    int ok = system_solve(first, second, &s) == expected;

    if (ok && expected == SYSTEM_INFINITELY_MANY) {
        ok = strcmp(s.line, line) == 0;
    }
    snprintf(label, sizeof label, "%s and %s", first, second);
    snprintf(got, sizeof got, "%s%s%s", kind_name(s.kind),
             s.kind == SYSTEM_INFINITELY_MANY ? ": " : "",
             s.kind == SYSTEM_INFINITELY_MANY ? s.line : "");
    CHECK(label, ok, got);
}

/* The steps after the two given equations should match the lesson. */
static void check_steps(const char *first, const char *second, const char *const *expected) {
    SystemSolution s;
    char label[128], got[160] = "";
    int i = 0, ok = 1;

    system_solve(first, second, &s);
    for (; expected[i] != NULL; i++) {
        if (i + 2 >= s.step_count || strcmp(s.steps[i + 2].equation, expected[i]) != 0) {
            ok = 0;
        }
    }
    ok = ok && s.step_count == i + 2;
    for (int j = 2; j < s.step_count; j++) {
        if (j > 2) strncat(got, ", ", sizeof got - strlen(got) - 1);
        strncat(got, s.steps[j].equation, sizeof got - strlen(got) - 1);
    }
    snprintf(label, sizeof label, "steps of %s and %s", first, second);
    CHECK(label, ok, got);
}

#define STEPS(first, second, ...)                                   \
    do {                                                            \
        const char *const expected_[] = { __VA_ARGS__, NULL };      \
        check_steps(first, second, expected_);                      \
    } while (0)

int main(void) {
    printf("== System of linear equations ==\n");
    check_one("x + y = 20", "x − y = 10", 'x', 15, 'y', 5);
    check_one("x + y = 20", "y − x = -10", 'x', 15, 'y', 5);
    /* the lesson sets this one up without solving it */
    check_one("q + d = 24", "25 × q + 10 × d = 450", 'q', 14, 'd', 10);

    printf("\n== Elimination method ==\n");
    STEPS("x + y = 20", "x − y = 10", "2x = 30", "x = 15", "y + 15 = 20", "y = 5");
    check_one("3x + y = 10", "-4x − 2y = 2", 'x', 11, 'y', -23);
    STEPS("3x + y = 10", "-4x − 2y = 2", "6x + 2y = 20", "2x = 22", "x = 11", "y + 33 = 10", "y = -23");

    printf("\n== Substitution method ==\n");
    check_kind("2x + y = 8", "2x + y = 8", SYSTEM_INFINITELY_MANY, "y = -2x + 8");
    check_kind("2x + y = 4", "2x + y = 8", SYSTEM_NO_SOLUTION, NULL);

    printf("\n== Solving with a table ==\n");
    check_one("y = x - 2", "y = -2x + 7", 'x', 3, 'y', 1);
    check_one("x + y = 2", "2x + 4y = 12", 'x', -2, 'y', 4);

    printf("\n== Without a unique solution ==\n");
    check_kind("4x - y = 5", "-4x + y = -5", SYSTEM_INFINITELY_MANY, "y = 4x - 5");
    STEPS("4x - y = 5", "-4x + y = -5", "0 = 0");
    check_kind("15x - 5y = 35", "-3x + y = -7", SYSTEM_INFINITELY_MANY, "y = 3x - 7");
    STEPS("15x - 5y = 35", "-3x + y = -7", "-15x + 5y = -35", "0 = 0");

    printf("\n== Number of solutions ==\n");
    check_kind("y = (-2/9)x + 6", "y = 2x + - 3", SYSTEM_ONE_SOLUTION, NULL);
    check_kind("y = -8x + 6", "y = 8x + -10", SYSTEM_ONE_SOLUTION, NULL);
    check_kind("y = 0.5x + 3", "y = 6x + 3", SYSTEM_ONE_SOLUTION, NULL);
    check_kind("y = -2x + 1", "y = -2x - 2", SYSTEM_NO_SOLUTION, NULL);
    check_kind("y = 3x + 5", "y = 3x + -8", SYSTEM_NO_SOLUTION, NULL);
    check_kind("y = (2/5)x + -6", "y = (2/5)x + 1", SYSTEM_NO_SOLUTION, NULL);
    check_kind("y = 2x + 1", "y = 2x + 1", SYSTEM_INFINITELY_MANY, "y = 2x + 1");
    check_kind("y = -4x + 1/2", "y = -4x + 1/2", SYSTEM_INFINITELY_MANY, "y = -4x + 1/2");
    check_kind("y = (3/4)x + 8", "y = (3/4)x + 8", SYSTEM_INFINITELY_MANY, "y = (3/4)x + 8");

    return CHECK_DONE();
}
