//
// Created by Clyde Stubbs on 29/11/18.
//

#ifndef GDL90_TESTS_H
#define GDL90_TESTS_H

extern bool x_assertIntEquals(int expected, int actual, char * file, int line);
extern bool x_assertFloatEquals(float expected, float actual, float tolerance, char *file, int line);
extern void x_fail(char * file, int line, char * message, ...);

#define assertFloatEquals(a, b, tolerance) x_assertFloatEquals((a), (b), (tolerance), __FILE__, __LINE__)
#define assertIntEquals(a, b) x_assertIntEquals((a), (b), __FILE__, __LINE__)
#define assertFalse(b) x_assertIntEquals(0, (b), __FILE__, __LINE__)
#define assertTrue(b) x_assertIntEquals(0, !(b), __FILE__, __LINE__)
#define fail(msg, ...) x_fail(__FILE__, __LINE__, msg, __VA_ARGS__)

#endif //GDL90_TESTS_H
