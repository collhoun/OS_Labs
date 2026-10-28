#ifndef WORKER_H
#define WORKER_H

#include "bigint.h"

// Структура, в которой поток хранит свои результаты
typedef struct
{
    uint64_t sum[ACC_LIMBS];
    uint64_t count;
    int id;         // Номер потока (для отладки)
    int error_flag; // 1 если была ошибка парсинга
} ThreadResult;

// Функция для обработки одного большого куска текста
void process_text_chunk(char *text, int len, ThreadResult *res);

#endif