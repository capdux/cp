[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 速查](README_AGENT.md)

# bytecount

面向 CP 的零堆分配 16-lane 硬件 SIMD 字节频次统计、UTF-8 字符计数、行数统计与区间计数引擎（对齐 Rust `bytecount`）。

基于 `@Vector(16, u8)` 与 `@select`，采用 255 步向量内累加（零溢出、极少水平归约），高速统计单字节、闭区间 `[lo, hi]`、UTF-8 码点、逻辑行数及 ASCII 空白字符。

## 源包溯源与零成本迁移

| 属性 | 说明 |
| --- | --- |
| 源生态平台 | **crates.io** |
| 原包名与版本 | `bytecount`（CP 生态版 `v1.0.0`） |
| 上游仓库 | `https://github.com/llogiq/bytecount` |
| 开源协议 | MIT（上游：Apache-2.0 OR MIT） |

从 Rust 迁移至 CP 保持原包名，零成本切换：

```rust
// Rust (crates.io: bytecount)
let n = bytecount::count(haystack, b'\n');
let chars = bytecount::num_chars(utf8_bytes);
```

```cp
// CP (cap: bytecount) — 16-lane SIMD + 255 步通道批累加
import { count, countFast, numChars, countLines } from "bytecount";
const n = countFast(&haystack, 0x0A);
const chars = numChars(&utf8_bytes);
```

## 安装

```bash
cap add bytecount
```

## 实战示例

### 基础用法：16-lane SIMD 字节、UTF-8 字符与行数统计

```cp
import { count, countFast, numChars, countLines } from "bytecount";

const text: []const u8 = "line1\nline2\nline3";

// 16-lane SIMD 字节频次统计
const nls1 = count(&text, '\n' as u8);     // 2（复用 memchr.countByte）
const nls2 = countFast(&text, '\n' as u8); // 2（255 步 SIMD 累加器）

// 逻辑行数统计（自动处理末尾是否带换行符）
const lines = countLines(&text);           // 3

// UTF-8 码点计数（跳过 0x80..0xBF 续字节）
const utf8: []const u8 = "hello 世界!";
const chars = numChars(&utf8);             // 9
```

### 进阶用法：字节区间与空白字符分析

```cp
import { countInRange, countAsciiWhitespace } from "bytecount";

const logLine: []const u8 = "2026-11-27 16:45:00 INFO status=200 latency=12ms\n";

// 16-lane SIMD 统计 ASCII 数字 '0'..'9' 个数
const digits = countInRange(&logLine, '0' as u8, '9' as u8);

// 统计大写字母 'A'..'Z' 个数
const uppers = countInRange(&logLine, 'A' as u8, 'Z' as u8);

// 统计 ASCII 空白符（空格、\t、\n、\r、\f）
const ws = countAsciiWhitespace(&logLine);
```

## API 速查表

| 函数 | 签名 | 说明 |
| --- | --- | --- |
| `count` | `fn(haystack: &[]u8, needle: u8): usize` | 统计 `needle` 出现次数（复用 `memchr.countByte`）。 |
| `countFast` | `fn(haystack: &[]u8, needle: u8): usize` | 16-lane SIMD + 255 步 `u8` 向量累加统计。 |
| `numChars` | `fn(utf8_bytes: &[]u8): usize` | 16-lane SIMD 统计有效 UTF-8 字符数（`(b & 0xC0) != 0x80`）。 |
| `countLines` | `fn(buf: &[]u8): usize` | 统计行数（空切片返回 `0`，末尾 `\n` 不多计一行）。 |
| `countAsciiWhitespace` | `fn(buf: &[]u8): usize` | 统计 ASCII 空白字节（`0x20`/`0x09`/`0x0A`/`0x0D`/`0x0C`）。 |
| `countInRange` | `fn(buf: &[]u8, lo: u8, hi: u8): usize` | 16-lane SIMD 统计满足 `lo <= b <= hi` 的字节数（`lo > hi` 返回 `0`）。 |

## 性能特性

- **16-lane SIMD 255 步批累加**：每 4080 字节（`255 × 16`）仅做一次水平求和，显著降低向量归约瓶颈。
- **零堆分配**：全程操作只读切片 `&[]u8`。

## 协议

MIT — 对齐 Rust `bytecount` 的纯 CP 净室实现。
