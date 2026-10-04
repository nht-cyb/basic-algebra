/* Examples from
   https://www.basic-mathematics.com/solve-and-graph-inequalities.html
   https://www.basic-mathematics.com/inequality-with-all-real-numbers-as-solutions.html
   https://www.basic-mathematics.com/inequality-with-no-solutions.html
   https://www.basic-mathematics.com/compound-inequality.html
   https://www.basic-mathematics.com/graphing-inequalities.html
   https://www.basic-mathematics.com/solving-absolute-value-inequalities.html
   https://www.basic-mathematics.com/graphing-linear-inequalities.html */

#include <string.h>
#include "check.h"
#include "inequality.h"

static void check_solution(const char *inequality, const char *expected) {
    InequalitySolution s;
    int ok = inequality_solve(inequality, &s) && strcmp(s.text, expected) == 0;
    CHECK(inequality, ok, s.ok ? s.text : s.error);
}

/* The steps after the given inequality should match the lesson. */
static void check_steps(const char *inequality, const char *const *expected) {
    InequalitySolution s;
    char label[160], got[400] = "";
    int i = 0, ok = 1;

    inequality_solve(inequality, &s);
    for (; expected[i] != NULL; i++) {
        if (i + 1 >= s.step_count || strcmp(s.steps[i + 1].equation, expected[i]) != 0) ok = 0;
    }
    ok = ok && s.step_count == i + 1;
    for (int j = 1; j < s.step_count; j++) {
        if (j > 1) strncat(got, ", ", sizeof got - strlen(got) - 1);
        strncat(got, s.steps[j].equation, sizeof got - strlen(got) - 1);
    }
    snprintf(label, sizeof label, "steps of %s", inequality);
    CHECK(label, ok, got);
}

#define STEPS(inequality, ...)                                      \
    do {                                                            \
        const char *const expected_[] = { __VA_ARGS__, NULL };      \
        check_steps(inequality, expected_);                         \
    } while (0)

/* The number line for the solution should be exactly as expected. */
static void check_graph(const char *inequality, const char *expected) {
    InequalitySolution s;
    char graph[1024] = "", label[96];
    inequality_solve(inequality, &s);
    solution_set_graph(&s.set, graph, sizeof graph);
    snprintf(label, sizeof label, "graph of %s", inequality);
    if (strcmp(graph, expected) == 0) {
        CHECK(label, 1, "circle and shading as expected");
    } else {
        CHECK(label, 0, "different, see below");
        printf("%s\n", graph);
    }
}

static void check_interval(const char *inequality, const char *expected) {
    InequalitySolution s;
    char text[96] = "", label[96];
    inequality_solve(inequality, &s);
    solution_set_interval_notation(&s.set, text, sizeof text);
    snprintf(label, sizeof label, "%s in interval notation", inequality);
    CHECK(label, strcmp(text, expected) == 0, text);
}

static void check_point(const PlaneInequality *p, const char *label, long x, long y, int expected) {
    int got = plane_inequality_check(p, fraction_integer(x), fraction_integer(y));
    CHECK(label, got == expected, got ? "true: shade this side" : "false: shade the other side");
}

