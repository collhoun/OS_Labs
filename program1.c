#include <stddef.h>

#include "common.h"

int main(void)
{
    Functions functions = {Derivative, translation};
    return run_commands(&functions, NULL, NULL);
}