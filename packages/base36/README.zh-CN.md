[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 速查](README_AGENT.md)

# `base36`

> **源包溯源**：`npm:base36` ([base36](https://www.npmjs.com/package/base36)) & `crates.io:radix_fmt` ([davimiku/radix_fmt](https://github.com/davimiku/radix_fmt)) — 协议：MIT

纯 CP 实现的零堆分配 Base36 与任意进制（`2..36`）64 位无符号整数编解码器、防溢出解析器、任意字节流 Base36 编解码器与校验工具。

## 零成本迁移对照（TS / Rust → CP）

直接沿用上游原包名 `base36`，无需任何改名：

```typescript
// TypeScript / Node.js (npm: base36)
import base36 from "base36";
const encoded = base36.base36encode(123456789);
const decoded = base36.base36decode(encoded);
```

```rust
// Rust (crates.io: radix_fmt)
use radix_fmt::radix_36;
let encoded = format!("{}", radix_36(123456789u64));
```

```cp
// CP（零堆分配，直接写入调用方缓冲区）
import { encodeBase36, decodeBase36 } from "base36";

let buf: [32]u8 = undefined;
let n = encodeBase36(123456789, buf[0..32]);
let encoded = buf[0..n] as string; // "21i3v9"
let res = decodeBase36(encoded);   // res.valid == true, res.value == 123456789
```

## 安装配置

在项目的 `cap.json` 中添加依赖：

```json
{
  "dependencies": {
    "base36": "1.0.0"
  }
}
```

## 核心 API 速览

| 符号 | 签名 | 说明 |
| :--- | :--- | :--- |
| `ParseU64Result` | `class { value: u64, valid: bool }` | 防溢出整数解码结果 |
| `DecodeBytesResult` | `class { len: usize, valid: bool }` | 任意字节切片 Base36 解码结果 |
| `encodeBase36` | `(val: u64, out: []u8): usize` | 将 `u64` 编码为小写 Base36 |
| `decodeBase36` | `(input: string): ParseU64Result` | 大小写不敏感解析 Base36 为 `u64` |
| `encodeU64` | `(val: u64, out: []u8): usize` | `encodeBase36` 别名（小写） |
| `encodeU64Upper` | `(val: u64, out: []u8): usize` | 将 `u64` 编码为大写 Base36 |
| `decodeU64` | `(input: string): ParseU64Result` | `decodeBase36` 别名 |
| `encodeRadix` | `(val: u64, radix: u8, uppercase: bool, out: []u8): usize` | 任意进制 `2..=36` 编码 |
| `decodeRadix` | `(input: string, radix: u8): ParseU64Result` | 任意进制 `2..=36` 防溢出解码 |
| `encodedLenU64` | `(val: u64, radix: u8): usize` | 预先计算编码后的精确字节长度 |
| `isValidBase36` | `(input: string): bool` | 校验非空 Base36 合法性 |
| `isValidBase36String` | `(input: string): bool` | `isValidBase36` 别名 |
| `encodeBytes` | `(data: &[]u8, uppercase: bool, out: []u8): usize` | 将任意字节切片编码为 Base36 |
| `decodeBytes` | `(input: string, out: []u8): DecodeBytesResult` | 将 Base36 字符串解码回原始字节 |
| `digitChar` / `digitVal` | `(v: u8, uppercase: bool): u8` / `(c: u8): i32` | 单字符进制转换辅助函数 |

## 实战示例

### 1. 整数 Base36 与任意进制编解码

```cp
import {
    encodeU64,
    encodeU64Upper,
    decodeU64,
    encodeRadix,
    decodeRadix,
    encodedLenU64,
    isValidBase36,
} from "base36";

let out: [64]u8 = undefined;

// 小写与大写 Base36
let n1 = encodeU64(46655, out[0..64]);            // "zzz" (n1 == 3)
let n2 = encodeU64Upper(46655, out[0..64]);       // "ZZZ" (n2 == 3)
let parsed = decodeU64("ZZZ");                    // parsed.valid == true, parsed.value == 46655

// 任意进制（二进制 / 十六进制等）
let need = encodedLenU64(255, 16);                // 2
let n3 = encodeRadix(255, 16, true, out[0..64]);  // "FF"
let hex = decodeRadix("ff", 16);                  // hex.valid == true, hex.value == 255

// 合法性校验
let ok = isValidBase36("a1B2z9");                 // true
let bad = isValidBase36("not-valid!");            // false
```

### 2. 任意字节缓冲区 Base36 编解码

```cp
import { encodeBytes, decodeBytes } from "base36";

let raw: [4]u8 = [0xDE, 0xAD, 0xBE, 0xEF];
let encoded_buf: [64]u8 = undefined;
let enc_len = encodeBytes(&raw[0..4], false, encoded_buf[0..64]);
let enc_str = encoded_buf[0..enc_len] as string;

let decoded_buf: [16]u8 = undefined;
let dec = decodeBytes(enc_str, decoded_buf[0..16]);
if (dec.valid) {
    // decoded_buf[0..dec.len] 即为原始字节
}
```

## 注意事项与边界说明

- **缓冲区容量**：`u64` 在 Base36 下最多占 13 字节，在二进制下最多占 64 字节；可先调用 `encodedLenU64(val, radix)` 预估长度。
- **溢出保护**：空字符串、非法字符、进制越界或 `u64` 溢出时均返回 `valid: false`，不会触发算术崩溃。
- **零堆分配**：所有接口均直接写入调用方传入的 `[]u8` 缓冲区。
