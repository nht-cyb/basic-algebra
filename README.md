# basic-algebra

Basic algebra functions in C, following the
[basic-mathematics.com algebra lessons](https://www.basic-mathematics.com/algebra-lessons.html).

## Build and run

```bash
make          # builds ./algebra from every file in src/
make test     # runs the lesson quizzes in tests/
make clean
```

## Writing an algebraic expression

[`expression.h`](include/expression.h) turns a phrase into an algebraic
expression, using the key words from the
[lesson](https://www.basic-mathematics.com/writing-an-algebraic-expression.html):

```bash
$ ./algebra "Seven more than three times a number x"
Seven more than three times a number x -> 3x + 7
```

| Key word | Phrase | Expression |
|---|---|---|
| more than | 6 more than n | `n + 6` |
| less than | two less than m | `m - 2` |
| sum | the sum of n and 6 | `n + 6` |
| difference | the difference of x and 9 | `x - 9` |
| product | the product of x and 10 | `10x` |
| quotient | the quotient of n and 5 | `n/5` |
| ratio | the ratio of y to 8 | `y/8` |
| twice, times | three times a number x | `3x` |

"a number" is `x` and "another number" is `y`, unless a letter follows, as in
"a number p". `added to` and `subtracted from` also work.
