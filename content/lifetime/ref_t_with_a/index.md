+++
title = "型別限制 T: 'a"
weight = 2
sort_by = "weight"

[extra]
+++

初學者很容易把 `T: 'a` 和 `&'a T` 搞錯，不單純只是只要生命週期要比 `a` 久

- `&'a T` : 一個指向 `T` 的借用，這個借用要活得比 `'a` 久，對應 C 中的 `T*`
- `T: 'a` : **這是型別限制 (Type Bound)** 代表 `T` 本身包含的所有內容 (如內部持有的借用) 都必須活得比 `'a` 久

簡言之，`T: 'a` 比 `&'a T` 更多可能性，可以是 (1) 引用 (2) 持有引用的結構 (3) 不含引用的 owned type

#### `T: 'a` 場景

設計一個 `check_lifetime`

```rust
fn check_lifetime<'a, T>(item: T)
where
    T: 'a,
{
    println!("Type T successfully satisfies the T: 'a bound.");
}
```

如果是 **基本型別** 像 `i32` 不包含任何生命週期受限的引用，它們的數據是完全 Owned 的，因此也就滿足最長生命週期

既然 `T: 'static` 成立，那麼對於任何生命週期 `'a`， `T: 'a` 自然也成立，因為 `'static` 永遠大於或等於任何 `'a`

```rust
let t_i32: i32 = 42;
check_lifetime::<'static, i32>(t_i32); // 即使我們要求 T: 'static (最長生命週期)，i32 也符合
```

