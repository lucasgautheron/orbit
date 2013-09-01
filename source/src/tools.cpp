// tools.cpp
// different tools
#include "project.h"

// debug message
void dbgoutf(const char *format, ...)
{
    defvformatstring(d, format, format);
    printf("%s\n", d);
}

// current time in ms (to be used for diff only)
#if defined(WIN32)
msec_t time_ms(void)
{
    return timeGetTime();
}
#else
msec_t time_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (msec_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}
#endif

float human_readable_size(int bytes, char &mul)
{
    const char* prefixes = "KMGT";
    float current = float(bytes);
    char p = 0;
    while(current > 1000 && (p = *prefixes++)) current /= 1000.0f;
    mul = p;
    return current;
}