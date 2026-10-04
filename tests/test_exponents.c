/* Examples from
   https://www.basic-mathematics.com/exponents.html
   https://www.basic-mathematics.com/laws-of-exponents.html
   https://www.basic-mathematics.com/properties-of-exponents.html */

#include <string.h>
#include "check.h"
#include "exponent.h"

static Power p(long base, long exponent) {
    return power_make(fraction_integer(base), exponent);
}

/* base^exponent should equal num/den */
static void check_value(const char *label, Power x, long num, long den) {
    Fraction v;
    char got[64] = "(error)";
    int ok = power_value(x, &v) == 0;
    if (ok) fraction_format(v, got, sizeof got);
    CHECK(label, ok && fraction_equal(v, fraction_make(num, den)), got);
}

/* a law of exponents should give the expected power, written out */
static void check_law(const char *label, int result, Power x, const char *expected) {
    char got[64] = "(error)";
    if (result == 0) power_format(x, got, sizeof got);
    CHECK(label, result == 0 && strcmp(got, expected) == 0, got);
}

/* two sides of a property should have the same value */
static void check_same(const char *label, Fraction left, Fraction right) {
    char got[64];
    fraction_format(left, got, sizeof got);
    CHECK(label, fraction_equal(left, right), got);
}

static Fraction value(Power x) {
    Fraction v = { 0, 0 };
    power_value(x, &v);
    return v;
}

static Fraction times(Fraction a, Fraction b) {
    Fraction v = { 0, 0 };
    fraction_multiply(a, b, &v);
    return v;
}

static Fraction over(Fraction a, Fraction b) {
    Fraction v = { 0, 0 };
    fraction_divide(a, b, &v);
    return v;
}

int main(void) {
    Power r;
    Fraction f;
    char got[64];

    printf("== Exponents ==\n");
    check_value("8^6 = 8 × 8 × 8 × 8 × 8 × 8", p(8, 6), 8L * 8 * 8 * 8 * 8 * 8, 1);
    check_value("12^10", p(12, 10), 61917364224L, 1);
    check_value("5^3 = 5 × 5 × 5", p(5, 3), 5 * 5 * 5, 1);
    check_value("9^4 = 9 × 9 × 9 × 9", p(9, 4), 9 * 9 * 9 * 9, 1);
    check_value("7^2 = 7 × 7", p(7, 2), 7 * 7, 1);
    check_value("6^6", p(6, 6), 6 * 6 * 6 * 6 * 6 * 6, 1);
    check_value("2^8", p(2, 8), 2 * 2 * 2 * 2 * 2 * 2 * 2 * 2, 1);
    check_value("7^1 = 7", p(7, 1), 7, 1);
    snprintf(got, sizeof got, "%ld", -value(p(2, 6)).num);
    CHECK("-2^6 = -(2^6) = -64", -value(p(2, 6)).num == -64, got);
    check_value("(-2)^6 = 2^6", p(-2, 6), 64, 1);
    check_value("(-2)^7 = -2^7", p(-2, 7), -128, 1);
    check_value("(2/3)^3 = 8/27", power_make(fraction_make(2, 3), 3), 8, 27);
    power_format(p(-2, 6), got, sizeof got);
    CHECK("(-2)^6 is written with parentheses", strcmp(got, "(-2)^6") == 0, got);

    printf("\n== Laws of exponents ==\n");
    check_law("2^3 × 2^2 = 2^5", power_multiply(p(2, 3), p(2, 2), &r), r, "2^5");
    check_law("4^3 × 4^4 = 4^7", power_multiply(p(4, 3), p(4, 4), &r), r, "4^7");
    check_same("2^3 × 2^2 = 32", times(value(p(2, 3)), value(p(2, 2))), value(p(2, 5)));
    check_law("5^8 / 5^5 = 5^3", power_divide(p(5, 8), p(5, 5), &r), r, "5^3");
    check_law("7^15 / 7^9 = 7^6", power_divide(p(7, 15), p(7, 9), &r), r, "7^6");
    check_law("7^9 / 7^15 = 7^-6", power_divide(p(7, 9), p(7, 15), &r), r, "7^-6");
    check_law("(8^3)^4 = 8^12", power_of_power(p(8, 3), 4, &r), r, "8^12");
    check_law("(6^5)^200 = 6^1000", power_of_power(p(6, 5), 200, &r), r, "6^1000");

    printf("\n== Properties of exponents ==\n");
    check_value("#1 4^0 = 1", p(4, 0), 1, 1);
    check_law("#2 4^6 × 4^5 = 4^11", power_multiply(p(4, 6), p(4, 5), &r), r, "4^11");
    check_law("#2 3^2 × 3^-8 = 3^-6", power_multiply(p(3, 2), p(3, -8), &r), r, "3^-6");
    check_law("#3 4^6 ÷ 4^5 = 4^1", power_divide(p(4, 6), p(4, 5), &r), r, "4^1");
    check_law("#3 9^4 ÷ 9^-3 = 9^7", power_divide(p(9, 4), p(9, -3), &r), r, "9^7");
    check_law("#4 (5^2)^4 = 5^8", power_of_power(p(5, 2), 4, &r), r, "5^8");
    check_law("#4 (6^2)^4 = 6^8", power_of_power(p(6, 2), 4, &r), r, "6^8");
    check_same("#5 (6 × 7)^5 = 6^5 × 7^5", value(p(6 * 7, 5)), times(value(p(6, 5)), value(p(7, 5))));
    check_same("#5 (12 × 10)^-4 = 12^-4 × 10^-4", value(p(12 * 10, -4)), times(value(p(12, -4)), value(p(10, -4))));
    check_same("#6 8^-4 = 1 / 8^4", value(p(8, -4)), over(fraction_integer(1), value(p(8, 4))));
    check_same("#6 15^-4 = 1 / 15^4", value(p(15, -4)), over(fraction_integer(1), value(p(15, 4))));
    check_same("#7 (8/5)^4 = 8^4 / 5^4", value(power_make(fraction_make(8, 5), 4)), over(value(p(8, 4)), value(p(5, 4))));
    check_same("#7 (12/5)^-4 = 12^-4 / 5^-4", value(power_make(fraction_make(12, 5), -4)), over(value(p(12, -4)), value(p(5, -4))));
    strcpy(got, "(error)");
    if (power_root(fraction_integer(27), 3, &f) == 1) fraction_format(f, got, sizeof got);
    CHECK("#8 27^(1/3) = 3", strcmp(got, "3") == 0, got);

    return CHECK_DONE();
}
