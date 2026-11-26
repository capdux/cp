# @cap/tinystr

Zero-allocation register-packed short ASCII strings (`TinyStr4` / `TinyStr8`) for CP — clean-room MIT parity with Rust ICU4X `tinystr` (835M downloads on crates.io).

纯 CP 零堆分配寄存器打包短 ASCII 字符串引擎：将 1-4 / 1-8 字节非空 ASCII 字符直接打包进 `u32` / `u64` 寄存器，实现零指针拷贝与单 CPU 指令 O(1) 比较，适用于货币码 (`USD`)、语言码 (`zh`/`en`)、4CC (`av01`)、HTTP 方法 (`DELETE`/`OPTIONS`) 与协议名 (`https`)。

## Install / 安装

```bash
cap add @cap/tinystr
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `TinyStr4` | `struct { raw: u32 }` | 1-4 byte non-NUL ASCII packed in `u32` |
| `TinyStr8` | `struct { raw: u64 }` | 1-8 byte non-NUL ASCII packed in `u64` |
| `fromStr` | `(s: string) -> TinyStrN?` | Pack ASCII string (rejects empty, over-length, NUL, non-ASCII) |
| `len` | `() -> usize` | Byte length |
| `equals` | `(other: TinyStrN) -> bool` | Single-instruction register comparison (`_.raw == other.raw`) |
| `toAsciiLower` / `toAsciiUpper` / `toAsciiTitle` | `() -> TinyStrN` | ASCII case conversion |
| `isAsciiAlpha` / `isAsciiNumeric` / `isAsciiAlphanumeric` | `() -> bool` | ASCII character class predicates |
| `writeTo` | `(out: []u8) -> string` | Unpack into caller buffer |

## Usage / 使用示例

```cp
import { TinyStr4, TinyStr8 } from "@cap/tinystr";

const usd = TinyStr4.fromStr("USD") ?? TinyStr4{ raw: 0 as u32 };
const lower = usd.toAsciiLower(); // "usd"

const method = TinyStr8.fromStr("OPTIONS") ?? TinyStr8{ raw: 0 as u64 };
let buf: [8]u8 = undefined;
const text = method.writeTo(buf[0..8]); // "OPTIONS"
```
