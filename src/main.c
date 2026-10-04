#include <stdio.h>
#include <string.h>
#include "expression.h"
#include "inequality.h"
#include "linear.h"
#include "quadratic.h"
#include "system.h"

static QuadraticMethod method = QUADRATIC_FORMULA;

/* "x^2 + 3x + 2 = 0": prints the steps and the solutions */
static int solve_quadratic(const char *equation) {
    QuadraticSolution s;

    quadratic_solve(equation, method, &s);
    if (s.kind == QUADRATIC_INVALID) {
        printf("%s -> cannot solve: %s\n", equation, s.error);
        return 1;
    }
    for (int i = 0; i < s.step_count; i++) {
        if (i == 0) {
            printf("%s\n", s.steps[i].equation);
        } else {
            printf("  %-30s %s\n", s.steps[i].equation, s.steps[i].action);
        }
    }
    switch (s.kind) {
    case QUADRATIC_TWO_REAL:
        printf("Two solutions: %s", s.text);
        if (!s.rational) printf(" (about %.4f and %.4f)", s.approx[0], s.approx[1]);
        printf("\n");
        break;
    case QUADRATIC_ONE_REAL:
        printf("One solution: %s\n", s.text);
        break;
    default:
        printf("No real solution: %s\n", s.text);
        break;
    }
    return 0;
}

/* "2x - 2 = 6": prints the steps and the solution */
static int solve(const char *equation) {
    LinearSolution s;
    char value[48];

    if (strchr(equation, '^') != NULL || strstr(equation, "²") != NULL) {
        return solve_quadratic(equation);
    }
    linear_solve(equation, &s);
    if (s.kind == LINEAR_INVALID && strstr(s.error, "multiplies variables") != NULL) {
        return solve_quadratic(equation); /* (9 - w)w = 14 */
    }
    if (s.kind == LINEAR_INVALID) {
        printf("%s -> cannot solve: %s\n", equation, s.error);
        return 1;
    }
    for (int i = 0; i < s.step_count; i++) {
        if (i == 0) {
            printf("%s\n", s.steps[i].equation);
        } else {
            printf("  %-24s %s\n", s.steps[i].equation, s.steps[i].action);
        }
    }
    switch (s.kind) {
    case LINEAR_ONE_SOLUTION:
        fraction_format(s.value, value, sizeof value);
        printf("Solution: %c = %s\n", s.variable, value);
        break;
    case LINEAR_NO_SOLUTION:
        printf("No solution: the %c terms cancel and the numbers left are not equal\n", s.variable);
        break;
    default:
        printf("Every number is a solution: both sides are the same\n");
        break;
    }
    return 0;
}

/* "x + y = 20; x - y = 10": prints the steps and the solution */
static int solve_system(const char *text) {
    SystemSolution s;
    char first[128], a[48], b[48];
    const char *second = strchr(text, ';') + 1;

    snprintf(first, sizeof first, "%.*s", (int)(second - 1 - text), text);
    system_solve(first, second, &s);
    if (s.kind == SYSTEM_INVALID) {
        printf("%s -> cannot solve: %s\n", text, s.error);
        return 1;
    }
    for (int i = 0; i < s.step_count; i++) {
        printf("  %-24s %s\n", s.steps[i].equation, s.steps[i].action);
    }
    switch (s.kind) {
    case SYSTEM_ONE_SOLUTION:
        fraction_format(s.values[0], a, sizeof a);
        fraction_format(s.values[1], b, sizeof b);
        printf("Solution: %c = %s, %c = %s\n", s.names[0], a, s.names[1], b);
        break;
    case SYSTEM_NO_SOLUTION:
        printf("No solution: the lines are parallel\n");
        break;
    default:
        printf("Infinitely many solutions: every (%c, %c) with %s\n", s.names[0], s.names[1], s.line);
        break;
    }
    return 0;
}

/* "5x - 2 ≥ 13": prints the steps, the solution and its graph. With two
   variables, as in "y > (2/3)x + 1", it graphs it on the plane instead. */
static int solve_inequality(const char *text) {
    InequalitySolution s;
    PlaneInequality p;
    char graph[8192], interval[128];

    if (plane_inequality_read(text, &p) == NULL) {
        plane_inequality_describe(&p, graph, sizeof graph);
        printf("%s\n%s\n", text, graph);
        plane_inequality_graph(&p, graph, sizeof graph);
        printf("%s\n", graph);
        return 0;
    }
    if (!inequality_solve(text, &s)) {
        printf("%s -> cannot solve: %s\n", text, s.error);
        return 1;
    }
    for (int i = 0; i < s.step_count; i++) {
        if (i == 0) {
            printf("%s\n", s.steps[i].equation);
        } else {
            printf("  %-30s %s\n", s.steps[i].equation, s.steps[i].action);
        }
    }
    solution_set_interval_notation(&s.set, interval, sizeof interval);
    printf("Solution: %s, or %s in interval notation\n", s.text, interval);
    solution_set_graph(&s.set, graph, sizeof graph);
    printf("%s\n", graph);
    return 0;
}

static int has_relation(const char *s) {
    return strpbrk(s, "<>") != NULL || strstr(s, "!=") != NULL ||
           strstr(s, "≤") != NULL || strstr(s, "≥") != NULL || strstr(s, "≠") != NULL;
}

/* "Seven more than three times a number x": prints 3x + 7 */
static int translate(const char *phrase) {
    char text[256];
    Expr *e = expr_parse(phrase);
    int failed = e == NULL || expr_format(e, text, sizeof text) < 0;

    if (failed) {
        printf("%s -> (not understood)\n", phrase);
    } else {
        printf("%s -> %s\n", phrase, text);
    }
    expr_free(e);
    return failed;
}

int main(int argc, char **argv) {
    int failed = 0;

    if (argc > 1 && strncmp(argv[1], "--", 2) == 0) {
        if (strcmp(argv[1], "--factoring") == 0) {
            method = QUADRATIC_FACTORING;
        } else if (strcmp(argv[1], "--square") == 0) {
            method = QUADRATIC_COMPLETING_THE_SQUARE;
        } else if (strcmp(argv[1], "--formula") != 0) {
            fprintf(stderr, "%s: unknown option %s\n", argv[0], argv[1]);
            return 2;
        }
        argv++;
        argc--;
    }

    if (argc < 2) {
        fprintf(stderr, "usage: %s [--formula | --factoring | --square] \"phrase or equation\" ...\n", argv[0]);
        fprintf(stderr, "example: %s \"Seven more than three times a number x\"\n", argv[0]);
        fprintf(stderr, "example: %s \"9x - 12 = 5x + 8\"\n", argv[0]);
        fprintf(stderr, "example: %s \"x + y = 20; x - y = 10\"\n", argv[0]);
        fprintf(stderr, "example: %s --factoring \"x^2 + 3x + 2 = 0\"\n", argv[0]);
        fprintf(stderr, "example: %s \"|x - 4| < 7\"\n", argv[0]);
        return 2;
    }

    for (int i = 1; i < argc; i++) {
        if (i > 1) printf("\n");
        if (has_relation(argv[i])) {
            failed |= solve_inequality(argv[i]);
        } else if (strchr(argv[i], ';') != NULL) {
            failed |= solve_system(argv[i]);
        } else if (strchr(argv[i], '=') != NULL) {
            failed |= solve(argv[i]);
        } else {
            failed |= translate(argv[i]);
        }
    }
    return failed;
}
