#include "bigint.h"
#include <stdio.h>
#include <string.h>

static int hex_char_to_int(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

int parse_hex_512(const char *str, int len, uint64_t *result)
{
    memset(result, 0, LIMBS * sizeof(uint64_t));
    for (int i = 0; i < len; i++)
    {
        int val = hex_char_to_int(str[i]);
        if (val < 0)
            return -1;
        int pos = len - 1 - i;
        result[pos / 16] |= ((uint64_t)val) << (4 * (pos % 16));
    }
    return 0;
}

void add_to_acc(uint64_t *acc, const uint64_t *val)
{
    uint64_t carry = 0;
    for (int i = 0; i < ACC_LIMBS; i++)
    {
        uint64_t v = (i < LIMBS) ? val[i] : 0;

        uint64_t sum = acc[i] + v;
        uint64_t next_carry = (sum < acc[i]) ? 1 : 0;

        sum += carry;
        if (sum < carry)
            next_carry = 1;

        acc[i] = sum;
        carry = next_carry;
    }
}

void merge_accs(uint64_t *dest, const uint64_t *src)
{
    uint64_t carry = 0;
    for (int i = 0; i < ACC_LIMBS; i++)
    {
        uint64_t sum = dest[i] + src[i];
        uint64_t next_carry = (sum < dest[i]) ? 1 : 0;
        sum += carry;
        if (sum < carry)
            next_carry = 1;
        dest[i] = sum;
        carry = next_carry;
    }
}

void div_acc_by_n(const uint64_t *sum, uint64_t n, uint64_t *mean)
{
    uint64_t rem = 0;
    memset(mean, 0, ACC_LIMBS * sizeof(uint64_t));

    for (int i = ACC_LIMBS - 1; i >= 0; i--)
    {
        unsigned __int128 cur = ((unsigned __int128)rem << 64) | sum[i];
        mean[i] = (uint64_t)(cur / n);
        rem = (uint64_t)(cur % n);
    }
    if (rem >= n - rem)
    {
        uint64_t carry = 1;
        for (int i = 0; i < ACC_LIMBS && carry; i++)
        {
            mean[i] += carry;
            carry = (mean[i] == 0) ? 1 : 0;
        }
    }
}

void print_bigint(const uint64_t *val, int limbs)
{
    int started = 0;
    for (int i = limbs - 1; i >= 0; i--)
    {
        if (started)
        {
            printf("%016llx", (unsigned long long)val[i]);
        }
        else if (val[i] != 0 || i == 0)
        {
            printf("%llx", (unsigned long long)val[i]);
            started = 1;
        }
    }
    printf("\n");
}