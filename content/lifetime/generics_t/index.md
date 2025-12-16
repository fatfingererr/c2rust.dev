+++
title = "所有型別的超集合 T"
weight = 1

[extra]
+++

在 Rust 裡，`T` 不是「只有 owned type」，而是「**所有型別的超集合**」，包含 `&T`、`&mut T`，甚至 `&&&T`

這個設計是 **零成本抽象（zero-cost abstraction）** 的核心之一

#### 初學者直覺的錯誤理解

很多人在剛學 Rust 泛型時，腦中會自然形成這樣的分類：

| 類別     | `T`      | `&T`       | `&mut T`   |
|----------|----------|------------|------------|
| 直覺想像 | 擁有資料 | 不可變借用 | 可變借用   |
| 範例     | `i32`    | `&i32`     | `&mut i32` |

於是會得出一個 **看起來很合理但完全錯誤** 的結論：

- `T` = 所有 owned types  
- `&T` = 所有 shared borrow  
- `&mut T` = 所有 mutable borrow  
- 三者是互斥的集合  

這種模型在 **C / C++ 的心智模型裡非常自然**，但在 Rust 裡是錯的

#### Rust 泛型的真實模型：`T` 是「超集合」

在 Rust 裡，泛型的實際意義是 **代表「任何可能的型別」，不管它是不是 reference。**

也就是：

| 泛型型別 | 實際包含的型別（部分）                                   |
|----------|----------------------------------------------------------|
| `T`      | `i32`, `&i32`, `&mut i32`, `&&i32`, `&mut &mut i32`, ... |
| `&T`     | `&i32`, `&&i32`, `&&mut i32`, ...                        |
| `&mut T` | `&mut i32`, `&mut &i32`, `&mut &mut i32`, ...            |

- `T` **包含** `&T` 與 `&mut T`
- `&T` 與 `&mut T` **彼此不重疊**
- 這三者都是 **無限集合**

#### 編譯器錯誤 (impl overlap)

看看這個例子：

```rust
trait Trait {}

impl<T> Trait for T {}
impl<T> Trait for &T {}     // ❌
impl<T> Trait for &mut T {} // ❌
```

編譯器錯誤不是因為 Rust 小氣，而是因為：

- `impl<T> Trait for T` **已經涵蓋了所有型別**
- `&T`、`&mut T` 本來就屬於 `T`
- 這會造成 **impl overlap**

但這樣寫就完全沒問題：

```rust
trait Trait {}

impl<T> Trait for &T {}     // ✅
impl<T> Trait for &mut T {} // ✅
```

#### 效能角度思考：為什麼 `T` 要包含 reference

如果 Rust 的 `T` 只代表 owned types，那麼：

- 每個函數都要寫三個版本  
- 或引入 runtime dispatch  

Rust 選擇的是 **用型別系統在編譯期解決，而不是 runtime 判斷**

也就是 Rust 把 aliasing、mutability、lifetime **全部放進型別系統**

#### 泛型測試

要對泛型進行測試，就得對編譯器整體性的綁住手腳，避免編譯器最佳化影響我們的測試

也就是我們要確定，Rust 的泛型與借用設計，在實際會用到的情形之下，不會造成 runtime 成本

在這之下才能測試不同寫法的差，例如 `T`, `&T`, `&mut T`

考慮一個 `process<T>` 如下：

```rust
fn process<T>(value: T) -> u64 {
    let v = std::hint::black_box(value);
    let addr = (&v as *const T as usize) as u64;
    std::hint::black_box(addr)
}
```

`std::hint::black_box` 是專門用來做效能分析的工具，告訴編譯器不能理解、推導與提前最佳化輸入值

也就是告訴編譯器說，(1) `value` 不能假設是常數 (2) 不能假設沒人用，`black_box` 裡面可能會用，所以編譯器會 **保存**

但這個保存可能放在 CPU 暫存器 (register) 或是放在棧 (stack)，也就是這個值不能被最佳化掉

而第二步 `&v` 告訴編譯器我需要變數的地址，並且我要轉成一個整數來用，因此 `v` 不能只放在暫存器上，還得放到 Stack 上

第三步 `black_box(addr)` 也是說，拿出來的地址也不能亂丟，不能被合併摺疊與提前計算

其中轉 `u64` 的原因，是完全避免編譯器可能對 `usize` 的特殊推理，把 `usize` 當成記憶體位址

因此這整個流程會操作到：

- 呼叫一個 function 
- 強迫建立一個 local variable
- 強迫他有 address
- 強迫他不能被最佳化
- 再做一次黑盒運算

在此之下去改變泛型引用，才能合理的比較之間的差別與成本

#### Rust 範例

