#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "expression.h"

/* ---- Building blocks ---- */

static Expr *expr_new(ExprKind kind) {
    Expr *e = calloc(1, sizeof *e);
    if (e != NULL) {
        e->kind = kind;
    }
    return e;
}

Expr *expr_number(long value) {
    Expr *e = expr_new(EXPR_NUMBER);
    if (e != NULL) {
        e->value = value;
    }
    return e;
}

Expr *expr_variable(char name) {
    Expr *e = expr_new(EXPR_VARIABLE);
    if (e != NULL) {
        e->name = name;
    }
    return e;
}

static Expr *expr_operator(ExprKind kind, Expr *left, Expr *right) {
    Expr *e = NULL;
    if (left != NULL && right != NULL) {
        e = expr_new(kind);
    }
    if (e == NULL) {
        expr_free(left);
        expr_free(right);
        return NULL;
    }
    e->left = left;
    e->right = right;
    return e;
}

Expr *expr_add(Expr *left, Expr *right) {
    return expr_operator(EXPR_ADD, left, right);
}

Expr *expr_subtract(Expr *left, Expr *right) {
    return expr_operator(EXPR_SUBTRACT, left, right);
}

Expr *expr_multiply(Expr *left, Expr *right) {
    return expr_operator(EXPR_MULTIPLY, left, right);
}

Expr *expr_divide(Expr *left, Expr *right) {
    return expr_operator(EXPR_DIVIDE, left, right);
}

void expr_free(Expr *e) {
    if (e == NULL) {
        return;
    }
    expr_free(e->left);
    expr_free(e->right);
    free(e);
}

/* ---- Writing an expression as text ---- */

typedef struct {
    char *out;
    size_t size;
    size_t len;
    int overflow;
} Writer;

static void put(Writer *w, const char *s) {
    size_t n = strlen(s);
    if (w->len + n >= w->size) {
        w->overflow = 1;
        return;
    }
    memcpy(w->out + w->len, s, n + 1);
    w->len += n;
}

static int is_sum(const Expr *e) {
    return e->kind == EXPR_ADD || e->kind == EXPR_SUBTRACT;
}

static int is_atom(const Expr *e) {
    return e->kind == EXPR_NUMBER || e->kind == EXPR_VARIABLE;
}

/* Numbers go in front of a product: x times 10 is written 10x. */
static void product_order(const Expr *e, const Expr **first, const Expr **second) {
    if (e->right->kind == EXPR_NUMBER && e->left->kind != EXPR_NUMBER) {
        *first = e->right;
        *second = e->left;
    } else {
        *first = e->left;
        *second = e->right;
    }
}

/* Whether e, written without parentheses, starts with a digit. */
static int starts_with_digit(const Expr *e) {
    const Expr *first, *second;
    switch (e->kind) {
    case EXPR_NUMBER:
        return 1;
    case EXPR_VARIABLE:
        return 0;
    case EXPR_MULTIPLY:
        product_order(e, &first, &second);
        return starts_with_digit(first);
    default:
        return starts_with_digit(e->left);
    }
}

static void write_expr(Writer *w, const Expr *e);

static void write_wrapped(Writer *w, const Expr *e, int wrap) {
    if (wrap) put(w, "(");
    write_expr(w, e);
    if (wrap) put(w, ")");
}

static void write_expr(Writer *w, const Expr *e) {
    char buf[32];
    const Expr *first, *second;
    int wrap_second;

    switch (e->kind) {
    case EXPR_NUMBER:
        snprintf(buf, sizeof buf, "%ld", e->value);
        put(w, buf);
        break;
    case EXPR_VARIABLE:
        buf[0] = e->name;
        buf[1] = '\0';
        put(w, buf);
        break;
    case EXPR_ADD:
        write_expr(w, e->left);
        put(w, " + ");
        write_expr(w, e->right);
        break;
    case EXPR_SUBTRACT:
        write_expr(w, e->left);
        put(w, " - ");
        write_wrapped(w, e->right, is_sum(e->right)); /* x - (y + 2) */
        break;
    case EXPR_MULTIPLY:
        product_order(e, &first, &second);
        wrap_second = !is_atom(second) && second->kind != EXPR_MULTIPLY;
        write_wrapped(w, first, is_sum(first) || first->kind == EXPR_DIVIDE);
        if (!wrap_second && starts_with_digit(second)) {
            put(w, " * "); /* 5 * 6, not 56 */
        }
        write_wrapped(w, second, wrap_second); /* 3x, 5(n + 6) */
        break;
    case EXPR_DIVIDE:
        write_wrapped(w, e->left, is_sum(e->left));
        put(w, "/");
        write_wrapped(w, e->right, !is_atom(e->right));
        break;
    }
}

int expr_format(const Expr *e, char *out, size_t size) {
    Writer w = { out, size, 0, 0 };
    if (size == 0) {
        return -1;
    }
    out[0] = '\0';
    write_expr(&w, e);
    return w.overflow ? -1 : (int)w.len;
}

/* ---- Reading a verbal phrase ---- */

#define MAX_TOKENS 64
#define MAX_TOKEN_LEN 32

typedef struct {
    char tokens[MAX_TOKENS][MAX_TOKEN_LEN];
    int count;
    int pos;
} Parser;

static const char *number_words[] = {
    "zero", "one", "two", "three", "four", "five", "six", "seven", "eight",
    "nine", "ten", "eleven", "twelve", "thirteen", "fourteen", "fifteen",
    "sixteen", "seventeen", "eighteen", "nineteen", "twenty"
};

