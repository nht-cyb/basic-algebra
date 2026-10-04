/* Examples from
   https://www.basic-mathematics.com/solving-quadratic-equations-by-factoring.html
   https://www.basic-mathematics.com/solve-by-completing-the-square.html
   https://www.basic-mathematics.com/solve-using-the-quadratic-formula.html
   https://www.basic-mathematics.com/quadratic-formula.html
   https://www.basic-mathematics.com/word-problems-involving-quadratic-equations.html */

#include <math.h>
#include <string.h>
#include "check.h"
#include "quadratic.h"

/* The two solutions, in either order. For one solution, give it twice. */
static void check_roots(const char *equation, QuadraticMethod method,
                        long n1, long d1, long n2, long d2) {
    QuadraticSolution s;
    Fraction want1 = fraction_make(n1, d1), want2 = fraction_make(n2, d2);
    int ok = quadratic_solve(equation, method, &s) != QUADRATIC_INVALID && s.rational;

    if (ok) {
        ok = (fraction_equal(s.roots[0], want1) && fraction_equal(s.roots[1], want2)) ||
             (fraction_equal(s.roots[0], want2) && fraction_equal(s.roots[1], want1));
    }
    CHECK(equation, ok, s.kind == QUADRATIC_INVALID ? s.error : s.text);
}

