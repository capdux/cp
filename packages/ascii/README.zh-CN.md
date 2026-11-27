[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# ascii

纯 CP 实现的零堆分配 ASCII 校验、强类型 `AsciiChar` / `AsciiStr` 封装、字节分类、大小写转换、去空白与忽略大小写比较库。

- **源包对齐**：对标 `crates.io` `ascii` `v1.1.0`（MIT，tomprogrammer/rust-ascii）
- **包版本**：`1.1.0`
- **内存模型**：全程零堆分配，支持只读 `AsciiStr` 视图、原地 `[]u8` 修改与栈缓冲写入。

## 安装与导入

```bash
cap add ascii
```

```cp
import {
    AsciiChar,
    AsciiStr,
    isAsciiAlpha,
    isAsciiDigit,
    isAsciiAlphanumeric,
    isAsciiHexDigit,
    isAsciiOctDigit,
    isAsciiWhitespace,
    isAsciiPrintable,
    isAsciiControl,
    isAsciiLowercase,
    isAsciiUppercase,
    isAsciiGraph,
    isAsciiPunctuation,
    toAsciiLowerByte,
    toAsciiUpperByte,
    eqIgnoreAsciiCase,
    toAsciiLower,
    toAsciiUpper,
    toAsciiLowercase,
    toAsciiUppercase,
    makeAsciiLowercase,
    makeAsciiUppercase,
    trimAscii,
} from "ascii";
```

## 核心 API

| 符号 | 签名 | 说明 |
| --- | --- | --- |
| 字节分类谓词 | `isAsciiAlpha`、`isAsciiDigit`、`isAsciiAlphanumeric`、`isAsciiHexDigit`、`isAsciiOctDigit`、`isAsciiWhitespace`、`isAsciiPrintable`、`isAsciiControl`、`isAsciiLowercase`、`isAsciiUppercase`、`isAsciiGraph`、`isAsciiPunctuation`（`(b: u8): bool`） | 完整的 ASCII 单字节分类判断函数。 |
| 单字节大小写 | `toAsciiLowerByte(b: u8): u8`、`toAsciiUpperByte(b: u8): u8` | 单个 ASCII 字母字节转小写/大写，非字母原样返回。 |
| `AsciiChar` | `class AsciiChar { byte: u8, ... }` | 已校验的 7-bit ASCII 字符：`fromByte(b): AsciiChar?`、`asByte()`、`isAlphabetic()`、`isAlphanumeric()`、`isControl()`、`isDigit()`、`isGraph()`、`isHexDigit()`、`isLowercase()`、`isPrintable()`、`isPunctuation()`、`isUppercase()`、`isWhitespace()`、`toAsciiUppercase()`、`toAsciiLowercase()`、`eqIgnoreAsciiCase(other)`。 |
| `AsciiStr` | `class AsciiStr { bytes: string, ... }` | 已校验的 ASCII 字符串视图：`fromStr(s): AsciiStr?`、`len()`、`isEmpty()`、`asStr()`、`get(idx): AsciiChar?`、`eqIgnoreAsciiCase(other)`、`trim()`、`trimStart()`、`trimEnd()`、`toAsciiLowercase(out): usize`、`toAsciiUppercase(out): usize`。 |
| `eqIgnoreAsciiCase` | `fn eqIgnoreAsciiCase(a: string, b: string): bool` | 忽略 ASCII 大小写比较两个字符串是否相等。 |
| `toAsciiLower` / `toAsciiLowercase` | `fn toAsciiLower(s: string, out: []u8): usize` | 将 `s` 转 ASCII 小写并写入 `out`，返回写入字节数。 |
| `toAsciiUpper` / `toAsciiUppercase` | `fn toAsciiUpper(s: string, out: []u8): usize` | 将 `s` 转 ASCII 大写并写入 `out`，返回写入字节数。 |
| `makeAsciiLowercase` / `makeAsciiUppercase` | `fn makeAsciiLowercase(buf: []u8)` / `fn makeAsciiUppercase(buf: []u8)` | 在 `buf` 上原地转换 ASCII 大小写，零额外内存。 |
| `trimAscii` | `fn trimAscii(s: string): string` | 去除首尾 ASCII 空白字符，返回零拷贝子切片。 |

## 使用示例

### 1. 强类型 `AsciiStr` 与 `AsciiChar` 校验

```cp
import { AsciiStr, AsciiChar } from "ascii";

fn validateDemo(): void {
    if (let s = AsciiStr.fromStr("  Hello-CP  ")) {
        const trimmed = s.trim(); // "Hello-CP"
        if (let ch = trimmed.get(0)) {
            const lower = ch.toAsciiLowercase().asByte(); // 'h'
        }
    }
    const invalid = AsciiChar.fromByte(0xFF); // null（非 ASCII）
}
```

### 2. 缓冲区与原地大小写转换

```cp
import { toAsciiLower, makeAsciiUppercase, eqIgnoreAsciiCase } from "ascii";

fn caseDemo(): void {
    let mut buf: [16]u8 = [0; 16];
    const n = toAsciiLower("Content-Type", buf[0..]); // "content-type"
    makeAsciiUppercase(buf[0..n]);                     // "CONTENT-TYPE"
    const same = eqIgnoreAsciiCase("HTTP", "http");    // true
}
```

## 与源包差异与安全亮点

- **零拷贝只读视图**：`AsciiStr` 直接封装只读 `string` 切片，`trim()` / `trimStart()` / `trimEnd()` 均不分配堆内存。
- **显式缓冲与原地修改**：以 `out: []u8` 缓冲写入与 `makeAsciiLowercase`/`makeAsciiUppercase` 原地修改替代堆分配 `AsciiString`。
- **底层协同**：复用 `cap:ascii` 原语，同时完整提供 Rust `ascii` crate 的强类型接口体验。
