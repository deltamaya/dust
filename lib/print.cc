//
// Created by delta on 17/03/2024.
//
#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif
#include <iostream>
/// putchard - putchar that takes a double and returns 0.
extern "C" DLLEXPORT double putchard(double X) {
    fputc((char)X, stderr);
    return 0;
}

/// printd - printf that takes a double prints it as "%f\n", returning 0.
extern "C" DLLEXPORT double printd(double X) {
    fprintf(stderr, "%lf\n", X);
    return 0;
}

extern "C" DLLEXPORT double scand() {
    double ret;
    scanf("%lf", &ret);
    return ret;
}