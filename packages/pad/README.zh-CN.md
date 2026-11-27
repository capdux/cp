[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 速查](README_AGENT.md)

# `pad`

> **源包溯源**：`npm:pad` ([adaltas/node-pad](https://github.com/adaltas/node-pad))、`crates.io:pad` ([ogham/rust-pad](https://github.com/ogham/rust-pad)) 与 `npm:left-pad` — 协议：MIT

纯 CP 实现的零堆分配、UTF-8 码点感知字符串填充库，完整覆盖左/右/居中填充（`pad`、`padStart`、`padEnd`、`padLeft`、`padRight`、`padCenter`、`padCenterWith`）、安全截断（`truncateChars`）与重复拼接（`repeatStr`）。

## 零成本迁移对照（TS / Rust → CP）

直接沿用原包名 `pad`，无需改名：

```typescript
// TypeScript / Node.js (npm: pad)
import pad from "pad";
const left = pad(8, "hi", "0");      // "000000hi"
const right = pad("hi", 8, ".");     // "hi......"
```

```rust
// Rust (crates.io: pad)
use pad::{PadStr, Alignment};
let s = "hi".pad_to_width_with_alignment(8, Alignment::Middle);
```

```cp
// CP（零堆分配，UTF-8 码点感知）
import { padStart, padEnd, padCenter } from "pad";

let buf: [64]u8 = undefined;
let n1 = padStart("hi", 8, '0', buf[0..64]);   // "000000hi"
let n2 = padEnd("hi", 8, '.', buf[0..64]);     // "hi......"
let n3 = padCenter("hi", 8, ' ', buf[0..64]);  // "   hi   "
```

## 安装配置

在 `cap.json` 中声明依赖：

```json
{
  "dependencies": {
    "pad": "1.0.0"
  }
}
```

## 核心 API 速览

| 符号 | 签名 | 说明 |
| :--- | :--- | :--- |
| `PadOptions` | `class { pad_char: u8, pad_str: string, truncate: bool, left: bool, fn defaults(): PadOptions }` | 填充配置（多字符模式、超长截断、对齐方向） |
| `pad` | `(s: string, target_chars: usize, opts: PadOptions, out: []u8): usize` | 完整填充接口，支持左右对齐、自定义 `pad_str` 与截断 |
| `padCenterWith` | `(s: string, target_chars: usize, opts: PadOptions, out: []u8): usize` | 带 `PadOptions` 的居中填充 |
| `padStart` / `padLeft` | `(s: string, target_chars: usize, pad_char: u8, out: []u8): usize` | 单字符左侧填充至 `target_chars` 码点 |
| `padEnd` / `padRight` | `(s: string, target_chars: usize, pad_char: u8, out: []u8): usize` | 单字符右侧填充至 `target_chars` 码点 |
| `padCenter` | `(s: string, target_chars: usize, pad_char: u8, out: []u8): usize` | 单字符居中填充 |
| `repeatStr` | `(s: string, count: usize, out: []u8): usize` | 将字符串重复 `count` 次写入 `out` |
| `countChars` | `(s: string): usize` | 统计 UTF-8 码点数量 |
| `truncateChars` | `(s: string, max_chars: usize): string` | 零拷贝按 UTF-8 码点边界截断 |
| `calcPadBytes` | `(pad_chars: usize, pad_char: u8, pad_str: string): usize` | 预计算填充所需的字节数 |
| `writePadPattern` | `(dst: []u8, pos: usize, pad_chars: usize, pad_char: u8, pad_str: string): usize` | 在指定偏移处写入重复填充串 |
| `copyBytes` / `fillByte` | `(dst: []u8, pos: usize, src: string)` / `(dst: []u8, pos: usize, count: usize, ch: u8)` | 底层缓冲区拷贝与字节填充 |

## 实战示例

### 1. 基础填充与重复

```cp
import { padLeft, padRight, padCenter, repeatStr, countChars } from "pad";

let buf: [64]u8 = undefined;

let n1 = padLeft("42", 5, '0', buf[0..64]);     // "00042"
let n2 = padRight("OK", 6, '.', buf[0..64]);    // "OK...."
let n3 = padCenter("CP", 6, '-', buf[0..64]);   // "--CP--"
let n4 = repeatStr("ab", 3, buf[0..64]);        // "ababab"
let chars = countChars("你好CP");                // 4（按 UTF-8 码点计数）
```

### 2. 多字符填充模式与超长截断

```cp
import { PadOptions, pad, padCenterWith, truncateChars } from "pad";

let buf: [64]u8 = undefined;

let opts = PadOptions.defaults();
opts.pad_str = ".-";
opts.left = true;
opts.truncate = true;

let n1 = pad("hello world", 5, opts, buf[0..64]);       // "hello"（触发截断）
let n2 = pad("hi", 6, opts, buf[0..64]);                // ".-.-hi"
let n3 = padCenterWith("CP", 6, opts, buf[0..64]);      // ".-CP.-"

let short = truncateChars("你好世界", 2);                // "你好"（零拷贝切片）
```

## 注意事项与边界说明

- **UTF-8 码点安全**：所有长度计算与截断均按 UTF-8 码点边界执行，不会把多字节中文或 Emoji 截成乱码。
- **零堆分配**：写入目标均为调用方提供的 `[]u8` 缓冲区，返回实际写入字节数。
- **截断控制**：`PadOptions.defaults()` 默认 `truncate: false`，源串超过 `target_chars` 时原样输出。
