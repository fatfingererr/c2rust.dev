+++
title = "生命週期 'a"
weight = 2
sort_by = "weight"

[extra]
+++

例如一個函數：
```rust
fn print_sum(x: &i32, y: &i32) {
    println!("{}", x + y);
}
```
這個 `print_sum` 中涉及到的 `x`, `y` 只在算完就沒了，他們可以有各自的生命週期

所以他其實也等價於以下做了生命週期標記的函數：

```rust
fn print_sum<'a, 'b>(x: &'a i32, y: &'b i32) {
    println!("{}", x + y);
}
```

其中 `x`, `y` 可以有各自的生命週期，顯示完此函數就不用了

但如果是：

```rust
fn max(x: &i32, y: &i32) -> &i32 {
    if x > y { x } else { y }
}
```

這個情況，回傳有可能是 `x` 也可能是 `y`，他們各自有不同的生命週期

所以就要顯性對齊：

```rust
fn max<'a>(x: &'a i32, y: &'a i32) -> &'a i32 {
    if x > y { x } else { y }
}
```

也就是說，這個函數在尋求一段生命周期 `'a` 能讓 `x`, `y` 和回傳值的生命週期一樣

而這必然結果就是生命週期最短的那個，例如以下：

```rust
fn main() {
    let a = 10;            // lifetime = L1
    let result: &i32;

    {
        let b = 20;        // lifetime = L2（比較短）
        result = max(&a, &b);
    }

    println!("{}", result); // 這行能不能合法？
}

// error[E0597]: `b` does not live long enough
```
就會遇到 **懸置引用 (dangling reference)** ，其中 `b` 是生命週期最短的，所以 `result` 也是

結果就是出了括號兩者都死了，`println!` 就沒辦法輸出

不想要這個問題，可以用 4 種方式避免：

- **1. 取值即拷貝**
```rust
fn max_val(x: &i32, y: &i32) -> i32 {
    if x > y { *x } else { *y }
}
```
因為 `i32` 取值即拷貝，就等於直接複製了一份


- **2. 回傳 `Option<&i32>`**
```rust
fn max_choose_x<'a>(x: &'a i32, y: &i32) -> Option<&'a i32> {
    if x > y { Some(x) } else { None }
}
```
直接綁死 `x` 的生命週期，就不受 `y` 影響


- **3. 回傳擁有權 `owned` (適合 `String`, `Vec`)**
```rust
fn max_owned(x: &String, y: &String) -> String {
    if x > y { x.clone() } else { y.clone() }
}
```

不管 lifetime，沒有借用限制，但需要 `clone()` 

- **4. 獲取擁有權 `take` (適合 `String`, `Vec`)**

```rust
fn max_take(x: String, y: String) -> String {
    if x > y { x } else { y }
}
```

沒有 `clone` 但是呼叫端要給出他的所有權

所以回頭看生命週期的標記：

```rust
fn foo<'a>(x: &'a i32) -> &'a i32 {
    x
}
```

你也可以說，這裡的 `'a` 是在強調 **回傳值不能比輸入活的更久**

#### Rust 範例

```rust
use std::time::Instant;
use std::hint::black_box;

#[inline(never)]
fn add(x: &u64) -> u64 {
    *x + 1
}

fn main() {
    const N: u64 = 50_000_000;

    let v: u64 = 123;
    let mut acc: u64 = 0;

    let start = Instant::now();

    for _ in 0..N {
        // black_box 防止編譯器把整個 loop 優化掉
        acc = acc.wrapping_add(add(black_box(&v)));
    }

    let elapsed = start.elapsed();

    println!("acc = {}", acc);
    println!("Rust elapsed: {:.3?}", elapsed);
}

/*
cd examples\lifetime\lifetime_a
cargo run --release

acc = 6200000000
Rust elapsed: 25.655ms
*/
```

#### C 範例

在 C 的版本，使用 `volatile` 強制每次 `v` 累加到 `acc` 中都要真的讀

這樣成本就會在 **迴圈**、**記憶體讀取**、**函式呼叫**、**編譯最佳化**

```c
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
```

C 依然比 Rust 快的原因可能是：

1. Rust 的 `black_box` 比 `volatile` 更重， `volatile` 可能對 LLVM 破壞不大
2. Rust 的 `Instant::now` + `Duration` 也比想像更重




