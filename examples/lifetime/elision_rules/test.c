#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

// C 語言沒有 slice 結構，我們模擬回傳一個指標
// 這裡沒有任何生命週期檢查，完全依賴工程師不回傳錯誤指標
// 為了公平，我們只模擬計算偏移量的開銷
const char *get_middle(const char *s, size_t len)
{
    return s + (len / 4);
}

int main()
{
    size_t len = 1000;
    char *s = (char *)malloc(len + 1);
    memset(s, 'a', len);
    s[len] = '\0';

    volatile size_t dummy = 0; // volatile 防止優化
    clock_t start = clock();

    for (int i = 0; i < 1000000000; i++)
    {
        const char *sub = get_middle(s, len);
        // C 語言這裡要取得長度比較麻煩，為了模擬 Rust slice 的開銷，
        // 我們假裝做了一個簡單運算
        dummy += (len / 4 * 3) - (len / 4);
    }

    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;

    printf("C result: %zu, Time: %f s\n", dummy, time_spent);
    free(s);
    return 0;
}
/*
cd examples/lifetime/elision_rules
gcc test.c -O3 -march=native -o test
./test

C result: 500000000000, Time: 0.257000 s
*/