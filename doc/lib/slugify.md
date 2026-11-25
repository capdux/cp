# `cap:slugify` — URL 安全 Slug 生成器与 Unicode Latin 音译器

> 参考来源：Rust [`slug`](https://crates.io/crates/slug) / [`deunicode`](https://crates.io/crates/deunicode) crate & `/data/dev/bun/src/string/immutable.zig` | 深度优化日期：2026-11-26

## 概述

`cap:slugify` 提供纯 CP、零外部依赖、零堆分配的 URL 安全 Slug 生成、Unicode Latin-1 / Latin Extended-A 变音符号音译、词边界感知定长截断、常见符号展开以及规范 Slug 合法性校验，适用于静态站点路由、多语言文章 URL、文档锚点 ID、包名归一化与短链生成。

## 核心能力与行为契约

1. **Unicode Latin 零分配音译（对标 Rust `deunicode`）**：
   - 覆盖完整 **Latin-1 Supplement（U+00C0..U+00FF）** 与 **Latin Extended-A（U+0100..U+017F）**，包括 `Café → cafe`、`Über Straße → uber-strasse`、`Ægir → aegir`、`Łódź → lodz`、`Œuvre → oeuvre`、`Þór → thor`、`François → francois`；
   - 英文直撇号（`'`）与印刷弯撇号（`‘` / `’`）均自动省略连接（`"Don’t Panic"` → `"dont-panic"`）；
   - 排版破折号（`–` / `—`）与商标符号（`™`）自动归一化。
2. **结构化配置 `SlugOptions`**：
   - `separator: u8 = '-'`：自定义分隔符（如 `'_'` 生成 snake_case）；
   - `lowercase: bool = true`：可设为 `false` 保留原大小写；
   - `max_len: usize = 0`：`> 0` 时在分隔符词边界处回退截断，避免将单词生硬劈开；
   - `expand_symbols: bool = false`：开启后将 `&` → `and`、`@` → `at`、`+` → `plus`、`%` → `percent`。
3. **规范校验**：
   - `isValidSlug(s)` 严格校验 `[a-z0-9]+(-[a-z0-9]+)*`；
   - `isValidSlugWithSep(s, sep)` 支持任意自定义分隔符校验。

## 导出 API

| 符号 / 签名 | 说明 |
| :--- | :--- |
| `SlugOptions` | 配置结构体：`separator`、`lowercase`、`max_len`、`expand_symbols` |
| `slugify(input, out): ([]const u8)?` | 生成标准 `'-'` 分隔的小写 URL Slug（含 Unicode Latin 音译），缓冲不足返回 `null` |
| `slugifyWithSep(input, sep, out): ([]const u8)?` | 使用自定义分隔符（如 `'_'`）生成 Slug |
| `slugifyWithOptions(input, opts, out): ([]const u8)?` | 完整配置模式：支持保留大小写、词边界截断、符号展开与自定义分隔符 |
| `slugifyBounded(input, max_len, out): ([]const u8)?` | 在 `max_len` 字节上限内按词边界截断生成标准 Slug |
| `deunicodeChar(cp: u32): []const u8` | 单码点零分配 Latin ASCII 音译（无映射时返回 `""`） |
| `deunicode(input, out): ([]const u8)?` | 保留空白与标点的纯文本 Latin 去重音音译 |
| `isValidSlug(s): bool` | 校验字符串是否符合标准连字符 Slug 规范 |
| `isValidSlugWithSep(s, sep): bool` | 校验字符串是否符合指定分隔符 `sep` 的 Slug 规范 |

## 使用示例

### 示例 1：基础 Slug 生成与多语言变音符号音译

```cp
import { slugify, isValidSlug } from "cap:slugify";

fn demoBasic() {
    let b1: [128]u8 = [0; 128];
    let b2: [128]u8 = [0; 128];

    const s1 = slugify("Don’t Panic: CP Lang v1.0!", b1[0..]).?;
    // s1 == "dont-panic-cp-lang-v1-0", isValidSlug(s1) == true

    const s2 = slugify("Café Crème — Über Straße & Łódź", b2[0..]).?;
    // s2 == "cafe-creme-uber-strasse-lodz"
}
```

### 示例 2：使用 `SlugOptions` 保留大小写、展开符号与词边界截断

```cp
import { SlugOptions, slugifyWithOptions, isValidSlugWithSep } from "cap:slugify";

fn demoOptions() {
    let b1: [128]u8 = [0; 128];
    let b2: [128]u8 = [0; 128];

    // 展开 & / @ / + / % 符号并使用下划线分隔
    const s1 = slugifyWithOptions(
        "C++ & Rust @ 100%",
        SlugOptions{ separator: '_', expand_symbols: true },
        b1[0..],
    ).?;
    // s1 == "c_plus_plus_and_rust_at_100_percent"
    // isValidSlugWithSep(s1, '_') == true

    // 保留大小写并在 15 字节内按词边界截断
    const s2 = slugifyWithOptions(
        "Hello World From CP",
        SlugOptions{ lowercase: false, max_len: 15 },
        b2[0..],
    ).?;
    // s2 == "Hello-World"
}
```

### 示例 3：纯文本去重音音译（`deunicode`）

```cp
import { deunicode } from "cap:slugify";

fn demoDeunicode() {
    let buf: [128]u8 = [0; 128];
    const ascii = deunicode("Crème brûlée in Zürich (Ægir)", buf[0..]).?;
    // ascii == "Creme brulee in Zurich (AEgir)"
}
```
