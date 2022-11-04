//
// Created by Clyde Stubbs on 29/11/18.
//

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include "tests.h"


bool x_assertIntEquals(int expected, int actual, char *file, int line) {
    if (expected != actual) {
        fprintf(stderr, "%s: %d: Expected %d but got %d\n", file, line, expected, actual);
        return false;
    }
    return true;
}

bool x_assertFloatEquals(float expected, float actual, float tolerance, char *file, int line) {
    if (expected < actual - tolerance || expected > actual + tolerance) {
        fprintf(stderr, "%s: %d: Expected %f but got %f\n", file, line, expected, actual);
        return false;
    }
    return true;
}

void x_fail(char *file, int line, char * message, ...) {
    va_list al;
    va_start(al, message);
    char buff[1024];
    vsprintf(buff, message, al);
    fprintf(stderr, "%s: %d: Failed: %s\n", file, line, buff);
    exit(1);
}

