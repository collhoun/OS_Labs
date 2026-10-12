#include <stdio.h>

int main()
{
    int c;
    int previous_space = 0;

    while ((c = getchar()) != EOF)
    {
        if (c == ' ')
        {
            if (previous_space)
                continue;
            previous_space = 1;
        }
        else
        {
            previous_space = 0;
        }
        putchar(c);
        if (c == '\n')
        {
            fflush(stdout);
        }
    }
    return 0;
}