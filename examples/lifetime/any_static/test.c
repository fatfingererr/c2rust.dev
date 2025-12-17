#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct
{
    void *data;
    int tag; // 0 for int
} Any;

int main()
{
    int count = 1000000;
    Any *list = malloc(sizeof(Any) * count);
    for (int i = 0; i < count; i++)
    {
        int *val = malloc(sizeof(int));
        *val = i;
        list[i].data = val;
        list[i].tag = 0;
    }

    clock_t start = clock();
    long long sum = 0;
    for (int i = 0; i < count; i++)
    {
        if (list[i].tag == 0)
        { // 模擬 downcast 檢查
            sum += *(int *)list[i].data;
        }
    }
    double time_taken = (double)(clock() - start) / CLOCKS_PER_SEC;
    printf("C Sum: %lld, Time: %f s\n", sum, time_taken);

    // 清理記憶體...
    return 0;
}
/*
cd examples\lifetime\any_static
gcc test.c -O3 -march=native -o test
./test

C Sum: 499999500000, Time: 0.001000 s
*/