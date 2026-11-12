#include "common.h"

pid_t start_child(
    const char *program,
    const char *shared_path)
{
    pid_t pid = fork();

    if (pid == -1)
    {
        return -1;
    }

    if (pid == 0)
    {
        execl(
            program,
            program,
            shared_path,
            (char *)NULL);

        perror(program);
        _exit(127);
    }

    return pid;
}

int check_children(pid_t children[2])
{
    for (int i = 0; i < 2; ++i)
    {
        if (children[i] <= 0)
        {
            errno = ECHILD;
            return -1;
        }

        int status;
        pid_t result;

        do
        {
            result = waitpid(
                children[i],
                &status,
                WNOHANG);
        } while (result == -1 && errno == EINTR);

        if (result == -1)
        {
            return -1;
        }

        if (result > 0)
        {
            fprintf(
                stderr,
                "child%d terminated unexpectedly\n",
                i + 1);

            children[i] = 0;
            errno = ECHILD;
            return -1;
        }
    }

    return 0;
}

int wait_done(
    sem_t *sem,
    pid_t children[2])
{
    for (;;)
    {
        struct timespec deadline;

        if (clock_gettime(CLOCK_REALTIME, &deadline) == -1)
        {
            return -1;
        }

        deadline.tv_sec += 1;

        if (sem_timedwait(sem, &deadline) == 0)
        {
            return 0;
        }

        if (errno == EINTR)
        {
            continue;
        }

        if (errno != ETIMEDOUT)
        {
            return -1;
        }

        if (check_children(children) == -1)
        {
            return -1;
        }
    }
}

int join_child(pid_t *pid)
{
    if (*pid <= 0)
    {
        return 0;
    }

    int status;
    pid_t result;

    do
    {
        result = waitpid(*pid, &status, 0);
    } while (result == -1 && errno == EINTR);

    if (result == -1)
    {
        return -1;
    }

    *pid = 0;

    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
    {
        errno = ECHILD;
        return -1;
    }

    return 0;
}

int main(void)
{
    int result = EXIT_FAILURE;
    int fd = -1;

    Shared *shared = MAP_FAILED;

    pid_t children[2] = {0, 0};

    char path[] = "/tmp/lab4-XXXXXX";
    int file_created = 0;

    char *line = NULL;
    size_t allocated = 0;

    sem_t *semaphores[4];
    size_t initialized = 0;

    fd = mkstemp(path);

    if (fd == -1)
    {
        perror("mkstemp");
        goto cleanup;
    }

    file_created = 1;

    off_t offset = text_offset();

    if (ftruncate(fd, offset + 1) == -1)
    {
        perror("ftruncate");
        goto cleanup;
    }

    shared = mmap(
        NULL,
        sizeof(Shared),
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        fd,
        0);

    if (shared == MAP_FAILED)
    {
        perror("mmap header");
        goto cleanup;
    }

    shared->length = 0;
    shared->stop = 0;

    semaphores[0] = &shared->run[0];
    semaphores[1] = &shared->run[1];
    semaphores[2] = &shared->done[0];
    semaphores[3] = &shared->done[1];

    for (size_t i = 0; i < 4; ++i)
    {
        if (sem_init(semaphores[i], 1, 0) == -1)
        {
            perror("sem_init");
            goto cleanup;
        }

        ++initialized;
    }

    children[0] = start_child("./child1", path);

    if (children[0] == -1)
    {
        perror("fork child1");
        goto cleanup;
    }

    children[1] = start_child("./child2", path);

    if (children[1] == -1)
    {
        perror("fork child2");
        goto cleanup;
    }

    fprintf(
        stderr,
        "Enter lines. Finish input with Ctrl+D.\n");

    ssize_t length;

    while ((length = getline(
                &line,
                &allocated,
                stdin)) != -1)
    {
        if (check_children(children) == -1)
        {
            perror("check children");
            goto cleanup;
        }

        size_t bytes = (size_t)length + 1;

        uintmax_t total =
            (uintmax_t)offset + (uintmax_t)bytes;

        if (total > (uintmax_t)INT64_MAX)
        {
            fprintf(stderr, "Input line is too large\n");
            goto cleanup;
        }

        if (ftruncate(fd, (off_t)total) == -1)
        {
            perror("ftruncate text");
            goto cleanup;
        }

        char *text = mmap(
            NULL,
            bytes,
            PROT_READ | PROT_WRITE,
            MAP_SHARED,
            fd,
            offset);

        if (text == MAP_FAILED)
        {
            perror("mmap input");
            goto cleanup;
        }

        memcpy(text, line, bytes);
        shared->length = (size_t)length;

        if (munmap(text, bytes) == -1)
        {
            perror("munmap input");
            goto cleanup;
        }

        if (sem_post(&shared->run[0]) == -1)
        {
            perror("sem_post child1");
            goto cleanup;
        }

        if (wait_done(
                &shared->done[0],
                children) == -1)
        {
            perror("wait child1");
            goto cleanup;
        }

        if (sem_post(&shared->run[1]) == -1)
        {
            perror("sem_post child2");
            goto cleanup;
        }

        if (wait_done(
                &shared->done[1],
                children) == -1)
        {
            perror("wait child2");
            goto cleanup;
        }

        size_t output_length = shared->length;

        text = mmap(
            NULL,
            output_length + 1,
            PROT_READ,
            MAP_SHARED,
            fd,
            offset);

        if (text == MAP_FAILED)
        {
            perror("mmap output");
            goto cleanup;
        }

        int output_error = 0;

        if (fwrite(
                text,
                1,
                output_length,
                stdout) != output_length)
        {
            output_error = 1;
        }

        if (munmap(text, output_length + 1) == -1)
        {
            perror("munmap output");
            goto cleanup;
        }

        if (output_error || fflush(stdout) == EOF)
        {
            fprintf(stderr, "Output error\n");
            goto cleanup;
        }
    }

    if (ferror(stdin))
    {
        fprintf(stderr, "Input error\n");
        goto cleanup;
    }

    shared->stop = 1;

    for (int i = 0; i < 2; ++i)
    {
        if (sem_post(&shared->run[i]) == -1)
        {
            perror("sem_post stop");
            goto cleanup;
        }
    }

    for (int i = 0; i < 2; ++i)
    {
        if (join_child(&children[i]) == -1)
        {
            perror("waitpid");
            goto cleanup;
        }
    }

    result = EXIT_SUCCESS;

cleanup:
    for (int i = 0; i < 2; ++i)
    {
        if (children[i] > 0)
        {
            if (kill(children[i], SIGTERM) == -1 &&
                errno != ESRCH)
            {
                perror("kill");
            }
        }
    }

    for (int i = 0; i < 2; ++i)
    {
        if (children[i] > 0)
        {
            if (join_child(&children[i]) == -1)
            {
                result = EXIT_FAILURE;
            }
        }
    }

    free(line);

    for (size_t i = 0; i < initialized; ++i)
    {
        if (sem_destroy(semaphores[i]) == -1)
        {
            perror("sem_destroy");
            result = EXIT_FAILURE;
        }
    }

    if (shared != MAP_FAILED)
    {
        if (munmap(shared, sizeof(Shared)) == -1)
        {
            perror("munmap header");
            result = EXIT_FAILURE;
        }
    }

    if (fd != -1)
    {
        if (close(fd) == -1)
        {
            perror("close");
            result = EXIT_FAILURE;
        }
    }

    if (file_created)
    {
        if (unlink(path) == -1)
        {
            perror("unlink");
            result = EXIT_FAILURE;
        }
    }

    return result;
}