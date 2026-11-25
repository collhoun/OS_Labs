#ifndef COMMON_H
#define COMMON_H

#include "contract.h"

typedef float (*DerivativeFn)(float, float);
typedef char *(*TranslationFn)(long);

typedef struct {
    DerivativeFn derivative;
    TranslationFn translate;
} Functions;

typedef void (*SwitchFn)(Functions *, void *);

int run_commands(Functions *functions, SwitchFn switch_library, void *context);

#endif