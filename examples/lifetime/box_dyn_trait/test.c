#include <stdio.h>
#include <time.h>

typedef struct
{
    int (*speak)(void *);
} VTable;
typedef struct
{
    void *data;
    VTable *vtable;
} DynAnimal;

int cat_speak(void *self) { return 1; }
VTable CAT_VTABLE = {.speak = cat_speak};

int main()
{
    DynAnimal cat = {.data = NULL, .vtable = &CAT_VTABLE};
    clock_t start = clock();
    long long sum = 0;
    for (int i = 0; i < 100000000; i++)
    {
        sum += cat.vtable->speak(cat.data);
    }
    printf("Result: %lld, C Time: %f s\n", sum, (double)(clock() - start) / CLOCKS_PER_SEC);
    return 0;
}
/*
cd examples\lifetime\box_dyn_trait
gcc test.c -O3 -march=native -o test
./test

Result: 100000000, C Time: 0.100000 s
*/