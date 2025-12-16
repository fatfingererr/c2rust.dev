use std::time::Instant;

#[inline(always)]
fn process<T>(value: T) -> u64 {
    // 讓編譯器「一定要」把 value 用進去，避免整段被優化掉
    // black_box 會阻止過度的常量折疊/刪除
    let v = std::hint::black_box(value);

    // 我們不去假設 T 有任何 trait，因此用「取位址」當作觀察點：
    // 1) 不需要 Copy / Deref / Borrow
    // 2) 對 &T / &mut T / owned T 都成立
    // 3) 成本通常會被 inline 到幾乎 0
    let addr = &v as *const T as usize as u64;

    // 再 black_box 一次，避免 addr 也被視為可預測
    std::hint::black_box(addr)
}

#[inline(always)]
fn process_mut<T>(value: &mut T) -> u64 {
    // 對 &mut 我們額外做一個「可觀察的寫入」：交換一次位元（針對 u64）
    // 但因為是泛型 T，我們不能直接改；所以示範用 u64 專用版本比較更公平
    let addr = value as *mut T as usize as u64;
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