static const char *tens_words[] = {
    "thirty", "forty", "fifty", "sixty", "seventy", "eighty", "ninety"
};

/* Splits the phrase into lowercase words, dropping punctuation. */
static int tokenize(Parser *p, const char *phrase) {
    int len = 0;
    p->count = 0;
    p->pos = 0;
    for (const char *c = phrase;; c++) {
        if (*c != '\0' && isalnum((unsigned char)*c)) {
            if (len == MAX_TOKEN_LEN - 1) {
                return -1;
            }
            p->tokens[p->count][len++] = (char)tolower((unsigned char)*c);
        } else if (len > 0) {
            p->tokens[p->count][len] = '\0';
            len = 0;
            if (++p->count == MAX_TOKENS) {
                return -1;
            }
        }
        if (*c == '\0') {
            return 0;
        }
    }
}

static const char *peek(const Parser *p, int ahead) {
    int i = p->pos + ahead;
    return i < p->count ? p->tokens[i] : "";
}

/* Consumes the given words if they come next. */
static int accept(Parser *p, const char *word) {
    if (strcmp(peek(p, 0), word) == 0) {
        p->pos++;
        return 1;
    }
    return 0;
}

static int accept2(Parser *p, const char *word1, const char *word2) {
    if (strcmp(peek(p, 0), word1) == 0 && strcmp(peek(p, 1), word2) == 0) {
        p->pos += 2;
        return 1;
    }
    return 0;
}

static int read_number(const char *word, long *value) {
    size_t i;
    if (isdigit((unsigned char)word[0])) {
        char *end;
        *value = strtol(word, &end, 10);
        return *end == '\0';
    }
    for (i = 0; i < sizeof number_words / sizeof number_words[0]; i++) {
        if (strcmp(word, number_words[i]) == 0) {
            *value = (long)i;
            return 1;
        }
    }
    for (i = 0; i < sizeof tens_words / sizeof tens_words[0]; i++) {
        if (strcmp(word, tens_words[i]) == 0) {
            *value = 30 + 10 * (long)i;
            return 1;
        }
    }
    return 0;
}

static int is_letter(const char *word) {
    return isalpha((unsigned char)word[0]) && word[1] == '\0';
}

/* A number, a letter, "a number [letter]" or "another number [letter]". */
static Expr *parse_operand(Parser *p) {
    long value;
    char name;

    if (accept2(p, "a", "number")) {
        name = 'x';
    } else if (accept2(p, "another", "number")) {
        name = 'y';
    } else if (read_number(peek(p, 0), &value)) {
        p->pos++;
        return expr_number(value);
    } else if (is_letter(peek(p, 0))) {
        return expr_variable(p->tokens[p->pos++][0]);
    } else {
        return NULL;
    }
    if (is_letter(peek(p, 0))) {
        name = p->tokens[p->pos++][0];
    }
    return expr_variable(name);
}

static Expr *parse_expr(Parser *p);

/* "<expr> <joiner> <expr>", as in "x and 9" or "y to 8". */
static int parse_pair(Parser *p, const char *joiner, Expr **left, Expr **right) {
    *left = parse_expr(p);
    if (*left == NULL || !accept(p, joiner)) {
        expr_free(*left);
        return 0;
    }
    *right = parse_expr(p);
    if (*right == NULL) {
        expr_free(*left);
        return 0;
    }
    return 1;
}

static Expr *parse_expr(Parser *p) {
    Expr *left, *right;

    accept(p, "the");

    /* "the sum of a and b", ... : the number after "of" comes first */
    if (accept2(p, "sum", "of")) {
        return parse_pair(p, "and", &left, &right) ? expr_add(left, right) : NULL;
    }
    if (accept2(p, "difference", "of")) {
        return parse_pair(p, "and", &left, &right) ? expr_subtract(left, right) : NULL;
    }
    if (accept2(p, "product", "of")) {
        return parse_pair(p, "and", &left, &right) ? expr_multiply(left, right) : NULL;
    }
    if (accept2(p, "quotient", "of")) {
        return parse_pair(p, "and", &left, &right) ? expr_divide(left, right) : NULL;
    }
    if (accept2(p, "ratio", "of")) {
        return parse_pair(p, "to", &left, &right) ? expr_divide(left, right) : NULL;
    }
    if (accept(p, "twice")) {
        return expr_multiply(expr_number(2), parse_expr(p));
    }

    left = parse_operand(p);
    if (left == NULL) {
        return NULL;
    }

    /* "a times b" */
    if (accept(p, "times")) {
        return expr_multiply(left, parse_expr(p));
    }
    /* "a more than b" is b + a: the number after "than" comes first */
    if (accept2(p, "more", "than") || accept2(p, "added", "to")) {
        return expr_add(parse_expr(p), left);
    }
    /* "a less than b" is b - a */
    if (accept2(p, "less", "than") || accept2(p, "subtracted", "from")) {
        return expr_subtract(parse_expr(p), left);
    }
    return left;
}

Expr *expr_parse(const char *phrase) {
    Parser p;
    Expr *e;

    if (tokenize(&p, phrase) != 0) {
        return NULL;
    }
    e = parse_expr(&p);
    if (e != NULL && p.pos != p.count) { /* words left over */
        expr_free(e);
        return NULL;
    }
    return e;
}
