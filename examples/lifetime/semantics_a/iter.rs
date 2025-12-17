struct ByteIter<'a> {
    remainder: &'a [u8],
}

impl<'a> ByteIter<'a> {
    // 錯誤標註：將回傳值與 self 的 mutable borrow 綁定
    fn next_restrictive<'b>(&'b mut self) -> Option<&'b u8> {
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
    let mut bytes = ByteIter { remainder: b"123" };

    let byte_1 = bytes.next_restrictive(); // 第一次借用：bytes 被標記為「正在被可變借用中」

    // ERROR 編譯錯誤！
    // 因為 byte_1 還活著，它要求 bytes 必須維持在「被 mutable 借用」的狀態。
    // 你不能在同一時間進行第二次可變借用
    let byte_2 = bytes.next_restrictive();

    println!("{:?}, {:?}", byte_1, byte_2);
}
