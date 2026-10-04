CC = gcc
CFLAGS = -Wall -Wextra -Iinclude
LDLIBS = -lm

SRCS = $(wildcard src/*.c)
LIB_SRCS = $(filter-out src/main.c, $(SRCS))
HDRS = $(wildcard include/*.h)
TESTS = $(patsubst %.c, %, $(wildcard tests/test_*.c))

algebra: $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(LDLIBS)

tests/test_%: tests/test_%.c tests/check.h $(LIB_SRCS) $(HDRS)
	$(CC) $(CFLAGS) -o $@ $< $(LIB_SRCS) $(LDLIBS)

# runs every test, then fails if any of them failed
test: $(TESTS)
	@failed=0; for t in $(TESTS); do echo "## $$t"; ./$$t || failed=1; echo; done; exit $$failed

clean:
	rm -f algebra $(TESTS)

.PHONY: test clean