如果是一個 **引用型別 (&'b SomeType)** 要滿足 `T: 'a`

條件是它本身的生命週期 `'b` 必須大於或等於 `'a`（即 `'b: 'a`）

而下方 `T = &'a str` 中引用的生命週期 `'b` 和約束的生命週期 `'a` 是相同的，因此它當然滿足 `T: 'a`

```rust
let owner_data = 100;                     // 'a 的範圍開始
let ref_to_owner: &i32 = &owner_data;     // 這裡的引用生命週期等同於 'b = 'a
check_lifetime::<'a, &i32>(ref_to_owner); // T = &'a i32，且 'b >= 'a，所以通過
```

如果是不含引用的 owned type，例如 `String`，它不含任何引用，它本身是 owned data，所以總是滿足 `'static`

```rust
let t_string = String::from("Owned data");
check_lifetime::<'a, String>(t_string); 
```

#### 複合型別

例如以下定義：

```rust
// Wrapper 是一個包含引用的結構體
struct Wrapper<'b> {
    data: &'b str, // 內部持有引用
}
```

第一個情況是 `Wrapper<'static>`，其中 `'b` 是最長的 `'static`

```rust
let static_str = "I live forever"; // 這是 'static 資料
let wrapper_static = Wrapper { data: static_str }; 
// T = Wrapper<'static>，'static >= 'a (短暫生命週期) 
check_lifetime::<'a, Wrapper<'static>>(wrapper_static);
```

第二個情況是 `Wrapper<'b_local>`，其中 `b_local` 比 `'a` 更短命

```rust
let shortlive_scope = {                              // 這裡定義了一個短暫作用域
    let local_str = String::from("I only live here"); 
    let local_ref = local_str.as_str();              // 這個引用的生命週期 'b 綁定於這個作用域
    let wrapper_local = Wrapper { data: local_ref }; // 'b_local 比 'a (短暫生命週期) 還要短

    check_lifetime::<'a, Wrapper<'b_local>>(wrapper_local); // FAILED

    local_str // 讓 local_str 存活到 block 結束，防止立即 drop
};
```

當定義了 `struct Wrapper<'b>` 時，這個 struct 的 lifetime 就被內部最短的引用所限制

要讓 `Wrapper<'b>` 滿足 `T: 'a`，必須滿足：`'b` 活得比 `'a` 久

這就是 `T: 'a` 在保護我們不會將生命週期短的 struct 傳入需要長久存活的環境中的關鍵作用

使用 `T: 'a` 的意義在於你可以寫一個泛型，它可以同時接受基本整數如 `i32`、字串引用 `&str`，和一個包含該引用的複雜結構體

並保證它們都滿足同一個 lifetime 的要求

確保你不能將一個內部包含懸空指標的 struct，傳遞給任何要求其生命週期夠長的環境

從而在不犧牲性能的前提下，徹底消除了記憶體不安全的風險

#### C 版本的 `T: 'a`

模擬 `&'a T` 如下：

```c
// Rust 的 fn t_ref<'a>(t: &'a int)
void t_ref(int* t) {
    printf("%d\n", *t); // 假設 t 指向的記憶體是有效的
}
```

模擬 `T: 'a` 則是：

```c
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
```

#### 效能測試

下方 Rust 的實作使用了 `IntRef<'b>` 結構體來封裝引用，並透過 `T: 'a` 泛型約束和 `AsRef` Trait 進行抽象化

Rust 的生命週期檢查、泛型和 Trait（例如 T: 'a）都是編譯時期的抽象

它們在編譯成機器碼後，被 LLVM 徹底優化掉了，最終留下的機器碼與 C 語言手動操作指標的程式碼是等效的

```rust
use std::time::Instant;
// 引入 black_box
use std::hint::black_box;
// 為了使用 black_box，你可能需要將 Rust 版本升級到 1.62.0 或更高 (如果遇到錯誤)

// 1. 定義結構體 (包含生命週期 'b)
struct IntRef<'b>(&'b i32);

// 實現 AsRef Trait，以便 process_bound 函式能夠使用它
impl<'b> AsRef<i32> for IntRef<'b> {
    fn as_ref(&self) -> &i32 {
        self.0
    }
}

// 2. 定義泛型函式 (T: 'a 的核心)
#[inline(never)] // 強制編譯器不要 inline (內聯) 該函式
fn process_bound<'a, T: 'a>(item: T) -> i32
    where
        T: AsRef<i32> // T 必須能被視為 i32 的引用
{
    // 強制進行實際的記憶體讀取操作
    *item.as_ref()
}

fn main() {
    let val = 10;
    let iterations = 1_000_000_000;

    let start = Instant::now();
    let mut sum: i64 = 0;

    for _ in 0..iterations {
        // 使用 black_box 包裹輸入，防止編譯器優化掉 IntRef(&val)
        let result = process_bound(black_box(IntRef(&val)));
        sum += result as i64;
    }

    let duration = start.elapsed();

    // 最終結果也使用 black_box，防止整個迴圈被優化
    let final_sum = black_box(sum);
    println!("Rust Result: {}, Time: {:?}", final_sum, duration);
}
/*
cd examples/lifetime/ref_t_with_a
cargo run --release

Rust Result: 10000000000, Time: 253.7863ms
*/
```

C 的實作中使用了 `struct IntRef` 包含裸指標 `int*`，並以傳值方式傳遞結構體

C 的執行時間是優化後的基線，這個時間代表了 CPU 執行 10 億次「讀取結構體中的指標 -> 解引用 -> 加法」操作所需的真實時間

```c
#include <stdio.h>
#include <time.h>

typedef struct
{
    int *ptr;
} IntRef;

// 禁止 inline 以公平比較
__attribute__((noinline)) int process_bound(IntRef item)
{
    return *(item.ptr);
}

int main()
{
    int val = 10;
    IntRef wrapper = {&val};
    long long iterations = 1000000000;

    clock_t start = clock();
    long long sum = 0;

    for (long long i = 0; i < iterations; i++)
    {
        // C 語言直接傳遞 struct (by value)，裡面包含指標
        IntRef temp = {&val};
        sum += process_bound(temp);
    }

    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;

    printf("C Result: %lld, Time: %f s\n", sum, time_spent);
    return 0;
}
/*
cd examples\lifetime\ref_t_with_a
gcc test.c -O3 -march=native -o test
./test

C Result: 10000000000, Time: 0.315000 s
*/
```

Rust 在這個測試中甚至比 C 語言版本更快，這並不意味著 Rust 永遠比 C 快。它更可能意味著：

- **LLVM 的優化效果**

雖然兩種語言都使用 LLVM 後端（gcc 通常也使用 LLVM 或類似的強力優化）

但在這個特定、簡單的測試案例中，rustc 結合 LLVM 的優化策略在處理 black_box 繞過優化後，展現了輕微優勢

- **證明了「零成本」**

最重要的是，Rust 的執行時間與 C 語言處於同一數量級，它們都是在 $10^9$ 次迭代中花費數百毫秒

這有力地證明了為了「記憶體安全」而付出的 `T: 'a` 等抽象成本，在執行時期是 0