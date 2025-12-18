+++
title = "多型 Box<dyn Trait>"
weight = 2
sort_by = "weight"

[extra]
+++

在 Rust 實現 **多型 (Polygmorphism)** 最常見的方法就是 `Box<dyn Trait>`

事實上當我們使用 `Box<dyn Trait>` 時，Rust 還會做：

1. 根據上下文對 `Trait` 加上 `'static` 或對應的 lifetime，保證在 `std::thread::spawn` 傳遞動態物件不會突然失效
2. `Box<dyn Trait>` 在記憶體佔兩個指標的空間，資料指標和 vtable 指標（指向一個表格，紀錄該 Trait 所有函式位址）

一個例子如下：

```rust
use std::hint::black_box;
use std::time::Instant;

// 定義我們的 Trait
trait Animal {
    fn speak(&self) -> i32;
}

// 實作 Cat
struct Cat;
impl Animal for Cat {
    #[inline(never)] // 為了公平測試，我們強制不讓編譯器直接把函式內聯化
    fn speak(&self) -> i32 {
        1
    }
}

/// 1. 靜態分派 (Static Dispatch)
/// 編譯器會為每一種具體型別 T 生成一份專屬的函式實作。
fn bench_static<T: Animal>(animal: T, iterations: u32) {
    let start = Instant::now();
    let mut sum = 0;
    for _ in 0..iterations {
        // 因為 T 是確定的，這裡沒有 Vtable 查表，直接 Call 函式位址
        sum += black_box(animal.speak());
    }
    println!("Static Dispatch:  Result = {}, Time = {:?}", sum, start.elapsed());
}

/// 2. 動態分派 (Dynamic Dispatch)
/// 使用 Box<dyn Trait>，這就是你之前測試的模式。
fn bench_dynamic(animal: Box<dyn Animal>, iterations: u32) {
    let start = Instant::now();
    let mut sum = 0;
    for _ in 0..iterations {
        // 透過胖指標尋找 Vtable，再跳轉到函式位址
        sum += black_box(animal.speak());
    }
    println!("Dynamic Dispatch: Result = {}, Time = {:?}", sum, start.elapsed());
}

fn main() {
    let iterations = 100_000_000;

    println!("--- Rust Performance Comparison ---");

    // 測試靜態分派
    let cat_static = Cat;
    bench_static(cat_static, iterations);

    // 測試動態分派
    let cat_dynamic: Box<dyn Animal> = Box::new(Cat);
    bench_dynamic(cat_dynamic, iterations);
}
/*
cd examples\lifetime\box_dyn_trait
cargo run --bin test --release

Static Dispatch:  Result = 100000000, Time = 29.2369ms
Dynamic Dispatch: Result = 100000000, Time = 27.1714ms
*/
```

而 C 沒有 `trait` 所以得透過 `struct` 和 Function Pointers 來模擬

1. 定義一個 **Vtable struct**
2. 定義一個 **物件 struct**，包含資料與指向 Vtable 的指標

並且 C 中我們寫的是 `cat.vtable->speak(cat.data)` 需要：

1. 先讀取 `cat.vtable` 的位址
2. 再從該位址讀取 `speak` 函式的位址
3. 最後跳轉

這比 Rust 多了一次 **記憶體解引用 (Dereference)**

```c
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
```

在動態分派的場景下，效能的殺手通常是 **間接跳轉 (Indirect Branch)**

C 的實作完全依賴函式指標，編譯器很難穿透指標去優化

而 Rust 代碼中，因為 animal 是在 main 函式內直接建立且沒有被修改過的

LLVM 發現雖然寫的是 `dyn Animal`，但百分之百確定這就是 `Cat`

於是 LLVM 會嘗試把「查表後跳轉」直接變成「內聯 (Inline)」或者直接呼叫具體函式

這讓 CPU 的分支預測器 (Branch Predictor) 輕鬆許多 