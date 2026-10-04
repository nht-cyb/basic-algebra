# basic-algebra

Basic algebra functions in C, following the
[basic-mathematics.com algebra lessons](https://www.basic-mathematics.com/algebra-lessons.html).

## Build and run

```bash
make          # builds ./algebra from every file in src/
make test     # checks every function against the lesson examples in tests/
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

## Exponents

[`exponent.h`](include/exponent.h) works with powers such as `4^11` or
`(2/3)^3`, following the lessons on
[exponents](https://www.basic-mathematics.com/exponents.html),
[laws of exponents](https://www.basic-mathematics.com/laws-of-exponents.html) and
[properties of exponents](https://www.basic-mathematics.com/properties-of-exponents.html).
Values are exact fractions, so `8^-4` is `1/4096`.

| Function | Property | Example |
|---|---|---|
| `power_value` | x^0 = 1, x^-n = 1 / x^n, (x/y)^n = x^n / y^n | `(2/3)^3` = `8/27`, `(-2)^7` = `-128` |
| `power_multiply` | x^n × x^m = x^(n + m) | `3^2 × 3^-8` = `3^-6` |
| `power_divide` | x^n ÷ x^m = x^(n - m) | `9^4 ÷ 9^-3` = `9^7` |
| `power_of_power` | (x^n)^m = x^(n × m) | `(6^5)^200` = `6^1000` |
| `power_root` | x^(1/n) = nth root of x | `27^(1/3)` = `3` |

`-2^6` is `-(2^6)` = `-64`, which is not `(-2)^6` = `64`. `power_format`
writes a negative base in parentheses so the two don't get mixed up.

## Rational and irrational numbers

`classify_number` in [`rational.h`](include/rational.h) says whether a
number is [rational](https://www.basic-mathematics.com/rational-numbers.html)
or [irrational](https://www.basic-mathematics.com/irrational-numbers.html),
and gives rational numbers as a fraction:

| Input | Result |
|---|---|
| `-8/2`, `0.75`, `0.150` | rational: `-4`, `3/4`, `3/20` |
| `0.(21)` (0.212121...) | rational: `7/33` |
| `√4`, `3√125` | rational: `2`, `5` |
| `√2`, `5√325`, `pi`, `e`, `golden ratio` | irrational |

Write a repeating decimal with the repeating part in parentheses. A
decimal without them ends where it is written, so it is always rational.

## Square roots

[`root.h`](include/root.h) follows the lessons on the
[square root](https://www.basic-mathematics.com/square-root-of-a-number.html),
the [square root algorithm](https://www.basic-mathematics.com/square-root-algorithm.html) and
[estimating the square root](https://www.basic-mathematics.com/estimate-the-square-root.html):

| Function | Example |
|---|---|
| `square_root(n, decimals, round, ...)` | √2685 to 2 decimals is `51.81`; √2 to 20 decimals is `1.41421356237309504880` |
| `square_root_between(n, &low, &high)` | √17 is between `4` and `5` |
| `square_root_estimate(n)` | √45 ≈ 7 - 4/14 = `6.714` |
| `root_exact(x, n, &root)` | the cube root of 125 is `5`; √2 is not a whole number |

`square_root` uses the digit-by-digit algorithm from the lesson, so every
digit is exact, up to 25 decimals.

## Linear equations

`linear_solve` in [`linear.h`](include/linear.h) solves a linear equation in
one variable, step by step, the way the lessons do:
[one-step](https://www.basic-mathematics.com/solve-one-step-equations.html),
[multiplication](https://www.basic-mathematics.com/solving-multiplication-equations.html),
[two-step](https://www.basic-mathematics.com/solving-two-step-equations.html),
[variable on both sides](https://www.basic-mathematics.com/solving-an-equation-with-a-variable-on-both-sides.html) and
[distributive property](https://www.basic-mathematics.com/solve-equations-using-the-distributive-property.html).

`./algebra` solves any argument that has an `=` sign:

```bash
$ ./algebra "9x - 12 = 5x + 8"
9x - 12 = 5x + 8
  4x - 12 = 8              Subtract 5x from each side
  4x = 20                  Add 12 to each side
  x = 5                    Divide each side by 4
Solution: x = 5
```

It understands numbers (`2`, `3.1`), one letter as the variable, `+ - * /`,
`×`, `÷`, brackets, and a number or bracket written in front of the variable
to multiply it (`2x`, `2(x - 4)`, `(2/5)x`). Answers are exact fractions, so
`1.5x = 0.75` gives `x = 1/2`.

It also says when an equation has no solution (`x + 1 = x + 2`), when every
number is a solution (`2(x + 1) = 2x + 2`), and why it cannot solve one, for
example `x*x = 4` (not linear) or `y = 2x + 5` (two variables).

## Systems of linear equations

`system_solve` in [`system.h`](include/system.h) solves two linear equations in
two variables with the
[elimination method](https://www.basic-mathematics.com/elimination-method.html),
step by step. Give `./algebra` the two equations separated by `;`:

```bash
$ ./algebra "3x + y = 10; -4x - 2y = 2"
  3x + y = 10              Equation 1
  -4x - 2y = 2             Equation 2
  6x + 2y = 20             Multiply equation 1 by 2
  2x = 22                  Add the two equations to eliminate y
  x = 11                   Divide each side by 2
  y + 33 = 10              Substitute x = 11 into equation 1
  y = -23                  Subtract 33 from each side
Solution: x = 11, y = -23
```

Following [the number of solutions](https://www.basic-mathematics.com/solutions-of-systems-of-linear-equations.html),
it says whether the system has one solution, no solution (parallel lines,
e.g. `y = 3x + 5; y = 3x - 8`), or infinitely many solutions (the same line
twice). For infinitely many it gives the line, as in
[this lesson](https://www.basic-mathematics.com/solve-a-system-without-a-unique-solution.html):
`4x - y = 5; -4x + y = -5` gives every `(x, y)` with `y = 4x - 5`.

The equations can be written in any linear form, such as `y = (2/5)x + 1`,
`25 × q + 10 × d = 450` or `2(x + y) = 3x - 1`.
