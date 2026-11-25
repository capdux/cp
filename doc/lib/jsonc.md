# `cap:jsonc` — 零分配 JSONC 注释与尾逗号剥离器

> 参考来源：Rust [`json_comments`](https://crates.io/crates/json_comments) crate & `/data/dev/bun/src/js_parser.zig` | 深度优化日期：2026-11-26

## 概述

`cap:jsonc` 提供纯 CP、零堆分配、字符串转义感知的 JSONC（JSON with Comments and Trailing Commas，如 `tsconfig.json`、`cap.json`、VS Code 配置）剥离与扫描工具，完整支持单行 `//`、多行 `/* ... */` 注释与 `}`/`]` 前尾逗号处理，并支持**空格原地替换模式（1:1 保持原始行号与列号偏移）**。

## 导出 API

| 符号 / 签名 | 说明 |
| :--- | :--- |
| `JsoncOptions` | 配置结构体：`strip_comments`、`strip_trailing_commas`、`preserve_lines`、`replace_with_spaces` |
| `stripJsonc(input, out): ([]const u8)?` | 紧凑剥离 `//`、`/* ... */` 与尾逗号写入 `out` |
| `stripJsoncPreserveLines(input, out): ([]const u8)?` | 剥离注释但保留块注释中的 `\n`，保持行号对齐 |
| `stripJsoncWithSpaces(input, out): ([]const u8)?` | 将注释与尾逗号替换为等宽空格（保留 `\n`/`\r`），保持**行号+列号 1:1 对齐** |
| `stripJsoncInPlace(buf: []u8): ([]const u8)?` | **零额外缓冲原地剥离**，直接在原可变缓冲区用空格覆写注释与尾逗号 |
| `stripJsoncWithOptions(input, opts, out)` | 完整配置模式（可单独控制注释剥离或尾逗号剥离） |
| `hasJsoncComments(input): bool` | 字符串感知的快速注释检测（不误报字符串内 `"https://..."`） |
| `countJsoncTrailingCommas(input): usize` | 统计合法尾逗号数量 |

## 使用示例

### 示例 1：紧凑剥离 `cap.json` / `tsconfig.json` 注释与尾逗号

```cp
import { stripJsonc, hasJsoncComments, countJsoncTrailingCommas } from "cap:jsonc";

fn demoCompact() {
    const raw = "{\n  // URL inside string is preserved\n  \"url\": \"https://cp-lang.org\",\n}\n";
    const has_c = hasJsoncComments(raw);        // true
    const tc = countJsoncTrailingCommas(raw);   // 1

    let buf: [256]u8 = [0; 256];
    const clean = stripJsonc(raw, buf[0..]).?;
    * = has_c;
    * = tc;
    * = clean;
}
```

### 示例 2：零额外缓冲原地剥离并 1:1 保留行号与列号（`stripJsoncInPlace`）

```cp
import { stripJsoncInPlace } from "cap:jsonc";

fn demoInPlace() {
    let text: [32]u8 = [0; 32];
    const src = "{\"a\": /* c */ 1,}";
    @memcpy(text[0..src.len], src);

    const clean = stripJsoncInPlace(text[0..src.len]).?;
    // clean == "{\"a\":         1 }" —— 长度、行号、列号与原文完全一致！
    * = clean;
}
```
