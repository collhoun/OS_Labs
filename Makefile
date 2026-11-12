CC = gcc

CPPFLAGS = -D_POSIX_C_SOURCE=200809L -D_FILE_OFFSET_BITS=64
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -g -pthread
LDLIBS = -pthread

PROGRAMS = parent child1 child2

.DEFAULT_GOAL := all

.PHONY: all run test clean

all: $(PROGRAMS)

$(PROGRAMS): %: %.c common.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LDLIBS)

run: all
	./parent

test: all
	@printf 'HeLLo    WORLD\nABC   DEF     GHI\n\n    \nEND' | ./parent | diff -u --label expected --label actual - <(printf 'hello world\nabc def ghi\n\n \nend')
	@printf 'Test passed\n'

clean:
	$(RM) $(PROGRAMS)
SHELL := /bin/bash