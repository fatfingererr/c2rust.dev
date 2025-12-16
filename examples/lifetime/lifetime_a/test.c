#include <stdio.h>
#include <stdint.h>

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

// MSVC / clang-cl: 防止編譯器跨越 barrier 亂搬運
#if defined(_MSC_VER)
#include <intrin.h>
#pragma intrinsic(_ReadWriteBarrier)
#define COMPILER_BARRIER() _ReadWriteBarrier()
#define NOINLINE __declspec(noinline)
#else
#define COMPILER_BARRIER() __asm__ __volatile__("" ::: "memory")
#define NOINLINE __attribute__((noinline))
#endif

#else
#include <time.h>
static double now_seconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}
#define COMPILER_BARRIER() __asm__ __volatile__("" ::: "memory")
#define NOINLINE __attribute__((noinline))
#endif

NOINLINE uint64_t add(volatile uint64_t *x)
{
    // 讀 volatile，避免被當成常數一路傳播
    return *x + 1;
}

int main(void)
{
    const uint64_t N = 50ULL * 1000 * 1000;

    volatile uint64_t v = 123; // volatile：讓 v 不能被當成編譯期常數
    uint64_t acc = 0;

    COMPILER_BARRIER();
    double start = now_seconds();

    for (uint64_t i = 0; i < N; i++)
    {
        acc += add(&v);
    }

    double end = now_seconds();
    COMPILER_BARRIER();

    double elapsed = end - start;

    printf("acc = %llu\n", (unsigned long long)acc);
    printf("C elapsed: %.3f ms (%.3f us)\n", elapsed * 1e3, elapsed * 1e6);
    return 0;
}
/*
cd examples\lifetime\lifetime_a
gcc test.c -O3 -march=native -o test
./test

acc = 6200000000
C elapsed: 18.713 ms (18712.900 us)
*/