#include <stdio.h>
#include <stdlib.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
static double now_seconds(void)
{
    static LARGE_INTEGER freq;
    static int init = 0;
    if (!init)
    {
        QueryPerformanceFrequency(&freq);
        init = 1;
    }
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart / (double)freq.QuadPart;
}

// Windows 的 stdio 鎖
#define LOCK_STDERR() _lock_file(stderr)
#define UNLOCK_STDERR() _unlock_file(stderr)

#else
#include <time.h>
static double now_seconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

// POSIX 的 stdio 鎖
#define LOCK_STDERR() flockfile(stderr)
#define UNLOCK_STDERR() funlockfile(stderr)

#endif

int main(void)
{
    const int N = 100000;

    LOCK_STDERR();

    double start = now_seconds();

    for (int i = 0; i < N; i++)
    {
        fprintf(stderr, "test %d\n", i);
    }

    fflush(stderr);

    double end = now_seconds();

    UNLOCK_STDERR();

    fprintf(stderr,
            "C stderr locked fprintf: %.6f sec\n",
            (end - start));
    return 0;
}