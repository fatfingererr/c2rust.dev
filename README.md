# impl&lt;C&gt; Fun for Rust

一個專注於 Rust 與 C 性能對比的 Tech Blog，提供實用的代碼範例、性能測試結果與詳細的技術解說。

**線上站點**：https://c2rust.dev
**技術棧**：[Zola](https://www.getzola.org/) + [Goyo](https://github.com/hahwul/goyo) 主題

---

## 目錄

- [快速開始](#快速開始)
- [內容管理](#內容管理)
- [配置管理](#配置管理)
- [範例代碼](#範例代碼)
- [部署到 GitHub Pages](#部署到-github-pages)
- [維護操作](#維護操作)
- [故障排除](#故障排除)
- [專案結構](#專案結構)
- [資源連結](#資源連結)
- [授權](#授權)

---

## 快速開始

### 環境要求

- **Zola** >= 0.17.0（[安裝指引](https://www.getzola.org/documentation/getting-started/installation/)）
- **Git**（用於子模組管理）

### 本地開發

```bash
# 克隆項目（含 Goyo 主題子模組）
git clone --recursive https://github.com/fatfingererr/c2rust.dev.git
cd c2rust.dev

# 如果已克隆但未拉取子模組，執行：
git submodule update --init --recursive

# 本地預覽（自動重載，預設 http://127.0.0.1:1111）
zola serve

# 本地預覽（包含草稿文章）
zola serve --drafts
```

### 構建生產版本

```bash
# 構建靜態檔案到 public/ 目錄
zola build

# 構建並顯示詳細資訊
zola build --verbose
```

---

## 內容管理

### 創建新文章

Zola 使用 **Section** 和 **Page** 兩種內容類型：

- **Section**：目錄層級，包含 `_index.md`（如 `content/logging/_index.md`）
- **Page**：實際文章，包含 `index.md`（如 `content/logging/eprintln/index.md`）

#### 創建新 Section

```bash
# 假設要創建 "error-handling" section
mkdir content/error-handling
echo "+++" > content/error-handling/_index.md
echo "title = \"錯誤處理\"" >> content/error-handling/_index.md
echo "weight = 3" >> content/error-handling/_index.md
echo "+++" >> content/error-handling/_index.md
```

#### 創建新文章（Page）

```bash
# 在 error-handling section 下創建 "result-type" 文章
mkdir content/error-handling/result-type
touch content/error-handling/result-type/index.md
```

然後編輯 `index.md`，參考下方的 Front matter 範本。

### Front matter 範本

每篇文章的開頭需要 TOML front matter，用於設定標題、日期、權重等：

```toml
+++
title = "文章標題"
weight = 1

[extra]
toc = true  # 啟用目錄
+++

文章內容從這裡開始...
```

**常用欄位說明**：

- `title`：文章標題（必填）
- `weight`：排序權重（數字越小越靠前）
- `[extra]`：額外設定
  - `toc = true`：在文章頁面顯示目錄（Table of Contents）

### 使用 Shortcodes

Goyo 主題提供 24+ 個預製 shortcodes，常用的包括：

#### 警告框

```markdown
{% alert_info(title="提示") %}
這是一個資訊提示框。
{% end %}

{% alert_warning(title="注意") %}
這是一個警告框。
{% end %}

{% alert_error(title="錯誤") %}
這是一個錯誤提示框。
{% end %}
```

#### 徽章

```markdown
{% badge_primary(text="Rust") %}
{% badge_success(text="穩定版") %}
{% badge_warning(text="實驗性") %}
```

#### 數學公式（KaTeX）

```markdown
{% math() %}
E = mc^2
{% end %}
```

#### Mermaid 圖表

```markdown
{% mermaid() %}
graph TD;
    A-->B;
    A-->C;
{% end %}
```

更多 shortcodes 請參考 [Goyo 主題文檔](https://github.com/hahwul/goyo)。

---

## 配置管理

站點配置位於 `config.toml`，以下是常用的配置項目。

### 站點基本設定

```toml
title = "impl&lt;C&gt; Fun for Rust"
base_url = "https://c2rust.dev"  # 部署時必須正確設定
compile_sass = false
build_search_index = true

[markdown]
highlight_code = true
```

**重要**：部署到 GitHub Pages 或自定義域名時，`base_url` 必須與實際域名一致，否則 CSS/JS 路徑會失效。

### Goyo 主題常用選項

在 `[extra]` 段落中設定主題參數：

```toml
[extra]
# Logo 設定
logo_text = "impl&lt;C&gt; Fun for Rust"

# 色彩設定
default_colorset = "dark"    # 預設主題："dark" 或 "light"
brightness = "normal"        # 亮度："darker", "normal", "lighter"

# 側邊欄設定
sidebar_expand_depth = 1     # 側邊欄預設展開層級（1-5）

# 頁腳 HTML
footer_html = "Powered by <a href='https://www.getzola.org'>Zola</a>"

# 主題切換按鈕
disable_theme_toggle = false  # 設為 true 則隱藏主題切換按鈕

# 編輯連結（可選）
edit_url = ""  # 例如："https://github.com/fatfingererr/c2rust.dev/edit/master"
```

### 導航選單配置

在 `[extra]` 中修改 `nav` 陣列：

```toml
[extra]
nav = [
  { name = "文章", url = "/", type = "url", icon = "fa-solid fa-book" },
  { name = "GitHub", url = "https://github.com/fatfingererr/c2rust.dev", type = "url", icon = "fa-brands fa-github" },
  { name = "更多", type = "dropdown", icon = "fa-solid fa-link", members = [
    { name = "關於", url = "/about", type = "url", icon = "fa-solid fa-user" }
  ] }
]
```

**圖示**：使用 [Font Awesome](https://fontawesome.com/icons) 圖示類別名稱。

---

## 範例代碼

`examples/` 目錄包含 C 與 Rust 性能對比範例：

```
examples/
└── logging/
    └── eprintln/
        ├── test.c          # C 實現（fprintf）
        ├── test.rs         # Rust 實現（eprintln!）
        ├── Cargo.toml      # Rust 專案配置
        └── result_*.tmp    # 性能測試結果
```

每個範例展示了相同功能在 C 和 Rust 中的實現方式與性能差異，包含完整的測試代碼與結果。詳細的編譯與執行說明請參閱各範例的文章頁面。

---

## 部署到 GitHub Pages

### 設置 GitHub Actions

創建 `.github/workflows/deploy.yml` 檔案：

```yaml
name: Deploy to GitHub Pages

on:
  push:
    branches: [master]
  workflow_dispatch:

permissions:
  contents: read
  pages: write
  id-token: write

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - name: Checkout
        uses: actions/checkout@v4
        with:
          submodules: recursive

      - name: Install Zola
        uses: taiki-e/install-action@v2
        with:
          tool: zola@0.19.1

      - name: Build
        run: zola build

      - name: Upload artifact
        uses: actions/upload-pages-artifact@v3
        with:
          path: ./public

  deploy:
    needs: build
    runs-on: ubuntu-latest
    environment:
      name: github-pages
      url: ${{ steps.deployment.outputs.page_url }}
    steps:
      - name: Deploy to GitHub Pages
        id: deployment
        uses: actions/deploy-pages@v4
```

### GitHub 儲存庫設定

1. 進入 GitHub 儲存庫的 **Settings** → **Pages**
2. **Source** 選擇 **GitHub Actions**
3. 推送代碼到 `master` 分支後，GitHub Actions 會自動構建並部署

### 域名設定

#### 使用 GitHub Pages 預設域名

如果使用 `https://fatfingererr.github.io/c2rust.dev/`，需在 `config.toml` 中設定：

```toml
base_url = "https://fatfingererr.github.io/c2rust.dev"
```

#### 使用自定義域名

1. 在 GitHub 儲存庫 **Settings** → **Pages** → **Custom domain** 設定域名（如 `c2rust.dev`）
2. 在 DNS 提供商設定 CNAME 記錄指向 `fatfingererr.github.io`
3. 在 `config.toml` 中設定：

```toml
base_url = "https://c2rust.dev"
```

### 手動部署

如果不使用 GitHub Actions，可以手動構建並推送到 `gh-pages` 分支：

```bash
# 構建
zola build

# 使用 ghp-import 工具（需先安裝）
pip install ghp-import
ghp-import -n -p -f public
```

---

## 維護操作

### 更新 Goyo 主題

Goyo 主題作為 Git 子模組管理，更新方式如下：

```bash
cd themes/goyo
git pull origin main
cd ../..
git add themes/goyo
git commit -m "chore: update Goyo theme"
git push
```

### 檢查構建錯誤

```bash
# 詳細構建模式（顯示所有處理檔案）
zola build --verbose

# 檢查內部連結是否有效
zola check
```

### 搜索索引重建

Zola 每次執行 `zola build` 時會自動重建搜索索引（`search_index.*.js`），無需手動操作。

如需停用搜索功能，在 `config.toml` 中設定：

```toml
build_search_index = false
```

---

## 故障排除

### 問題：子模組未初始化（themes/goyo/ 為空）

**原因**：克隆時未使用 `--recursive` 參數。

**解決方案**：

```bash
git submodule update --init --recursive
```

### 問題：構建失敗，錯誤訊息「找不到主題」

**原因**：`config.toml` 中 `theme = "goyo"` 設定錯誤，或 `themes/goyo/` 目錄不存在。

**解決方案**：

1. 確認 `config.toml` 中有 `theme = "goyo"`
2. 確認 `themes/goyo/theme.toml` 存在
3. 若不存在，執行子模組初始化：

```bash
git submodule update --init --recursive
```

### 問題：部署後 CSS/JS 失效，頁面無樣式

**原因**：`config.toml` 中的 `base_url` 設定錯誤。

**解決方案**：

確認 `base_url` 與實際部署的 URL 一致：

- GitHub Pages 預設域名：`https://fatfingererr.github.io/c2rust.dev`
- 自定義域名：`https://c2rust.dev`

修改後重新構建並部署。

### 問題：本地 `zola serve` 正常，但部署後 404

**原因**：可能是 `base_url` 與部署環境不符，或 GitHub Pages 未正確設定。

**解決方案**：

1. 確認 GitHub Pages 已啟用（Settings → Pages → Source 設為 GitHub Actions）
2. 檢查 GitHub Actions 構建日誌是否有錯誤
3. 確認 `base_url` 正確

---

## 專案結構

```
c2rust.dev/
├── .claude/              # Claude Code 配置（Git 子模組）
├── .github/
│   └── workflows/
│       └── deploy.yml    # GitHub Actions 自動部署配置
├── config.toml           # Zola 站點配置
├── content/              # 文章內容
│   ├── _index.md        # 首頁
│   ├── anyhow/          # Section: anyhow 錯誤處理
│   ├── compilation/     # Section: 編譯相關
│   └── logging/         # Section: 日誌輸出
│       └── eprintln/
│           └── index.md # Page: eprintln! 文章
├── themes/
│   └── goyo/            # Goyo 主題（Git 子模組）
├── examples/            # C vs Rust 範例代碼
│   └── logging/
│       └── eprintln/
│           ├── test.c
│           ├── test.rs
│           └── Cargo.toml
├── static/              # 靜態資源（圖片、檔案等）
├── sass/                # SASS 樣式（未使用，由主題提供）
├── templates/           # 自定義模板（未使用，由主題提供）
├── public/              # 構建輸出（由 .gitignore 排除）
├── LICENSE              # MIT 授權
└── README.md            # 本檔案
```

---

## 資源連結

- **Zola 官方文檔**：https://www.getzola.org/documentation/
- **Goyo 主題 GitHub**：https://github.com/hahwul/goyo
- **Goyo 主題 Demo**：https://goyo.hahwul.com
- **Tera 模板語法**：https://keats.github.io/tera/docs/
- **Font Awesome 圖示**：https://fontawesome.com/icons

---

## 授權

本專案採用 [MIT License](LICENSE)。