/* The steps after the given equation should match the lesson. */
static void check_steps(const char *equation, QuadraticMethod method, const char *const *expected) {
    QuadraticSolution s;
    char label[160], got[400] = "";
    int i = 0, ok = 1;

    quadratic_solve(equation, method, &s);
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

#define STEPS(equation, method, ...)                                \
    do {                                                            \
        const char *const expected_[] = { __VA_ARGS__, NULL };      \
        check_steps(equation, method, expected_);                   \
    } while (0)

static void check_text(const char *equation, QuadraticMethod method, const char *expected) {
    QuadraticSolution s;
    quadratic_solve(equation, method, &s);
    CHECK(equation, strcmp(s.text, expected) == 0, s.kind == QUADRATIC_INVALID ? s.error : s.text);
}

static void check_abc(const char *equation, long a, long b, long c) {
    Quadratic q;
    char got[96] = "";
    const char *error = quadratic_read(equation, &q);
    int ok = error == NULL && fraction_equal(q.a, fraction_integer(a)) &&
             fraction_equal(q.b, fraction_integer(b)) && fraction_equal(q.c, fraction_integer(c));
    if (error == NULL) {
        snprintf(got, sizeof got, "a = %ld/%ld, b = %ld/%ld, c = %ld/%ld",
                 q.a.num, q.a.den, q.b.num, q.b.den, q.c.num, q.c.den);
    }
    CHECK(equation, ok, error ? error : got);
}

static void check_vertex(const char *expression, long num, long den, int minimum) {
    Quadratic q;
    Fraction x, y;
    char got[96] = "(error)", value[48];
    int kind = -1;

    if (quadratic_read(expression, &q) == NULL) {
        kind = quadratic_vertex(&q, &x, &y);
        fraction_format(x, value, sizeof value);
        snprintf(got, sizeof got, "%s at %s", kind == 1 ? "minimum" : "maximum", value);
    }
    CHECK(expression, kind == minimum && fraction_equal(x, fraction_make(num, den)), got);
}

#define FACTORING QUADRATIC_FACTORING
#define SQUARE QUADRATIC_COMPLETING_THE_SQUARE
#define FORMULA QUADRATIC_FORMULA

int main(void) {
    QuadraticSolution s;
    char got[96];

    printf("== Solving by factoring ==\n");
    check_roots("x^2 + 3x + 2 = 0", FACTORING, -2, 1, -1, 1);
    STEPS("x^2 + 3x + 2 = 0", FACTORING, "(x + 2)(x + 1) = 0", "x + 2 = 0 or x + 1 = 0", "x = -2 or x = -1");
    check_roots("x^2 + -3x + 2 = 0", FACTORING, 2, 1, 1, 1);
    check_roots("x^2 + x + -30 = 0", FACTORING, -6, 1, 5, 1);
    STEPS("x^2 + x + -30 = 0", FACTORING, "x² + x - 30 = 0", "(x + 6)(x - 5) = 0", "x + 6 = 0 or x - 5 = 0", "x = -6 or x = 5");
    check_roots("x^2 + -x + -30 = 0", FACTORING, 6, 1, -5, 1);
    check_roots("6x^2 + 27x + 30 = 0", FACTORING, -15, 6, -2, 1);

    printf("\n== Solving by completing the square ==\n");
    check_roots("x^2 + 6x + 8 = 0", SQUARE, -2, 1, -4, 1);
    STEPS("x^2 + 6x + 8 = 0", SQUARE, "x² + 6x = -8", "x² + 6x + 9 = 1", "(x + 3)² = 1", "x + 3 = ±1", "x = -2 or x = -4");
    check_roots("x^2 + -6x + 8 = 0", SQUARE, 4, 1, 2, 1);
    check_roots("3x^2 + 8x + -3 = 0", SQUARE, 1, 3, -3, 1);

    printf("\n== Solving with the quadratic formula ==\n");
    check_abc("6x^2 + 8x + 7 = 0", 6, 8, 7);
    check_abc("x^2 + 8x - 7 = 0", 1, 8, -7);
    check_abc("-x^2 - 8x + 7 = 0", -1, -8, 7);
    check_roots("x^2 + 8x + 7 = 0", FORMULA, -1, 1, -7, 1);
    STEPS("x^2 + 8x + 7 = 0", FORMULA, "a = 1, b = 8, c = 7", "Δ = 8² - 4(1)(7) = 36",
          "x = (-8 ± √36) / 2", "x = (-8 ± 6) / 2", "x = -1 or x = -7");
    check_roots("4x^2 - 11x - 3 = 0", FORMULA, 3, 1, -1, 4);
    check_roots("x^2 + x - 2 = 0", FORMULA, 1, 1, -2, 1);
    check_roots("4x^2 + 12x + 9 = 0", FORMULA, -3, 2, -3, 2);
    check_text("x^2 + 8x + 25 = 0", FORMULA, "x = -4 ± 3i");
    check_roots("x^2 - 5x + 4 = 0", FORMULA, 4, 1, 1, 1);

    printf("\n== Applications of the quadratic formula ==\n");
    quadratic_solve("30 = -16t^2 + 28t", FORMULA, &s);
    snprintf(got, sizeof got, "Δ = %ld, %s", s.discriminant.num,
             s.kind == QUADRATIC_TWO_COMPLEX ? "no real solution" : "real solutions");
    CHECK("30 = -16t^2 + 28t: the ball never reaches 30 feet",
          s.kind == QUADRATIC_TWO_COMPLEX && fraction_equal(s.discriminant, fraction_integer(-1136)), got);
    check_text("x^2 = 314", FORMULA, "x = ±√314");
    quadratic_solve("x^2 = 314", FORMULA, &s);
    snprintf(got, sizeof got, "%.2f and %.2f", s.approx[0], s.approx[1]);
    CHECK("x^2 = 314: the side is 17.72 inches", fabs(fabs(s.approx[0]) - 17.72) < 0.005, got);

    printf("\n== Word problems ==\n");
    check_roots("(9 - w)w = 14", FACTORING, 7, 1, 2, 1);
    check_roots("w^2 - 9w + 14 = 0", FACTORING, 7, 1, 2, 1);
    check_roots("(12 - m) × m = 35", FACTORING, 5, 1, 7, 1);
    check_vertex("0.00002x^2 - 0.04x + 38", 1000, 1, 1);
    check_vertex("-w^2 + 4.5w", 9, 4, 0);

    return CHECK_DONE();
}
