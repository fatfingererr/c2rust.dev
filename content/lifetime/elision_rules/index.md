+++
title = "<'a> 省略規則"
weight = 1

[extra]
+++

由於生命週期使用上的模式能被歸納總結，因此 Rust 有所謂 **生命週期省略規則 (Lifetime Elision Rules)**

1. **規則 1** 每個輸入參數的引用都會有一個獨立的生命週期
2. **規則 2** 如果有且僅有一個輸入生命週期，那麼它會被賦予給所有的輸出引用
3. **規則 3** 如果有多個輸入，但其中一個是 `&self` 或 `&mut self`（方法），那麼 self 的生命週期會賦予給所有輸出引用

例如以下 2 個例子：

```rust
fn trim(s: &str) -> &str;

// 編譯器看到的 (展開版 - 符合規則 2)
// 只有一個輸入，所以輸出一定跟輸入活得一樣久
fn trim<'a>(s: &'a str) -> &'a str;
```

```rust
fn compare(&self, s: &str) -> &str;

// 編譯器看到的 (展開版 - 符合規則 3)
// 這是 struct 的方法，通常回傳值都跟 self 有關
fn compare<'a, 'b>(&'a self, s: &'b str) -> &'a str;
```

只要你使用了引用 (Reference)，本質上就是泛型 (Generic)，且充滿了生命週期標註

只是編譯器很貼心地幫你隱藏了

C 語言沒有「生命週期」檢查，也沒有 **借用檢查器（Borrow Checker）**

在 C 語言中，這一切都依賴工程師的紀律

例如在 Rust 中，下面的程式碼會被編譯器直接攔截：

```rust
// Rust: 錯誤！無法判斷回傳值的生命週期
fn get_str() -> &str {
    let s = String::from("hello");
    &s // 錯誤：s 在這裡就會被銷毀，不能回傳它的引用
}
```

但在 C 中，你可以寫出這個 Bug：

```c
// C: 編譯器可能只會給個警告，甚至什麼都不說
char* get_str() {
    char s[] = "hello";
    return s; // 危險！回傳了指向 stack 記憶體的指標
} 
// 函數結束後，s 的記憶體被釋放，回傳的指標指向垃圾數據

/*
cd examples\lifetime\elision_rules
gcc bad_ref.c -O3 -march=native -o bad_ref
./bad_ref

bad_ref.c: In function 'get_str':
bad_ref.c:4:12: warning: function returns address of local variable [-Wreturn-local-addr]
    4 |     return s;
      |            ^
*/
```

當 Rust 編譯成 Machine Code 時，那些 `<'a>` 標籤全部都會消失

Rust 的 trim 函數在執行時，與 C 的 trim 函數做的事情一樣

這意味著你獲得了安全性，卻不需要付出效能代價

#### 效能測試

```rust
use std::time::Instant;

// 這裡有生命週期省略，實際上是 fn get_middle<'a>(s: &'a str) -> &'a str
// 編譯器會進行嚴格檢查，確保回傳的 slice 不會活得比 s 久
#[inline(never)] // 禁止 inline 以模擬真實函數呼叫開銷
fn get_middle(s: &str) -> &str {
    let len = s.len();
    &s[len / 4..(len / 4) * 3]
}

fn main() {
    let s = "a".repeat(1000); // 建立一個長字串
    let mut dummy: usize = 0; // 防止編譯器優化掉迴圈

    let start = Instant::now();
    for _ in 0..1_000_000_000 {
        let sub = get_middle(&s);
        dummy = dummy.wrapping_add(sub.len());
    }
    let duration = start.elapsed();

    println!("Rust result: {}, Time: {:?}", dummy, duration);
}
/*
cd examples/lifetime/elision_rules
cargo run --release

Rust result: 500000000000, Time: 1.2555444s
*/
```

這邊範例中 Rust 比較慢的原因是，他比 C 多做了 Bounds Checking

其中 `&s[len / 4..(len / 4) * 3]` 中使用到 `[start..end]` 語法

這時候編譯器會插入一段隱藏指令:

1. 檢查 start <= end？
2. 檢查 end <= s.len()？
3. 如果檢查失敗，程式必須 Panic 以保證安全

而在 C 中 `return s + (len / 4)` 只是簡單的指標加法

```c
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
```

如果要對準，Rust 可以考慮寫成：

```rust
fn get_middle(s: &str) -> &str {
    let len = s.len();
    let start = len / 4;
    let end = len / 4 * 3;
    
    // 使用 unsafe 繞過邊界檢查，這才對應 C 語言的行為
    unsafe {
        s.get_unchecked(start..end)
    }
}
```

#### 胖指標 vs. 瘦指標

在 C 語言中，字串只是一個 **地址 Address**

1. `const char*` 是一個 8 bytes 的指標
2. 執行 `get_middle` 時，CPU 只需要回傳一個 64 位元的整數

但在 Rust 中，&str 是一個 Slice（切片），它是一個 **胖指標 (Fat Pointer)**

它包含：地址 (8 bytes) + 長度 (8 bytes) = 16 bytes

每次執行 **get_middle** ，Rust 必須同時計算並回傳「起始位置」與「長度」

同時 `sub.len()`：

```rust
for _ in 0..1_000_000_000 {
    let sub = get_middle(black_box(&s)); 
    dummy = dummy.wrapping_add(sub.len()); // <--- 這裡！
}
```

在 Rust 中是直接讀取胖指標裡的長度欄位

但在 10 億次 的疊加下，CPU 搬運 16 bytes (Rust) 與搬運 8 bytes (C) 的記憶體頻寬壓力是不一樣的

C 語言的指標加法非常容易被 CPU 的流水線（Pipeline）優化

而 Rust 的 Slice 建構涉及兩個暫存器的操作

在極致的微基準測試（Micro-benchmark）中，這點微小的差異被放大了

如果在 Rust 裡改用 `*const u8` (裸指標) 模擬 C 的行為，速度會完全追平 C