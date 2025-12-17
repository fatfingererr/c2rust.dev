+++
title = "輸入輸出綁定 &'b mut self"
weight = 2
sort_by = "weight"

[extra]
+++

在 Rust 中 Borrow Check 只保證記憶體安全，不保證執行邏輯符合需求

例如下面這個只能呼叫一次的 Iterator，就是把 return 的生命週期與 `&mut self` 綁一起

```rust
struct ByteIter<'a> {
    remainder: &'a [u8],
}

impl<'a> ByteIter<'a> {
    // 錯誤標註：將回傳值與 self 的 mutable borrow 綁定
    fn next_restrictive<'b>(&'b mut self) -> Option<&'b u8> {
        if self.remainder.is_empty() {
            None
        } else {
            let byte = &self.remainder[0];
            self.remainder = &self.remainder[1..];
            Some(byte)
        }
    }
}
```

其中 `&'b mut self -> Option<&'b u8>` ：

- `&'b mut self` 這叫做我會修改借用這個 struct，生命週期是 `'b`
- 回傳 `&'b u8` 回傳的生命週期也得完全等於 `'b`, `Option<...>` 容納 `None`

所以：

```rust
fn main() {
    let mut bytes = ByteIter { remainder: b"123" };
    
    let byte_1 = bytes.next_restrictive(); // 第一次借用：bytes 被標記為「正在被可變借用中」
    
    // ERROR 編譯錯誤！
    // 因為 byte_1 還活著，它要求 bytes 必須維持在「被 mutable 借用」的狀態。
    // 你不能在同一時間進行第二次可變借用
    let byte_2 = bytes.next_restrictive(); 
    
    println!("{:?}, {:?}", byte_1, byte_2);
}
/*
cd examples\lifetime\semantics_a
cargo run --bin iter --release

error[E0499]: cannot borrow `bytes` as mutable more than once at a time
*/
```
而正確寫法應該是： `fn next(&mut self) -> Option<&'remainder u8>`

在這個狀態下，回傳了 `byte_1` 之後，借用就解除了，`bytes` 就可以再被借用

#### 效能測試

```c
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
```

```rust
use std::time::Instant;

struct ByteIter<'remainder> {
    remainder: &'remainder [u8],
}

impl<'remainder> ByteIter<'remainder> {
    fn new(remainder: &'remainder [u8]) -> Self {
        Self { remainder }
    }

    // Correct lifetime: bound to the data source 'remainder
    fn next(&mut self) -> Option<&'remainder u8> {
        if self.remainder.is_empty() {
            None
        } else {
            let byte = &self.remainder[0];
            self.remainder = &self.remainder[1..];
            Some(byte)
        }
    }
}

fn main() {
    // Prepare 100 million bytes (approx. 100MB)
    let data = vec![1u8; 100_000_000];
    let mut iter = ByteIter::new(&data);
    let mut sum: u64 = 0;

    println!("Rust: Calculation started...");
    let start = Instant::now();

    // The Option<T> is optimized away by the compiler into a null pointer check
    while let Some(b) = iter.next() {
        sum += *b as u64;
    }

    let duration = start.elapsed();
    println!("Rust Sum: {}", sum);
    println!("Rust Time elapsed: {:.3?}", duration);
}
/*
cd examples/lifetime/semantics_a
cargo run --bin test --release

Rust: Calculation started...
Rust Sum: 100000000
Rust Time elapsed: 14.645ms
*/
```

效能幾乎一樣