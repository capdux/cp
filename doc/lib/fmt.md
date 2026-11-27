# `cap:fmt` — Zero-Heap Buffered Formatting & Strict Integer Parsing

## 1. 概述
`cap:fmt` 是 CP 生态的零堆（Zero-Heap）格式化与数值解析基础库，面向调用方提供的固定大小 `[]u8` 缓冲区提供确定性、防越界截断的文本/整数/十六进制/定点数/JSON 字符串写入，以及严格字符校验、防溢出的无堆整数解析。

## 2. 核心类型：`BufWriter`
```cp
pub struct BufWriter {
    out: []u8,
    pos: usize,
}
```

| 方法 | 签名 | 说明 |
| :--- | :--- | :--- |
| `BufWriter.new` | `(out: []u8): BufWriter` | 构造绑定到 `out` 的零堆写入器（`pos = 0`） |
| `written` | `(): usize` | 返回已写入字节数 |
| `remaining` | `(): usize` | 返回剩余可写字节数 |
| `asString` | `(): string` | 返回已写入内容切片 `out[0..pos]` |
| `writeByte` | `(*_, b: u8)` | 写入单字节（缓冲区满时静默截断，不越界） |
| `writeStr` | `(*_, s: string)` | 写入字符串切片 |
| `writeRepeat` | `(*_, ch: u8, count: usize)` | 重复写入指定字节 `count` 次 |
| `writeU64` | `(*_, val: u64)` | 十进制无符号 64 位整数写入 |
| `writeI64` | `(*_, val: i64)` | 十进制有符号 64 位整数写入（安全支持 `i64` 最小值） |
| `writeHexU64` | `(*_, val: u64, uppercase: bool)` | 十六进制 64 位整数写入（支持大/小写） |
| `writeHexByte` | `(*_, b: u8, uppercase: bool)` | 写入固定 2 位十六进制字节 |
| `writeFloatFixed` | `(*_, val: f64, decimals: u8)` | 定点小数格式化（四舍五入、支持 `NaN` / `±Infinity`） |
| `writeJsonStr` | `(*_, s: string)` | 写入带双引号的 RFC 8259 转义 JSON 字符串 |

## 3. 自由函数 API
| 函数 | 签名 | 说明 |
| :--- | :--- | :--- |
| `writeStr` | `(s: string, out: []u8, pos: usize): usize` | 向 `out[pos..]` 写入字符串并返回新偏移 |
| `writeU64` | `(val: u64, out: []u8, pos: usize): usize` | 向 `out[pos..]` 写入 `u64` 并返回新偏移 |
| `writeI64` | `(val: i64, out: []u8, pos: usize): usize` | 向 `out[pos..]` 写入 `i64` 并返回新偏移 |
| `writeHexByte` | `(b: u8, uppercase: bool, out: []u8, pos: usize): usize` | 向 `out[pos..]` 写入 2 位十六进制字节并返回新偏移 |
| `parseU64` | `(s: string): u64?` | 严格十进制解析 `u64`（支持可选 `+`，防溢出） |
| `parseI64` | `(s: string): i64?` | 严格十进制解析 `i64`（支持可选 `+`/`-`，安全支持 `i64` 最小值） |
| `parseU32` | `(s: string): u32?` | 严格十进制解析 `u32`（超界返回 `null`） |
| `parseU16` | `(s: string): u16?` | 严格十进制解析 `u16`（超界返回 `null`） |
| `parseHexU64` | `(s: string): u64?` | 严格十六进制解析 `u64`（支持大小写与可选 `0x`/`0X` 前缀，防溢出） |

## 4. 示例
```cp
import { BufWriter, parseI64, parseHexU64 } from "cap:fmt";

let buf: [64]u8 = undefined;
let w = BufWriter.new(buf[0..]);
w.writeStr("v=");
w.writeI64(-42);
w.writeStr(", hex=0x");
w.writeHexU64(255, true);
// w.asString() == "v=-42, hex=0xFF"

const n = parseI64("-9223372036854775808");
const h = parseHexU64("0xdeadbeef");
```
