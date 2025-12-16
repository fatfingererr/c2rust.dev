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
