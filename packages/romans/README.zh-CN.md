[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# romans

纯 CP 实现的零分配、严格模式罗马数字（`1..3999`）双向解析、校验与格式化库。

- **源包对齐**：对标 `npm` `romans` 与 `crates.io` `roman`（MIT）
- **包版本**：`1.0.0`
- **内存模型**：全程零堆分配，格式化结果直接写入调用方提供的栈缓冲区 `[]u8`。

## 安装与导入

```bash
cap add romans
```

```cp
import {
    MIN_ROMAN,
    MAX_ROMAN,
    MAX_ROMAN_LEN,
    toRoman,
    toRomanLower,
    romanize,
    romanizeChecked,
    parseRoman,
    deromanize,
    isValidRoman,
    romanLen,
    romanCharValue,
} from "romans";
```

## 核心 API

| 符号 | 签名 | 说明 |
| --- | --- | --- |
| `MIN_ROMAN` | `const MIN_ROMAN: u16` | 最小可表示罗马数字（`1`）。 |
| `MAX_ROMAN` | `const MAX_ROMAN: u16` | 最大可表示罗马数字（`3999`）。 |
| `MAX_ROMAN_LEN` | `const MAX_ROMAN_LEN: usize` | 合法罗马数字的最大字节长度（`15`，如 `MMMDCCCLXXXVIII`）。 |
| `romanCharValue` | `fn romanCharValue(b: u8): i32` | 返回单个 ASCII 罗马字符（`I/V/X/L/C/D/M`，忽略大小写）的数值，非法返回 `-1`。 |
| `romanLen` | `fn romanLen(num: u16): usize` | 计算 `num` 格式化后的精确字节长度（超出 `1..3999` 返回 `0`）。 |
| `toRoman` | `fn toRoman(num: u16, out: []u8): usize` | 格式化为大写罗马数字写入 `out`，返回写入字节数（越界或缓冲区不足返回 `0`）。 |
| `toRomanLower` | `fn toRomanLower(num: u16, out: []u8): usize` | 格式化为小写罗马数字写入 `out`，返回写入字节数。 |
| `romanize` | `fn romanize(num: i32, out: []u8): usize` | 对标 npm `romanize` 的 `i32` 封装，超出 `1..3999` 返回 `0`。 |
| `romanizeChecked` | `fn romanizeChecked(num: i32, out: []u8): usize?` | 带边界检查的格式化函数，越界或缓冲不足返回 `null`。 |
| `parseRoman` | `fn parseRoman(input: string): i32` | 严格解析规范罗马数字（忽略大小写），返回 `1..3999`，非法返回 `-1`。 |
| `deromanize` | `fn deromanize(input: string): i32?` | 返回可选类型 `i32?` 的严格解析函数，非法或非规范写法返回 `null`。 |
| `isValidRoman` | `fn isValidRoman(input: string): bool` | 判断 `input` 是否为 `1..3999` 范围内的合法规范罗马数字。 |

## 使用示例

### 1. 零分配格式化到栈缓冲区

```cp
import { toRoman, toRomanLower } from "romans";

fn formatExample(): void {
    let mut buf: [16]u8 = [0; 16];
    const n = toRoman(2026, buf[0..]);
    // buf[0..n] == "MMXXVI"

    const m = toRomanLower(42, buf[0..]);
    // buf[0..m] == "xlii"
}
```

### 2. 严格解析与合法性校验

```cp
import { deromanize, isValidRoman, parseRoman } from "romans";

fn parseExample(): void {
    const val = deromanize("MCMXCIV"); // Some(1994)
    const bad = deromanize("IIII");    // null（拒绝非规范写法）
    const ok = isValidRoman("xliv");   // true
    const raw = parseRoman("MMXXVI");  // 2026（非法时返回 -1）
}
```

### 3. 长度预估与安全检查写入

```cp
import { romanLen, romanizeChecked } from "romans";

fn checkedExample(): void {
    const needed = romanLen(3888); // 15 ("MMMDCCCLXXXVIII")
    let mut out: [16]u8 = [0; 16];
    if (let written = romanizeChecked(3888, out[0..])) {
        // out[0..written] 即为最终结果
    }
}
```

## 与源包差异与安全亮点

- **零堆分配**：不同于 JS 版 `romanize(n)` 每次分配新字符串，CP 版直接写入调用方栈切片 `out: []u8`（16 字节即可覆盖所有合法结果，`MAX_ROMAN_LEN == 15`）。
- **严格规范校验**：自动拒绝 `IIII`、`VV`、`IC` 等非标准减法或重复写法。
- **无异常开销**：使用 `usize?` / `i32?` 可选值或 `0` / `-1` 哨兵值反馈错误，完全消除运行时异常。
