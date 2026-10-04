/* Examples from
   https://www.basic-mathematics.com/rational-numbers.html
   https://www.basic-mathematics.com/irrational-numbers.html
   A repeating decimal such as 0.2121212121... is written 0.(21). */

#include "check.h"
#include "rational.h"

/* expected_num/expected_den is only checked for rational numbers */
static void check_number(const char *text, NumberKind expected, long expected_num, long expected_den) {
    Fraction v = { 0, 1 };
    char got[64], value[48] = "";
    NumberKind kind = classify_number(text, &v);
    int ok = kind == expected;

    if (kind == NUMBER_RATIONAL) {
        fraction_format(v, value, sizeof value);
        if (expected == NUMBER_RATIONAL) {
            ok = ok && fraction_equal(v, fraction_make(expected_num, expected_den));
        }
    }
    snprintf(got, sizeof got, "%s%s%s",
             kind == NUMBER_RATIONAL ? "rational" :
             kind == NUMBER_IRRATIONAL ? "irrational" : "invalid",
             kind == NUMBER_RATIONAL ? " = " : "", value);
    CHECK(text, ok, got);
}

#define RATIONAL(text, num, den) check_number(text, NUMBER_RATIONAL, num, den)
#define IRRATIONAL(text) check_number(text, NUMBER_IRRATIONAL, 0, 1)

int main(void) {
    printf("== Rational numbers ==\n");
    RATIONAL("2/3", 2, 3);
    RATIONAL("5/2", 5, 2);
    RATIONAL("1/4", 1, 4);
    RATIONAL("2", 2, 1);
    RATIONAL("-8/2", -4, 1);
    RATIONAL("0", 0, 1);
    RATIONAL("2/5", 2, 5);
    RATIONAL("0.4", 2, 5);
    RATIONAL("40/100", 2, 5);
    RATIONAL("0.(251)", 251, 999);
    RATIONAL("0.150", 3, 20);
    RATIONAL("9", 9, 1);
    RATIONAL("-8", -8, 1);
    RATIONAL("0.(21)", 7, 33);
    RATIONAL("0.75", 3, 4);
    RATIONAL("-1/2", -1, 2);
    RATIONAL("15/6", 5, 2);
    RATIONAL("-4/3", -4, 3);
    RATIONAL("6/20", 3, 10);

    printf("\n== Irrational numbers ==\n");
    RATIONAL("√4", 2, 1);
    IRRATIONAL("√2");
    IRRATIONAL("√7");
    IRRATIONAL("√35");
    IRRATIONAL("√8");
    RATIONAL("1.(2)", 11, 9);
    RATIONAL("4.(36)", 48, 11);
    IRRATIONAL("pi");
    IRRATIONAL("golden ratio");
    IRRATIONAL("e");
    RATIONAL("15.(8451)", 17604, 1111);
    RATIONAL("3√125", 5, 1);
    IRRATIONAL("5√325");

    return CHECK_DONE();
}
