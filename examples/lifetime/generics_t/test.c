#include <stdint.h>
#include <stdio.h>
#include <time.h>

#if defined(__GNUC__) || defined(__clang__)
#define NOINLINE __attribute__((noinline))
static inline void black_box_u64(uint64_t x)
{
    asm volatile("" : : "r"(x) : "memory");
}
static inline void black_box_ptr(const void *p)
{
    asm volatile("" : : "r"(p) : "memory");
}
#else
#define NOINLINE
static inline void black_box_u64(uint64_t x) { (void)x; }
static inline void black_box_ptr(const void *p) { (void)p; }
#endif

static inline uint64_t now_ns(void)
{
#if defined(CLOCK_MONOTONIC)
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
#else
    return 0;
#endif
}

// 對照 Rust: owned (傳值)
static inline uint64_t process_u64(uint64_t value)
{
    black_box_u64(value);
    // 用位址當作「確實使用過」的觀察點：取 local 的位址
    uint64_t addr = (uint64_t)(uintptr_t)&value;
    black_box_u64(addr);
    return addr;
}

// 對照 Rust: shared borrow (&T) => const pointer
static inline uint64_t process_u64_ref(const uint64_t *value)
{
    black_box_ptr(value);
    uint64_t addr = (uint64_t)(uintptr_t)value;
    black_box_u64(addr);
    return addr;
}

// 對照 Rust: mutable borrow (&mut T) => mutable pointer + 寫入
static inline uint64_t process_u64_mut(uint64_t *value)
{
    // 真正寫一次，避免被當成沒用
    *value = (*value * 6364136223846793005ull) + 1ull;
    black_box_ptr(value);
    black_box_u64(*value);
    return *value;
}

static uint64_t bench_owned(size_t n)
{
    uint64_t acc = 0;
    for (size_t i = 0; i < n; i++)
    {
        acc += process_u64((uint64_t)i);
    }
    return acc;
}

static uint64_t bench_ref(size_t n)
{
    uint64_t acc = 0;
    const uint64_t x = 123456789ull;
    for (size_t i = 0; i < n; i++)
    {
        acc += process_u64_ref(&x);
    }
    return acc;
}

static uint64_t bench_mut(size_t n)
{
    uint64_t acc = 0;
    uint64_t x = 123456789ull;
    for (size_t i = 0; i < n; i++)
    {
        acc += process_u64_mut(&x);
    }
    return acc;
}

int main(void)
{
    const size_t n = 50000000;

    // warm up
    (void)bench_owned(1000000);
    (void)bench_ref(1000000);
    (void)bench_mut(1000000);

    uint64_t t0 = now_ns();
    uint64_t a0 = bench_owned(n);
    uint64_t d0 = now_ns() - t0;

    uint64_t t1 = now_ns();
    uint64_t a1 = bench_ref(n);
    uint64_t d1 = now_ns() - t1;

    uint64_t t2 = now_ns();
    uint64_t a2 = bench_mut(n);
    uint64_t d2 = now_ns() - t2;

    printf("N = %zu\n", n);
    printf("owned (u64)      : %llu ns  acc=%llu\n",
           (unsigned long long)d0, (unsigned long long)a0);
    printf("shared (const*)  : %llu ns  acc=%llu\n",
           (unsigned long long)d1, (unsigned long long)a1);
    printf("mutable (*mut)   : %llu ns  acc=%llu\n",
           (unsigned long long)d2, (unsigned long long)a2);

    return 0;
}