int main(void) {
    PlaneInequality p;
    char text[256];
    const char *error;

    printf("== Solve and graph inequalities ==\n");
    check_solution("x + 3 < 9", "x < 6");
    check_solution("5x - 2 ≥ 13", "x ≥ 3");
    STEPS("5x - 2 ≥ 13", "5x ≥ 15", "x ≥ 3");
    check_solution("2 + 3(5 - x) ≥ 38", "x ≤ -7");
    STEPS("2 + 3(5 - x) ≥ 38", "-3x + 17 ≥ 38", "-3x ≥ 21", "x ≤ -7");

    printf("\n== All real numbers as solutions ==\n");
    check_solution("x - x > -1", "all real numbers");
    check_solution("5x + x + 3 > 6x + -4", "all real numbers");
    check_solution("8x + 4 ≤ 20 or 3x - 2 > 1", "all real numbers");
    STEPS("8x + 4 ≤ 20 or 3x - 2 > 1", "8x + 4 ≤ 20", "8x ≤ 16", "x ≤ 2",
          "3x - 2 > 1", "3x > 3", "x > 1", "all real numbers");
    check_solution("|x| > -10", "all real numbers");

    printf("\n== No solutions ==\n");
    check_solution("x - x > 6", "no solution");
    check_solution("2x + x - 5 > 3x + 4", "no solution");
    check_solution("x < 2 and x > 9", "no solution");
    check_solution("|x| < -3", "no solution");

    printf("\n== Compound inequalities ==\n");
    check_solution("x > 5 and x ≤ 7", "5 < x ≤ 7");
    check_solution("x ≤ -1 or x > 7", "x ≤ -1 or x > 7");
    check_solution("x ≥ 2 and x < 4", "2 ≤ x < 4");
    check_interval("x ≥ 2 and x < 4", "[2, 4)");
    check_graph("x ≥ 2 and x < 4",
                " ────────────────●━━━━━━━○────────────────\n"
                "-2  -1   0   1   2   3   4   5   6   7   8");
    check_solution("x ≥ -2 and x > 1", "x > 1");
    check_solution("x ≥ -2 or x > 1", "x ≥ -2");
    check_solution("x > 2 or x < -3", "x < -3 or x > 2");
    check_solution("x > 2 and x < -3", "no solution");

    printf("\n== Graphing inequalities ==\n");
    check_graph("x > 2",
                " ────────────────────○━━━━━━━━━━━━━━━━━━━▶\n"
                "-3  -2  -1   0   1   2   3   4   5   6   7");
    check_graph("x < -3",
                " ◀━━━━━━━━━━━━━━━━━━━○────────────────────\n"
                "-8  -7  -6  -5  -4  -3  -2  -1   0   1   2");
    check_graph("x ≥ 6",
                " ────────────────────●━━━━━━━━━━━━━━━━━━━▶\n"
                " 1   2   3   4   5   6   7   8   9  10  11");
    check_graph("x ≤ -1",
                " ◀━━━━━━━━━━━━━━━━━━━●────────────────────\n"
                "-6  -5  -4  -3  -2  -1   0   1   2   3   4");
    check_solution("x ≠ 2", "x ≠ 2");
    check_solution("x > 2 or x < 2", "x ≠ 2");
    check_interval("x ≠ 2", "(-∞, 2) ∪ (2, ∞)");
    check_graph("x ≠ 2",
                " ◀━━━━━━━━━━━━━━━━━━━○━━━━━━━━━━━━━━━━━━━▶\n"
                "-3  -2  -1   0   1   2   3   4   5   6   7");

    printf("\n== Absolute value inequalities ==\n");
    check_solution("|x| < 8", "-8 < x < 8");
    STEPS("|x| < 8", "x < 8 and -x < 8", "x < 8", "-x < 8", "x > -8", "-8 < x < 8");
    check_solution("|x − 4| < 7", "-3 < x < 11");
    check_solution("|3x + 3| > 15", "x < -6 or x > 4");

    printf("\n== Graphing linear inequalities ==\n");
    error = plane_inequality_read("y > (2/3)x + 1", &p);
    CHECK("y > (2/3)x + 1 can be read", error == NULL, error ? error : "yes");
    plane_inequality_describe(&p, text, sizeof text);
    CHECK("y > (2/3)x + 1: dashed line, shade above",
          strcmp(text, "Draw y = (2/3)x + 1 as a dashed line (it is not included) and shade above it") == 0, text);
    check_point(&p, "test point (-3, 4)", -3, 4, 1);
    check_point(&p, "test point (3, 1)", 3, 1, 0);
    plane_inequality_read("y ≥ (2/3)x + 1", &p);
    plane_inequality_describe(&p, text, sizeof text);
    CHECK("y ≥ (2/3)x + 1: solid line", strstr(text, "solid line") != NULL, text);

    return CHECK_DONE();
}
