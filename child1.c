#include <stdio.h>
#include <ctype.h>

int main()
{
    int c; // не char потому что EOF = -1
    while ((c = getchar()) != EOF)
    {
        putchar(tolower(c));
        if (c == '\n')
        {
            fflush(stdout); // кидаем строку в stdout
        }
    }

    return 0;
}