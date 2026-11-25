#ifndef CONTRACT_H
#define CONTRACT_H

float Derivative(float A, float deltaX);

/* The caller frees the result. NULL means allocation failed. */
char *translation(long x);

#endif