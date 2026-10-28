#ifndef BIGINT_H
#define BIGINT_H

#include <stdint.h>

#define LIMBS 8
#define ACC_LIMBS 9

int parse_hex_512(const char *str, int len, uint64_t *result);
void add_to_acc(uint64_t *acc, const uint64_t *val);
void merge_accs(uint64_t *dest, const uint64_t *src);
void div_acc_by_n(const uint64_t *sum, uint64_t n, uint64_t *mean);
void print_bigint(const uint64_t *val, int limbs);

#endif