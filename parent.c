#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>   // для системных вызовов
#include <sys/wait.h> // waitpid

static void die(const char *what)
{
    // печатаем сообщение об ошибки и завершаем программу
    perror(what);
    exit(EXIT_FAILURE); // передаем макрос о неуспехе -> закрытие потоков ввода вывода и тд
}

pid_t spawn(const char *prog, int in_fd, int out_fd, int *all, int n)
{
    pid_t pid = fork(); // >0 - родитель, 0 - ребенок, -1 - ошибка
    if (pid < 0)
        die("fork");

    if (pid == 0)
    {
        if (dup2(in_fd, STDIN_FILENO) < 0)
            die("dup2 stdin");
        if (dup2(out_fd, STDOUT_FILENO) < 0)
            die("dup2 stdout");

        for (int i = 0; i < n; i++)
        {
            close(all[i]);
        }
        execl(prog, prog, (char *)NULL); // становимся другой программой
        die("execl");                    // сюди придем только в случае ошибки
    }
    return pid;
}

int main(void)
{
    int p1[2], p2[2], p3[2];

    if (pipe(p1) < 0)
        die("pipe1"); // parent -> child1
    if (pipe(p2) < 0)
        die("pipe2"); // child1 -> child2
    if (pipe(p3) < 0)
        die("pipe3"); // child2 -> parent

    int all[6] = {p1[0], p1[1], p2[0], p2[1], p3[0], p3[1]};

    pid_t c1 = spawn("./child1", p1[0], p2[1], all, 6);
    pid_t c2 = spawn("./child2", p2[0], p3[1], all, 6);

    close(p1[0]);
    close(p2[0]);
    close(p2[1]);
    close(p3[1]);

    char *line = NULL;
    size_t capacity = 0;
    ssize_t len = 0;

    while ((len = getline(&line, &capacity, stdin)) != -1)
    {
        if (write(p1[1], line, len) < 0)
        {
            die("write");
        }

        char buffer[4000];
        for (;;)
        {
            ssize_t n = read(p3[0], buffer, sizeof(buffer));
            if (n < 0)
            {
                die("read");
            }
            if (n == 0) // детей уже нет ответа не будет
            {
                break;
            }
            fwrite(buffer, 1, n, stdout);
            if (buffer[n - 1] == '\n')
                break;
        }
    }
    free(line);
    close(p1[1]);
    close(p3[0]);

    if (waitpid(c1, NULL, 0) < 0)
        die("waitpid c1");
    if (waitpid(c2, NULL, 0) < 0)
        die("waitpid c2");
    return 0;
}