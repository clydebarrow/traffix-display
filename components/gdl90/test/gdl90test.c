//
// Created by Clyde Stubbs on 3/11/2022.
//

#include <printf.h>
#include <riemann.h>
#include <stdbool.h>
#include <stdlib.h>
#include "tests.h"

static bool testGreatCircleDistance() {
    bool passed = true;
    float distance = greatCircleDistance(-28.359791f, 152.078140f, -28.359791f, 152.078140f);
    passed &= assertFloatEquals(0, distance, 0.1f);
    distance = greatCircleDistance(-28.f, 152.f, -28.0001f, 152.f);
    passed &= assertFloatEquals(11.1f, distance, .1f);
    distance = greatCircleDistance(-28.f, 152.f, -28.01f, 152.f);
    passed &= assertFloatEquals(1111.f, distance, 1.f);
    distance = greatCircleDistance(51.15f, 1.33f, 50.97f, 1.85f);
    passed &= assertFloatEquals(41488, distance, 1.0f);
    distance = greatCircleDistance(-33, -71.6f, 31.4f, 121.8f);
    passed &= assertFloatEquals(18742600, distance, 100.0f);
    distance = greatCircleDistance(-32.f, 120.f, 1.73f, -100.0f);
    passed &= assertFloatEquals(14645366.f, distance, 100.0f);
    return passed;
}

static struct {
    const char *name;

    bool (*func)(void);
} testlist[] = {
        {"Great Circles", testGreatCircleDistance}
};

int main() {
    int passed = 0;
    int idx;
    for (idx = 0; idx != sizeof(testlist) / sizeof(testlist[0]); idx++) {
        if (testlist[idx].func()) {
            passed++;
            fprintf(stderr, "\033[32mPassed\033[0m: %s\n", testlist[idx].name);
        } else {
            fprintf(stderr, "\033[31mFailed\033[0m: %s\n", testlist[idx].name);
        }
    }
    if (passed == idx) {
        fprintf(stderr, "\033[32mAll %d tests passed\033[0m\n", idx);
        exit(0);
    }
    fprintf(stderr, "\033[32mPassed: %d\n\033[31mFailed: %d\033[0m\n", passed, idx-passed);
    exit(1);
}