SHELL := /bin/bash

CC = gcc
CPPFLAGS = -D_POSIX_C_SOURCE=200809L
CFLAGS = -std=c11 -Wall -Wextra -g

LIBRARIES = liblab1.so liblab2.so
PROGRAMS = program1 program2

.DEFAULT_GOAL := all

.PHONY: all run run-dynamic test clean

all: $(LIBRARIES) $(PROGRAMS)

liblab%.so: lib%.c contract.h translation.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -fPIC $(LDFLAGS) -shared $< -o $@ -lm

common.o: common.c common.h contract.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

program1: program1.c common.o common.h contract.h liblab1.so
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) program1.c common.o -o $@ \
		-L. -Wl,-rpath,'$$ORIGIN' -llab1

program2: program2.c common.o common.h contract.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) program2.c common.o -o $@ -ldl

run: program1
	./program1

run-dynamic: $(LIBRARIES) program2
	./program2

test: all
	@set -o pipefail; printf '1 1 0.1\n2 10\n2 -10\n2 0\n' | ./program1 | \
		diff -u <(printf '%s\n' -0.867062 1010 -1010 0) -
	@set -o pipefail; printf '1 1 0.1\n2 10\n0\n1 1 0.1\n2 -10\n2 0\n0\n2 10' | ./program2 | \
		diff -u <(printf '%s\n' -0.867062 1010 -0.840069 -101 0 1010) -
	@printf 'Tests passed\n'

clean:
	$(RM) $(PROGRAMS) $(LIBRARIES) common.o