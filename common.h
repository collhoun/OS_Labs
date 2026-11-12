#ifndef COMMON_H
#define COMMON_H

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <semaphore.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

typedef struct
{
    sem_t run[2];
    sem_t done[2];

    size_t length;
    int stop;
} Shared;

void fail(const char *message)
{
    perror(message);
    exit(EXIT_FAILURE);
}

off_t text_offset(void)
{
    long page = sysconf(_SC_PAGESIZE);

    if (page <= 0)
    {
        fprintf(stderr, "Cannot determine page size\n");
        exit(EXIT_FAILURE);
    }

    size_t page_size = (size_t)page;
    size_t pages =
        (sizeof(Shared) + page_size - 1) / page_size;

    return (off_t)(pages * page_size);
}

Shared *map_header(int fd)
{
    Shared *shared = mmap(
        NULL,
        sizeof(Shared),
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        fd,
        0);

    if (shared == MAP_FAILED)
    {
        fail("mmap header");
    }

    return shared;
}

char *map_text(int fd, size_t bytes)
{
    char *text = mmap(
        NULL,
        bytes,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        fd,
        text_offset());

    if (text == MAP_FAILED)
    {
        fail("mmap text");
    }

    return text;
}

void wait_sem(sem_t *sem)
{
    while (sem_wait(sem) == -1)
    {
        if (errno != EINTR)
        {
            fail("sem_wait");
        }
    }
}

void post_sem(sem_t *sem)
{
    if (sem_post(sem) == -1)
    {
        fail("sem_post");
    }
}

int worker_main(int argc, char **argv, int index)
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s shared_file\n", argv[0]);
        return EXIT_FAILURE;
    }

    int fd = open(argv[1], O_RDWR);

    if (fd == -1)
    {
        fail("open shared file");
    }

    Shared *shared = map_header(fd);

    for (;;)
    {

        wait_sem(&shared->run[index]);

        if (shared->stop)
        {
            break;
        }

        size_t length = shared->length;

        if (length == SIZE_MAX)
        {
            fprintf(stderr, "Invalid string length\n");
            return EXIT_FAILURE;
        }

        size_t bytes = length + 1;

        struct stat info;

        if (fstat(fd, &info) == -1)
        {
            fail("fstat");
        }

        off_t offset = text_offset();

        if (info.st_size < offset ||
            (uintmax_t)bytes >
                (uintmax_t)(info.st_size - offset))
        {
            fprintf(stderr, "Shared file is too small\n");
            return EXIT_FAILURE;
        }

        char *text = map_text(fd, bytes);

        if (index == 0)
        {

            for (size_t i = 0; i < length; ++i)
            {
                text[i] =
                    (char)tolower((unsigned char)text[i]);
            }
        }
        else
        {

            size_t write_pos = 0;
            int previous_space = 0;

            for (size_t read_pos = 0;
                 read_pos < length;
                 ++read_pos)
            {
                int current_space =
                    (text[read_pos] == ' ');

                if (current_space && previous_space)
                {
                    continue;
                }

                text[write_pos++] = text[read_pos];
                previous_space = current_space;
            }

            text[write_pos] = '\0';
            shared->length = write_pos;
        }

        if (munmap(text, bytes) == -1)
        {
            fail("munmap text");
        }

        post_sem(&shared->done[index]);
    }

    if (munmap(shared, sizeof(Shared)) == -1)
    {
        fail("munmap header");
    }

    if (close(fd) == -1)
    {
        fail("close");
    }

    return EXIT_SUCCESS;
}

#endif