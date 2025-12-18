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