```rust
use std::time::Instant;

#[inline(always)]
fn process<T>(value: T) -> u64 {
    let v = std::hint::black_box(value);
    let addr = (&v as *const T as usize) as u64;
    std::hint::black_box(addr)
}

#[inline(always)]
fn process_mut<T>(value: &mut T) -> u64 {
    // 對 &mut 我們額外做一個「可觀察的寫入」：交換一次位元（針對 u64）
    // 但因為是泛型 T，我們不能直接改；所以示範用 u64 專用版本比較更公平
    let addr = (value as *mut T as usize) as u64;
    std::hint::black_box(addr)
}

#[inline(always)]
fn process_u64_mut(x: &mut u64) -> u64 {
    // 真的寫一次，避免「&mut 其實沒用到」而被刪掉
    *x = x.wrapping_mul(6364136223846793005u64).wrapping_add(1);
    std::hint::black_box(*x)
}

fn bench_owned(n: usize) -> u64 {
    let mut acc = 0u64;
    for i in 0..n {
        // owned: T = u64
        acc = acc.wrapping_add(process(i as u64));
    }
    acc
}

fn bench_ref(n: usize) -> u64 {
    let mut acc = 0u64;
    let x = 123456789u64;
    for _ in 0..n {
        // shared: T = &u64
        acc = acc.wrapping_add(process(&x));
    }
    acc
}

fn bench_mut(n: usize) -> u64 {
    let mut acc = 0u64;
    let mut x = 123456789u64;
    for _ in 0..n {
        // mutable: 對照 C 的 int* 修改
        acc = acc.wrapping_add(process_u64_mut(&mut x));
    }
    acc
}

fn main() {
    // 迴圈次數：你可以依照機器快慢調整（先從 50_000_000 試）
    let n = 50_000_000usize;

    // warm up（讓 CPU 進入穩態，避免第一次測被冷啟動影響）
    let _ = bench_owned(1_000_000);
    let _ = bench_ref(1_000_000);
    let _ = bench_mut(1_000_000);

    let t0 = Instant::now();
    let a0 = bench_owned(n);
    let d0 = t0.elapsed();

    let t1 = Instant::now();
    let a1 = bench_ref(n);
    let d1 = t1.elapsed();

    let t2 = Instant::now();
    let a2 = bench_mut(n);
    let d2 = t2.elapsed();

    println!("N = {n}");
    println!("owned (T=u64)        : {:?}  acc={}", d0, a0);
    println!("shared (T=&u64)      : {:?}  acc={}", d1, a1);
    println!("mutable (&mut u64)   : {:?}  acc={}", d2, a2);
}

/*
cd examples/lifetime/generics_t
cargo run --release

N = 50000000
owned (T=u64)        : 24.5691ms  acc=12992747882800000000
shared (T=&u64)      : 25.7718ms  acc=12992747882800000000
mutable (&mut u64)   : 40.1917ms  acc=14108795302277110464
*/
```

#### C 的對照範例

在 C 也能實作這三類的效果，也能看出時間落差

```c
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#if defined(__GNUC__) || defined(__clang__)
  #define NOINLINE __attribute__((noinline))
  static inline void black_box_u64(uint64_t x) {
      asm volatile("" : : "r"(x) : "memory");
  }
  static inline void black_box_ptr(const void* p) {
      asm volatile("" : : "r"(p) : "memory");
  }
#else
  #define NOINLINE
  static inline void black_box_u64(uint64_t x) { (void)x; }
  static inline void black_box_ptr(const void* p) { (void)p; }
#endif

static inline uint64_t now_ns(void) {
#if defined(CLOCK_MONOTONIC)
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
#else
    return 0;
#endif
}

// 對照 Rust: owned (傳值)
static inline uint64_t process_u64(uint64_t value) {
    black_box_u64(value);
    // 用位址當作「確實使用過」的觀察點：取 local 的位址
    uint64_t addr = (uint64_t)(uintptr_t)&value;
    black_box_u64(addr);
    return addr;
}

// 對照 Rust: shared borrow (&T) => const pointer
static inline uint64_t process_u64_ref(const uint64_t* value) {
    black_box_ptr(value);
    uint64_t addr = (uint64_t)(uintptr_t)value;
    black_box_u64(addr);
    return addr;
}

// 對照 Rust: mutable borrow (&mut T) => mutable pointer + 寫入
static inline uint64_t process_u64_mut(uint64_t* value) {
    // 真正寫一次，避免被當成沒用
    *value = (*value * 6364136223846793005ull) + 1ull;
    black_box_ptr(value);
    black_box_u64(*value);
    return *value;
}

static uint64_t bench_owned(size_t n) {
    uint64_t acc = 0;
    for (size_t i = 0; i < n; i++) {
        acc += process_u64((uint64_t)i);
    }
    return acc;
}

static uint64_t bench_ref(size_t n) {
    uint64_t acc = 0;
    const uint64_t x = 123456789ull;
    for (size_t i = 0; i < n; i++) {
        acc += process_u64_ref(&x);
    }
    return acc;
}

static uint64_t bench_mut(size_t n) {
    uint64_t acc = 0;
    uint64_t x = 123456789ull;
    for (size_t i = 0; i < n; i++) {
        acc += process_u64_mut(&x);
    }
    return acc;
}

int main(void) {
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

/*
cd examples/lifetime/generics_t
gcc -O3 -march=native -flto -DNDEBUG test.c -o test
./test

N = 50000000
owned (u64)      : 8863900 ns  acc=4272108245200000000
shared (const*)  : 14346100 ns  acc=4272108245600000000
mutable (*mut)   : 43464200 ns  acc=14108795302277110464
*/
```

從結果看：

| 範例    | C        | Rust     | Rust / C 倍數 | 說明                                         |
|---------|----------|----------|---------------|----------------------------------------------|
| owned   | 8.86 ms  | 24.57 ms | **≈ 2.8×**    | C 版本在此情境下被高度最佳化，接近「空迴圈」 |
| shared  | 14.35 ms | 25.77 ms | **≈ 1.8×**    | C shared 仍可被合併，Rust 保留更多實際工作   |
| mutable | 43.46 ms | 40.19 ms | **≈ 0.9×**    | 兩邊皆有真實寫入，效能同一量級、結果最可信   |

- mutable case 中，Rust 與 C 的效能幾乎一致，顯示 Rust 泛型與借用模型本身沒有額外 runtime 成本
- owned / shared 的差距，主要反映編譯器是否能將迴圈最佳化成近乎空操作