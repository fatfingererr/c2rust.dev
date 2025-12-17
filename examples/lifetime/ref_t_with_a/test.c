#include <stdio.h>
#include <time.h>

typedef struct
{
    int *ptr;
} IntRef;

// 禁止 inline 以公平比較
__attribute__((noinline)) int process_bound(IntRef item)
{
    return *(item.ptr);
}

int main()
{
    int val = 10;
    IntRef wrapper = {&val};
    long long iterations = 1000000000;

    clock_t start = clock();
    long long sum = 0;

    for (long long i = 0; i < iterations; i++)
    {
        // C 語言直接傳遞 struct (by value)，裡面包含指標
        IntRef temp = {&val};
        sum += process_bound(temp);
    }

    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;

    printf("C Result: %lld, Time: %f s\n", sum, time_spent);
    return 0;
}
/*
cd examples\lifetime\ref_t_with_a
gcc test.c -O3 -march=native -o test
./test

C Result: 10000000000, Time: 0.315000 s
*/