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
