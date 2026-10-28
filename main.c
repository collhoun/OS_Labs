#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <time.h>
#include "bigint.h"
#include "worker.h"

typedef struct
{
    char *data;
    int len;
} Task;

static Task *queue;
static int q_cap = 0;
static int q_head = 0;
static int q_tail = 0;
static int q_count = 0;
static int end_of_file = 0;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond_not_empty = PTHREAD_COND_INITIALIZER;
static pthread_cond_t cond_not_full = PTHREAD_COND_INITIALIZER;

static void *worker_thread(void *arg)
{
    ThreadResult *res = (ThreadResult *)arg;

    while (1)
    {
        pthread_mutex_lock(&mutex);

        while (q_count == 0 && !end_of_file)
            pthread_cond_wait(&cond_not_empty, &mutex);

        if (q_count == 0 && end_of_file)
        {
            pthread_mutex_unlock(&mutex);
            break;
        }

        Task task = queue[q_head];
        q_head = (q_head + 1) % q_cap;
        q_count--;

        pthread_cond_signal(&cond_not_full);
        pthread_mutex_unlock(&mutex);

        process_text_chunk(task.data, task.len, res);
        free(task.data);
    }

    return NULL;
}

int main(int argc, char **argv)
{
    char *filename = NULL;
    int num_threads = 1;
    long mem_mb = 64;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-f") == 0 && i + 1 < argc)
            filename = argv[++i];
        else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc)
            num_threads = atoi(argv[++i]);
        else if (strcmp(argv[i], "-m") == 0 && i + 1 < argc)
            mem_mb = atol(argv[++i]);
    }

    if (!filename || num_threads < 1 || mem_mb < 1)
    {
        printf("Использование: %s -f <файл> -t <потоки> [-m <память, МБ>]\n", argv[0]);
        return 1;
    }

    long mem_bytes = mem_mb * 1024L * 1024L;
    int chunk_size = 64 * 1024;
    long slots = mem_bytes / chunk_size - num_threads - 2;
    if (slots < 1)
    {
        chunk_size = (int)(mem_bytes / (num_threads + 3));
        slots = 1;
        if (chunk_size < 4096)
        {
            fprintf(stderr, "Слишком мало памяти для %d потоков\n", num_threads);
            return 1;
        }
    }
    q_cap = (int)(slots > 1024 ? 1024 : slots);

    FILE *file = fopen(filename, "r");
    if (!file)
    {
        perror("Ошибка открытия файла");
        return 1;
    }

    queue = malloc(q_cap * sizeof(Task));
    pthread_t *threads = malloc(num_threads * sizeof(pthread_t));
    ThreadResult *results = calloc(num_threads, sizeof(ThreadResult));

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < num_threads; i++)
    {
        results[i].id = i;
        pthread_create(&threads[i], NULL, worker_thread, &results[i]);
    }

    char *buffer = malloc(chunk_size + 1);
    char tail[512] = {0};
    int tail_len = 0;

    while (!feof(file))
    {
        memcpy(buffer, tail, tail_len);

        size_t bytes_read = fread(buffer + tail_len, 1, chunk_size - tail_len, file);
        int total_len = tail_len + bytes_read;

        if (total_len == 0)
            break;

        int cut_pos = total_len;
        if (!feof(file))
        {
            while (cut_pos > 0 && !isspace((unsigned char)buffer[cut_pos - 1]))
                cut_pos--;
        }

        tail_len = total_len - cut_pos;
        if (tail_len >= (int)sizeof(tail))
        {
            fprintf(stderr, "Ошибка: слишком длинная строка без разделителей\n");
            break;
        }
        memcpy(tail, buffer + cut_pos, tail_len);

        Task new_task;
        new_task.len = cut_pos;
        new_task.data = malloc(cut_pos > 0 ? cut_pos : 1);
        memcpy(new_task.data, buffer, cut_pos);

        pthread_mutex_lock(&mutex);
        while (q_count == q_cap)
            pthread_cond_wait(&cond_not_full, &mutex);
        queue[q_tail] = new_task;
        q_tail = (q_tail + 1) % q_cap;
        q_count++;
        pthread_cond_signal(&cond_not_empty);
        pthread_mutex_unlock(&mutex);
    }

    pthread_mutex_lock(&mutex);
    end_of_file = 1;
    pthread_cond_broadcast(&cond_not_empty);
    pthread_mutex_unlock(&mutex);

    uint64_t total_sum[ACC_LIMBS] = {0};
    uint64_t total_count = 0;
    int error = 0;

    for (int i = 0; i < num_threads; i++)
    {
        pthread_join(threads[i], NULL);
        merge_accs(total_sum, results[i].sum);
        total_count += results[i].count;
        if (results[i].error_flag)
            error = 1;
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    if (total_count > 0)
    {
        uint64_t mean[ACC_LIMBS];
        div_acc_by_n(total_sum, total_count, mean);

        printf("Обработано чисел: %llu\n", (unsigned long long)total_count);
        printf("Среднее значение: 0x");
        print_bigint(mean, ACC_LIMBS);
        printf("Время работы: %.3f сек\n", elapsed);
    }
    else
    {
        printf("Числа не найдены.\n");
    }

    free(buffer);
    free(queue);
    free(threads);
    free(results);
    fclose(file);

    return error ? 2 : 0;
}