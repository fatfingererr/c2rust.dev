use std::any::Any;

fn main() {
    // 1. 建立一個容器，準備存放「任何類型」的資料
    // 這個變數活得比較久（存活於整個 main 函式）
    let mut storage: Option<Box<dyn Any>> = None;

    {
        // 2. 建立一個局部變數，它是「短命」的
        let short_lived = String::from("我是短命的字串");

        // 3. 取得它的引用 (&str)
        // 這個引用的生命週期 'a 只在這個大括號內有效
        let reference = &short_lived;

        // 4. 【關鍵點】嘗試把這個「帶有有限生命週期」的引用轉成 Any
        // 假設 Any 允許非 'static，這行會成功
        let dynamic_val: Box<dyn Any> = Box::new(reference);

        // 5. 將這個動態物件存入外部的 storage
        storage = Some(dynamic_val);
    } // 6. <--- 這裡是大括號結束點！
    // short_lived 在這裡被銷毀（Drop），記憶體被回收。
    // 注意：此時 storage 裡面還存著 dynamic_val，
    // 而 dynamic_val 裡面還存著指向 short_lived 的指標。

    // 7. 嘗試從外部容器拿回資料
    if let Some(any_val) = storage {
        // 8. 透過 downcast 嘗試將 Any 轉回原本的型別 &str
        // 因為 TypeId 只紀錄型別是 &str，不知道它的生命週期已經結束了
        if let Some(recovered_str) = any_val.downcast_ref::<&str>() {
            // 9. 【災難發生】Use-After-Free
            // 這裡的 recovered_str 指向的記憶體已經被釋放或是被別的資料蓋掉了
            println!("讀取到的內容：{}", recovered_str);
        }
    }
}
/*
cd examples/lifetime/any_static
cargo run --bin bad_any --release

error[E0597]: `short_lived` does not live long enough
  --> bad_any.rs:14:25
   |
10 |         let short_lived = String::from("我是短命的字串");
   |             ----------- binding `short_lived` declared here
...
14 |         let reference = &short_lived;
   |                         ^^^^^^^^^^^^ borrowed value does not live long enough
...
18 |         let dynamic_val: Box<dyn Any> = Box::new(reference);
   |                                         ------------------- coercion requires that `short_lived` is borrowed for `'static`
...
22 |     } // 6. <--- 這裡是大括號結束點！
   |     - `short_lived` dropped here while still borrowed
*/
