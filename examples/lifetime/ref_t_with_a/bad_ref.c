#include <stdio.h>

// 如 Rust 的 struct Ref<'a, T: 'a>(&'a T);
typedef struct
{
    int *data_ptr;
} Ref;

// 如 Rust 的 fn t_bound<'a, T: 'a>(t: T)
void t_bound(Ref r)
{
    // 如果 r.data_ptr 指向的記憶體已經被釋放，這裡就會爆炸
    printf("%d\n", *(r.data_ptr));
}

Ref create_bad_ref()
{
    int local_val = 100;
    Ref r;
    r.data_ptr = &local_val;
    return r; // 返回了指向區域變數的結構體
}

int main()
{
    Ref my_ref = create_bad_ref();
    // 在 Rust 中，編譯器會說 create_bad_ref 的返回值不滿足 T: 'a
    // 因為 local_val 的生命週期在函式結束時就沒了。

    t_bound(my_ref); // Undefined Behavior (可能印出亂碼或崩潰)
}
/*
cd examples\lifetime\ref_t_with_a
gcc bad_ref.c -O3 -march=native -o bad_ref
./bad_ref

0 (不等於 100，因為 local_val 已經不在了)
*/