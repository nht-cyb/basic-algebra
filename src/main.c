#include <stdio.h>
#include "expression.h"

int main(int argc, char **argv) {
    char text[256];
    int failed = 0;

    if (argc < 2) {
        fprintf(stderr, "usage: %s \"phrase\" ...\n", argv[0]);
        fprintf(stderr, "example: %s \"Seven more than three times a number x\"\n", argv[0]);
        return 2;
    }

    for (int i = 1; i < argc; i++) {
        Expr *e = expr_parse(argv[i]);
        if (e == NULL || expr_format(e, text, sizeof text) < 0) {
            printf("%s -> (not understood)\n", argv[i]);
            failed = 1;
        } else {
            printf("%s -> %s\n", argv[i], text);
        }
        expr_free(e);
    }
    return failed;
}
