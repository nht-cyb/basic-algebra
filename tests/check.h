/* A tiny test helper: CHECK prints PASS or FAIL for each case and
   counts the failures; CHECK_DONE prints the score and gives main's
   return value. */

#ifndef CHECK_H
#define CHECK_H

#include <stdio.h>

static int check_total, check_failed;

#define CHECK(label, condition, got)                                   \
    do {                                                               \
        int check_ok_ = (condition);                                   \
        check_total++;                                                 \
        if (!check_ok_) check_failed++;                                \
        printf("%s %s -> %s\n", check_ok_ ? "PASS" : "FAIL", (label), (got)); \
    } while (0)

#define CHECK_DONE()                                                   \
    (printf("Score: %d out of %d\n", check_total - check_failed, check_total), \
     check_failed == 0 ? 0 : 1)

#endif
