#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct
{
    const unsigned char *remainder;
    size_t len;
} ByteIter;

// Initialize Iterator
ByteIter new_iter(const unsigned char *data, size_t len)
{
    ByteIter iter = {.remainder = data, .len = len};
    return iter;
}

// Emulating Rust's Option<&u8>
// In C, returning NULL represents None
const unsigned char *next(ByteIter *self)
{
    if (self->len == 0)
    {
        return NULL;
    }
    const unsigned char *byte = &self->remainder[0];
    self->remainder++;
    self->len--;
    return byte;
}

int main()
{
    size_t data_size = 100000000; // 100 million bytes
    unsigned char *data = malloc(data_size);
    if (!data)
        return 1;

    for (size_t i = 0; i < data_size; i++)
    {
        data[i] = 1;
    }

    ByteIter iter = new_iter(data, data_size);
    unsigned long long sum = 0;

    printf("C: Calculation started...\n");
    clock_t start = clock();

    while (1)
    {
        const unsigned char *b = next(&iter);
        if (!b)
            break;
        sum += *b;
    }

    clock_t end = clock();
    double cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;

    printf("C Sum: %llu\n", sum);
    printf("C Time elapsed: %.3f seconds\n", cpu_time_used);

    free(data);
    return 0;
}
/*
cd examples\lifetime\semantics_a
gcc test.c -O3 -march=native -o test
./test

C: Calculation started...
C Sum: 100000000
C Time elapsed: 0.017 seconds
*/