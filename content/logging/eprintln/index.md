+++
title = "eprintln!"
weight = 1

[extra]
+++

在 C 裡，我們通常看到的是：

```c
fprintf(stderr, "failed to open file: %s\n", path);
```

而 `eprintln!` 是 Rust 標準函式庫提供的巨集，用來：

1. 輸出到 `stderr`
2. 自動補換行
3. 支援格式化字串
4. 在編譯期做格式與型別檢查

```rust
eprintln!("error code = {}", code);
```

如果格式或型別不對，編譯期就會報錯，而不是等到 runtime

## 性能對齊

### C

為了對齊 Rust 版本，我們做了以下限制：

1. 使用 `fprintf(stderr, ...)`，確保輸出目標與 Rust 的 `eprintln!` 完全一致，而非預設的 `stdout`
2. 每次僅輸出 `test <i>\n`，不額外加入檔名、行號、函數名或 log 前綴，避免字串長度與格式化成本差異影響結果
3. 在進入迴圈前先鎖住 `stderr`，在結束後再解鎖，對齊 Rust 中 `stderr().lock()` 的行為，避免鎖競爭成本主導 benchmark
4. 處理跨平台 `stdio` 鎖差異
5. 改用 `clock_gettime(CLOCK_MONOTONIC)`（Unix-like）或 `QueryPerformanceCounter`（Windows），避免 `clock()` 量測 CPU time 造成的失真
6. 僅在整個輸出迴圈結束後呼叫一次 `fflush(stderr)`，對齊 Rust 在手動 `flush()` 的情境，避免頻繁 flush 造成不公平的 I/O 成本
7. 使用 `-O2` 編譯，確保量測結果反映實際部署時的行為，而非 `debug build` 的額外開銷

```c
#include <stdio.h>
#include <stdlib.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
static double now_seconds(void) {
    static LARGE_INTEGER freq;
    static int init = 0;
    if (!init) {
        QueryPerformanceFrequency(&freq);
        init = 1;
    }
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart / (double)freq.QuadPart;
}

// Windows 的 stdio 鎖
#define LOCK_STDERR()   _lock_file(stderr)
#define UNLOCK_STDERR() _unlock_file(stderr)

#else
#include <time.h>
static double now_seconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

// POSIX 的 stdio 鎖
#define LOCK_STDERR()   flockfile(stderr)
#define UNLOCK_STDERR() funlockfile(stderr)

#endif

int main(void) {
    const int N = 100000;

    LOCK_STDERR();

    double start = now_seconds();

    for (int i = 0; i < N; i++) {
        fprintf(stderr, "test %d\n", i);
    }

    fflush(stderr);

    double end = now_seconds();

    UNLOCK_STDERR();

    fprintf(stderr,
            "C stderr locked fprintf: %.6f sec\n",
            (end - start));
    return 0;
}

/*
cd examples/logging/eprintln
gcc -O2 test.c -o test
.\test.exe 2> result_c.tmp
Get-Content result_c.tmp | Select-Object -Last 1

# C stderr locked fprintf: 6.666700 sec
*/
```

### Rust

```rust
use std::io::{self, Write};
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

/*
cd examples/logging/eprintln
cargo run --release 2>result_rs.tmp
Get-Content result_rs.tmp | Select-Object -Last 1

Rust stderr locked writeln: 6.630935s
#
```

在將 stderr 重導到檔案、並對齊 **一次鎖、一次 flush** 之後，C 與 Rust 的總耗時幾乎一致

這表示在大量輸出場景下，整體成本主要由 I/O 路徑（寫檔與緩衝）主導；語言層級的差異與封裝成本相對次要