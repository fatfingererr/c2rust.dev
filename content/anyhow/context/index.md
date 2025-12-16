+++
title = "anyhow::Context"
weight = 1

[extra]
+++

在 C 中錯誤處理通常是這樣：

```c
if (read(fd, buf, len) < 0) {
    return -1;
}

// 更進階一點
FILE *fp = fopen(path, "r");
if (!fp) {
    perror("fopen failed");
    return -1;
}
```

為了更好的捕捉錯誤，去實現 **錯誤棧 (Error Stack)** 去傳遞錯誤，我們可以實作 `Context`：

1. 讓錯誤以錯誤碼呈現，實現 **Error is a value**
2. 讓錯誤可以被包裝（wrap）
3. 錯誤可以一路往上拋

```c
typedef struct Error {
    int code;
    const char *message;
    const char *context;
} Error;

// 不再只回傳錯誤碼 -1，而是回傳一個 Error*
Error *error_with_context(Error *inner, const char *ctx) {
    Error *err = malloc(sizeof(Error));
    err->code = inner->code;
    err->message = inner->message;
    err->context = ctx;
    return err;
}

Error *err = init_system();
if (err) {
    printf("Error: %s\nContext: %s\n", err->message, err->context);
}
```

而在 `Rust` 中只要一行就能搞定：

```rust
init_system().with_context(|| "初始化系統失敗").map_err(|e| eprintln!("{:#}", e)).ok();
```