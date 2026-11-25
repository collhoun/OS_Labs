#include <math.h>

#include "contract.h"
#include "translation.h"

float Derivative(float A, float deltaX)
{
    if (!isfinite(A) || !isfinite(deltaX) || deltaX == 0.0f) {
        return NAN;
    }

    double point = A;
    double step = deltaX;
    return (float)((cos(point + step) - cos(point - step)) / (2.0 * step));
}

char *translation(long x)
{
    return translate_to_base(x, 3);
}