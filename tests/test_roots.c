/* Examples from
   https://www.basic-mathematics.com/square-root-of-a-number.html
   https://www.basic-mathematics.com/square-root-algorithm.html
   https://www.basic-mathematics.com/estimate-the-square-root.html
   https://www.basic-mathematics.com/irrational-numbers.html */

#include <math.h>
#include <string.h>
#include "check.h"
#include "root.h"

static void check_root(const char *label, unsigned long n, int decimals, int round, const char *expected) {
    char got[64] = "(error)";
    square_root(n, decimals, round, got, sizeof got);
    CHECK(label, strcmp(got, expected) == 0, got);
}

static void check_between(unsigned long n, unsigned long low, unsigned long high) {
    char label[64], got[64];
    unsigned long l, h;
    square_root_between(n, &l, &h);
    snprintf(label, sizeof label, "%lu < sqrt(%lu) < %lu", low, n, high);
    snprintf(got, sizeof got, "%lu and %lu", l, h);
    CHECK(label, l == low && h == high, got);
}

static void check_estimate(const char *label, unsigned long n, double expected, double within) {
    char got[64];
    double e = square_root_estimate(n);
    snprintf(got, sizeof got, "%.4f", e);
    CHECK(label, fabs(e - expected) < within, got);
}

int main(void) {
    printf("== Square root of a number ==\n");
    check_root("sqrt(16) = 4", 16, 0, 0, "4");
    check_root("sqrt(4) = 2", 4, 0, 0, "2");
    check_root("sqrt(64) = 8", 64, 0, 0, "8");

    printf("\n== Square root algorithm ==\n");
    check_root("sqrt(2685) = 51.81 (by hand, digits cut off)", 2685, 2, 0, "51.81");

    printf("\n== Digits of irrational square roots ==\n");
    check_root("sqrt(2) = 1.4142135 (cut off)", 2, 7, 0, "1.4142135");
    check_root("sqrt(2) = 1.4142136 (rounded)", 2, 7, 1, "1.4142136");
    check_root("sqrt(2) = 1.41421356237309504880", 2, 20, 0, "1.41421356237309504880");
    check_root("sqrt(7) = 2.64575131", 7, 8, 1, "2.64575131");
    check_root("sqrt(35) = 5.9160797831", 35, 10, 1, "5.9160797831");
    check_root("sqrt(8) = 2.82842712475", 8, 11, 1, "2.82842712475");

    printf("\n== Estimate the square root ==\n");
    check_between(34, 5, 6);
    check_between(17, 4, 5);
    check_between(102, 10, 11);
    check_between(22, 4, 5);
    check_between(23, 4, 5);
    check_estimate("sqrt(45) is about 6.714", 45, 6.714, 0.001);
    check_estimate("sqrt(39) is about 6.25", 39, 6.25, 0.0001);

    return CHECK_DONE();
}
