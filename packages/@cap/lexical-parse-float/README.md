# @cap/lexical-parse-float

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:lexical-parse-float` ([https://github.com/Alexhuszagh/rust-lexical](https://github.com/Alexhuszagh/rust-lexical))

Zero-allocation pure CP strict floating-point and radix integer string parser — clean-room parity with `rust-lexical` (`lexical-parse-float`, `lexical-parse-integer`, and `lexical-util`, 158.3M+ downloads on crates.io, MIT).

纯 CP 零堆分配严格数值字符串解析器，支持带符号浮点数、科学计数法（`e`/`E`）、`inf`/`-inf`/`nan` 特殊值、2..36 进制有符号整数解析与严格 `i64` 溢出校验，并拒绝空串与尾随垃圾字符。

## Install / 安装

```bash
cap add @cap/lexical-parse-float
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `ParseFloatResult` | `struct { ok: bool; value: f64; consumed: usize; }` | Floating-point parse result |
| `ParseIntResult` | `struct { ok: bool; value: i64; consumed: usize; }` | Signed integer parse result |
| `parseFloat` | `(s: string): ParseFloatResult` | Strictly parse `f64` (sign, decimal, scientific, `inf`/`nan`) |
| `parseIntRadix` | `(s: string, radix: u32): ParseIntResult` | Strictly parse `i64` in radix `2..=36` with overflow protection |

## Usage / 使用示例

```cp
import { parseFloat, parseIntRadix } from "@cap/lexical-parse-float";

const f = parseFloat("-3.14159e2");
const i = parseIntRadix("ff", 16);
```
