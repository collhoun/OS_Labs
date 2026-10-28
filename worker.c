#include "worker.h"
#include <stdio.h>
#include <ctype.h>

void process_text_chunk(char *text, int len, ThreadResult *res)
{
    char buffer[130];
    int buf_len = 0;

    for (int i = 0; i <= len; i++)
    {
        char c = (i < len) ? text[i] : '\n';

        if (isspace(c))
        {
            if (buf_len > 0)
            {
                uint64_t temp[LIMBS];
                if (parse_hex_512(buffer, buf_len, temp) == 0)
                {
                    add_to_acc(res->sum, temp);
                    res->count++;
                }
                else
                {
                    res->error_flag = 1;
                    fprintf(stderr, "Ошибка: неверный формат числа\n");
                }
                buf_len = 0;
            }
        }
        else
        {
            if (buf_len < 128)
            {
                buffer[buf_len++] = c;
            }
            else
            {
                res->error_flag = 1;
                fprintf(stderr, "Ошибка: число слишком длинное\n");
                buf_len = 0;
            }
        }
    }
}