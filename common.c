#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

static int parse_float(const char *text, float *value)
{
    char *end;
    errno = 0;
    *value = strtof(text, &end);
    return end != text && *end == '\0' && errno == 0 && isfinite(*value);
}

static int parse_long(const char *text, long *value)
{
    char *end;
    errno = 0;
    *value = strtol(text, &end, 10);
    return end != text && *end == '\0' && errno == 0;
}

static void process_command(char *line, Functions *functions,
                            SwitchFn switch_library, void *context)
{
    const char *spaces = " \t\r\n\v\f";
    char *command = strtok(line, spaces);
    if (command == NULL) {
        return;
    }

    char *first = strtok(NULL, spaces);
    char *second = strtok(NULL, spaces);

    if (strcmp(command, "0") == 0 && first == NULL) {
        if (switch_library != NULL) {
            switch_library(functions, context);
        } else {
            fputs("Command 0 is only available in program2\n", stderr);
        }
        return;
    }

    if (strcmp(command, "1") == 0 && first != NULL && second != NULL &&
        strtok(NULL, spaces) == NULL) {
        float point;
        float step;

        if (!parse_float(first, &point) || !parse_float(second, &step) ||
            step == 0.0f) {
            fputs("Expected finite A and nonzero deltaX in float range\n", stderr);
            return;
        }
        printf("%.6f\n", functions->derivative(point, step));
        return;
    }

    if (strcmp(command, "2") == 0 && first != NULL && second == NULL) {
        long value;
        if (!parse_long(first, &value)) {
            fputs("Expected an integer in long range\n", stderr);
            return;
        }

        char *result = functions->translate(value);
        if (result == NULL) {
            fputs("Cannot allocate memory for the result\n", stderr);
            return;
        }

        puts(result);
        free(result);
        return;
    }

    fputs("Usage: 1 A deltaX | 2 x | 0 (program2 only)\n", stderr);
}

int run_commands(Functions *functions, SwitchFn switch_library, void *context)
{
    char *line = NULL;
    size_t capacity = 0;

    while (getline(&line, &capacity, stdin) != -1) {
        process_command(line, functions, switch_library, context);
    }
    free(line);

    if (!feof(stdin)) {
        perror("getline");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}