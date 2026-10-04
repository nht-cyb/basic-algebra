CC = gcc
CFLAGS = -Wall -Wextra -Iinclude

SRCS = $(wildcard src/*.c)
LIB_SRCS = $(filter-out src/main.c, $(SRCS))
HDRS = $(wildcard include/*.h)

algebra: $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) -o $@ $(SRCS)

tests/test_quiz: tests/test_quiz.c $(LIB_SRCS) $(HDRS)
	$(CC) $(CFLAGS) -o $@ tests/test_quiz.c $(LIB_SRCS)

test: tests/test_quiz
	./tests/test_quiz

clean:
	rm -f algebra tests/test_quiz

.PHONY: test clean
