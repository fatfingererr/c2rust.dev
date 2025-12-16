+++
title = "靜態 T: 'static"
weight = 1

[extra]
+++

在 `T: 'static` 中：
- `T` 是型別參數，如同 `fn f<T>(value: T){ ... }` 
- `:` 是 trait 或生命週期限制，意思是型別 `T` 需要滿足後續的限制
- `static` 說明限制是關於生命周期，也就是生命週期是整個程式執行期間

錯誤的例子：
```rust
fn bad<'a>(x: &'a i32) {
    requires_static(x); 
}

fn requires_static<T: 'static>(t: T) {}
```



一般人對 `T: 'static` 的印象就是：

1. 資料會活到程式結束
2. 一定是編譯期產生的
3. 不可變
4. 效能比較差 / 比較重

#### 1.「有能力」活到程式結束，但沒有一定要

簡單的反例：

```rust
fn consume<T: 'static>(value: T) {
    println!("use it");
    drop(value); // 這裡就死了
}

fn main() {
    let s = String::from("hello");
    consume(s);
    println!("program still running");
}
```

在這中 `String` 是 `T: 'static`，但它中途就被 `drop()` 釋放了

因此沒有強迫




這些直覺 **有一半來自 C 的 `static`，另一半來自錯誤的類比**

#### 正確理解

- `&'static T` 真正的永久 reference
- `T: 'static` 不含短生命週期 borrow 的型別
- `'static` 編譯期邊界，不是 runtime 特性





#### Real Static: 永遠有效的 **借用** `&'static T`

```rust
let s: &'static str = "hello";
```
這代表的是：
- 我持有一個 reference，
- 而我可以無限期地持有它，而且它永遠不會失效

這對被借用的 T 要求極高：
- 不能被移動
- 不能被釋放
- 通常不可變
- 位置必須固定

C 中等價於：

```c
static const char* s = "hello";
// or
static const int table[256] = { ... };
```

#### 可以活很久，但不代表一定得要 `T: 'static`

```rust
fn drop_static<T: 'static>(value: T) {
drop(value);
}

drop_static(String::from("hello"));
drop_static(vec![1, 2, 3]);
drop_static(42u64);
```
這意味著 `T: 'static` 不含任何比 `'static` 更短的借用

#### 效能面：零成本的生命週期保證

```rust
fn spawn_background<T: Send + 'static>(t: T) {
std::thread::spawn(move || {
drop(t);
});
}
```

這裡的 `'static`：

- 沒有 runtime 檢查
- 沒有 GC
- 沒有 indirection
- 完全是編譯期分析

也就保證了 `零成本安全（Zero-cost abstraction）`

#### 需要 `T: 'static` 的場景

- thread pool
- async executor
- networking pipeline
- lock-free queue

原因只有一個，runtime 追蹤 lifetime 太慢了，全部在編譯期解決


#### 誤解一

```rust
fn main() {
    let s: &'static str = "hello";
}
```

從這個最基本的例子可以看到：

- 字串常值在 binary 裡
- 放在唯讀區段
- 程式整個生命週期都有效

於是很自然地推導出以下結論（但它們是錯的）：

- `T: 'static` 表示 `T` 活到程式結束  
- `T: 'static` 就等於 `&'static T`  
- `'static` 一定不可變  
- `'static` 一定是編譯期建立的  


