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
