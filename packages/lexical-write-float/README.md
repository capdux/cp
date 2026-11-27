[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# lexical-write-float

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:lexical-write-float` ([https://github.com/Alexhuszagh/rust-lexical](https://github.com/Alexhuszagh/rust-lexical))

Zero-allocation pure CP integer and floating-point formatter — clean-room parity with `rust-lexical` (`lexical-write-float` & `lexical-write-integer`, 138.4M+ downloads on crates.io, MIT).

纯 CP 零堆分配整数与浮点数格式化器，支持 2..36 进制有符号/无符号整数输出（安全处理 `i64::MIN`）、定点小数（含 `NaN`/`inf`/`-inf`/负零与四舍五入进位）以及科学计数法输出。

## Install / 安装

```bash
cap add lexical-write-float
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `writeUintRadix` | `(val: u64, radix: u32, uppercase: bool, out: []u8): usize` | Format `u64` in radix `2..=36` |
| `writeIntRadix` | `(val: i64, radix: u32, uppercase: bool, out: []u8): usize` | Format `i64` in radix `2..=36` (`i64::MIN`-safe) |
| `writeFloatFixed` | `(val: f64, decimals: u8, out: []u8): usize` | Format `f64` in fixed-point notation with rounding |
| `writeFloatSci` | `(val: f64, precision: u8, out: []u8): usize` | Format `f64` in scientific notation (`1.23e+4`) |

## Usage / 使用示例

```cp
import { writeIntRadix, writeFloatFixed, writeFloatSci } from "lexical-write-float";

let buf: [64]u8 = undefined;
const n1 = writeIntRadix(-255, 16, true, buf[0..64]); // "-FF"
const n2 = writeFloatFixed(3.14159, 2, buf[0..64]);   // "3.14"
const n3 = writeFloatSci(12345.0, 2, buf[0..64]);     // "1.23e+4"
```
