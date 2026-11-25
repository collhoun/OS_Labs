#ifndef TRANSLATION_H
#define TRANSLATION_H

#include <limits.h>
#include <stdlib.h>

static char *translate_to_base(long x, unsigned int base)
{
    char digits[sizeof(long) * CHAR_BIT + 2];
    size_t length = 0;
    unsigned long value = (unsigned long)x;

    if (x < 0) {
        /* Unsigned subtraction also handles LONG_MIN. */
        value = 0UL - value;
    }

    do {
        digits[length++] = (char)('0' + value % base);
        value /= base;
    } while (value != 0);

    if (x < 0) {
        digits[length++] = '-';
    }

    char *result = malloc(length + 1);
    if (result == NULL) {
        return NULL;
    }

    for (size_t i = 0; i < length; ++i) {
        result[i] = digits[length - 1 - i];
    }
    result[length] = '\0';

    return result;
}

#endif