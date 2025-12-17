+++
title = "Any 與 'static"
weight = 2
sort_by = "weight"

[extra]
+++

在 Rust 中，如果想轉換成動態型別，會用到 `std::any::Any`，而 `Any` 的定義是：

```rust
pub trait Any: 'static { ... }
```

為什麼是 `'static` 的一個主因，就是 _型別標示 (TypeId) 的唯一性與生命週期無關_

Rust 的編譯器為每個類型生成一個唯一的 TypeId，然而 TypeId 不包含生命周期資訊

如果 Rust 允許 `Any` 處理帶有 `非 static` 生命周期的類型

你可能會把一個生命周期較短的引用**偽裝** 成一個生命周期較長的類型

因此，`Any` 限制 `T` 必須滿足 `'static`（意即該類型不包含任何非永久的引用），以確保運行時的類型轉換是絕對安全的

#### 偽裝成「長生命週期」

首先先建立一個容器，準備存放「任何類型」的資料，這個變數活得比較久

```rust
use std::any::Any;

fn main() {
    let mut storage: Option<Box<dyn Any>> = None;
    {
        // ...
    }
}
```

接著建立一個短命的局部變數，並取得它的引用 `&str`

這個引用的生命週期 `'a` 只在大括號內有效

```rust
// ...

{
    let short_lived = String::from("我是短命的字串");
    let reference = &short_lived;

    // ...
}
```

最關鍵的操作如下，我們把這個短命的引用轉成 `Any`，也就是假設 `Any` 允許非 `'static`，則這行會成功

並將這個動態物件存入外部的容器 `storage`

```rust
// ...
{
    // ...

    let dynamic_val: Box<dyn Any> = Box::new(reference);
    storage = Some(dynamic_val);

    // ...
}
```

接著大括號結束，`short_lived` 在這裡被 Drop，記憶體被回收

此時 `storage` 裡面還存著 `dynamic_val`，而裡面還存著指向 `short_lived` 的指標

接著我們嘗試從外部容器拿回資料

```rust
// ...
{
    // ...
}

if let Some(any_val) = storage {
        
    // ...
}
```
透過 `downcast_ref` 將 `Any` 轉回原本的 `&str`，因為 TypeId 只紀錄型別是 `&str`，不知道它的生命週期已經結束了

```rust
// ...
{
    // ...
}

if let Some(any_val) = storage {
        
    // ...
    if let Some(recovered_str) = any_val.downcast_ref::<&str>() {

        //...
    }
}
```

最後錯誤發生的位置就會在裡面使用 `recovered_str`：

```rust
// ...
if let Some(recovered_str) = any_val.downcast_ref::<&str>() {
        
    // 這裡的 
    println!("讀取到的內容：{}", recovered_str); 
}
```

因為 `recovered_str` 指向的記憶體已經被釋放或是被別的資料蓋掉了

當然，實際上這不會發生，編譯時就會失敗了:

```ps1
cd examples/lifetime/any_static
cargo run --bin bad_any --release
```

```md
error[E0597]: `short_lived` does not live long enough
  --> bad_any.rs:14:25
   |
10 |         let short_lived = String::from("我是短命的字串");
   |             ----------- binding `short_lived` declared here
...
14 |         let reference = &short_lived;
   |                         ^^^^^^^^^^^^ borrowed value does not live long enough
...
18 |         let dynamic_val: Box<dyn Any> = Box::new(reference);
   |                                         ------------------- coercion requires that `short_lived` is borrowed for `'static`
...
22 |     } // 6. <--- 這裡是大括號結束點！
   |     - `short_lived` dropped here while still borrowed
```

#### 效能測試

將 1,000,000 個整數放入「動態類型」容器中，然後遍歷並進行 **型別檢查（Downcast）** 後累加

```rust
use std::any::Any;

fn main() {
    let mut vec: Vec<Box<dyn Any>> = Vec::new();
    for i in 0..1_000_000 {
        vec.push(Box::new(i as i32));
    }

    let start = std::time::Instant::now();
    let mut sum = 0i64;
    for item in vec {
        if let Some(val) = item.downcast_ref::<i32>() {
            sum += *val as i64;
        }
    }
    println!("Rust Sum: {}, Time: {:?}", sum, start.elapsed());
}
/*
cd examples\lifetime\any_static
cargo run --bin test --release

Rust Sum: 499999500000, Time: 15.3325ms
*/

```

其中 Rust `Box<dyn Any>` 涉及到了兩次間接定址

第一次是尋找 `Box` 指向的記憶體，第二次是透過虛擬表 (vtable) 來處理 `dyn` 相關的操作

```c
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
```

C 只是做一個簡單的整數比對 `if (list[i].tag == 0)`

這在 CPU 層面非常快，甚至可以被分支預測器（Branch Predictor）優化得幾乎沒有成本

而 Rust 呼叫 `downcast_ref::<i32>()` 內部會進行 TypeId 的比對

TypeId 實際上是一個 64 位元或 128 位元的 Hash，比對雜湊值的開銷比比對一個簡單的 int 要高

另外 C 的指標轉換 `(int *)list[i].data` 在編譯後幾乎不產生額外指令

而 Rust 的 `downcast` 是一個函式呼叫，雖然會被 inline，但內部的邏輯判斷仍比 C 複雜

然而如果在 C 中不小心把 tag 設為 0 `int`，但其實裡面存的是 `float`

C 會毫無怨言地執行，然後給你一個無意義的亂數結果，甚至導致崩潰

如果在 Rust 中想追求類似 C 的效能，但又想要安全性，通常得改用 `Enum` 而不是 `Box<dyn Any>`

```rust
// 底層就是 Tagged Union
enum MyAny {
    Int(i32),
    Float(f32),
}
```