use std::hint::black_box;
use std::time::Instant;

#[inline(never)]
fn get_middle_offset(len: usize) -> usize {
    let start = len / 4;
    let end = (len / 4) * 3;
    end - start
}

fn main() {
    let s = "a".repeat(1000);
    let s_len = s.len();
    let mut dummy: u64 = 0; // 使用 u64 確保容納 5000 億

    let start = Instant::now();
    for _ in 0..1_000_000_000 {
        // 1. 模擬 C 語言傳入參數並計算偏移
        let offset = get_middle_offset(black_box(s_len));
        // 2. 模擬 C 語言最後的 dummy += 500
        dummy = dummy.wrapping_add(offset as u64);
    }
    let duration = start.elapsed();

    println!("Rust Result: {}, Time: {:?}", dummy, duration);
}

/*
cd examples/lifetime/elision_rules
// 修改 Cargo.toml 的 bin.name 為 fast
cargo run --release

Rust Result: 500000000000, Time: 771.5585ms
*/
