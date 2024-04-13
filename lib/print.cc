//
// Created by delta on 17/03/2024.
//
#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

#include <iostream>
extern "C" {
DLLEXPORT double putchard(double X) {
    fputc((char) X, stderr);
    return 0;
}
DLLEXPORT double printd(double X) {
    fprintf(stderr, "%lf\n", X);
    return 0;
}
DLLEXPORT double prints(const char *X) {
    fprintf(stderr, "%s\n", X);
    return 0;
}
DLLEXPORT double scand() {
    double ret;
    scanf("%lf", &ret);
    return ret;
}
}

