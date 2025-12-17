use std::time::Instant;

struct ByteIter<'remainder> {
    remainder: &'remainder [u8],
}

impl<'remainder> ByteIter<'remainder> {
    fn new(remainder: &'remainder [u8]) -> Self {
        Self { remainder }
    }

    // Correct lifetime: bound to the data source 'remainder
    fn next(&mut self) -> Option<&'remainder u8> {
        if self.remainder.is_empty() {
            None
        } else {
            let byte = &self.remainder[0];
            self.remainder = &self.remainder[1..];
            Some(byte)
        }
    }
}

fn main() {
    // Prepare 100 million bytes (approx. 100MB)
    let data = vec![1u8; 100_000_000];
    let mut iter = ByteIter::new(&data);
    let mut sum: u64 = 0;

    println!("Rust: Calculation started...");
    let start = Instant::now();

    // The Option<T> is optimized away by the compiler into a null pointer check
    while let Some(b) = iter.next() {
        sum += *b as u64;
    }

    let duration = start.elapsed();
    println!("Rust Sum: {}", sum);
    println!("Rust Time elapsed: {:.3?}", duration);
}
/*
cd examples/lifetime/semantics_a
cargo run --bin test --release

Rust: Calculation started...
Rust Sum: 100000000
Rust Time elapsed: 14.645ms
*/
