use std::io::{ self, Write };
use std::time::Instant;

fn main() {
    let n = 100000;

    let stderr = io::stderr();
    let mut err = stderr.lock(); // ✅ 對齊 C：只鎖一次

    let start = Instant::now();

    for i in 0..n {
        writeln!(err, "test {}", i).unwrap();
    }

    err.flush().unwrap(); // ✅ 對齊 C：只 flush 一次
    let elapsed = start.elapsed();

    // 計時結束後再輸出結果（不混進測試區間）
    eprintln!("Rust stderr locked writeln: {:?}", elapsed);
}
