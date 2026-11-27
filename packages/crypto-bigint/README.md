[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# crypto-bigint

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:crypto-bigint` ([https://github.com/RustCrypto/crypto-bigint](https://github.com/RustCrypto/crypto-bigint))

Zero-allocation constant-time fixed-width 256-bit unsigned integer (`U256`) arithmetic and modular operations for CP — clean-room parity with RustCrypto `crypto-bigint` (365M+ downloads on crates.io, Apache-2.0 OR MIT).

纯 CP 零堆分配恒定时间 256 位无符号大整数（`U256`，4×`u64` 小端 limbs）算术库，提供字节序/十六进制编解码、恒定时间加减乘移位、恒定时间比较/选择与密码学模加模减。

## Install / 安装

```bash
cap add crypto-bigint
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `U256` | `ZERO`, `ONE`, `fromU64`, `fromBeBytes`, `fromLeBytes`, `toBeBytes`, `toLeBytes`, `fromBeHex`, `toHex` | Fixed-width 256-bit unsigned integer & codecs |
| `U256` (arith) | `wrappingAdd`, `wrappingSub`, `wrappingMul`, `shl1`, `shr1`, `bitLen`, `ctEq`, `ctCmp`, `ctSelect` | Constant-time 256-bit arithmetic & comparisons |
| `addMod` / `subMod` | `(a: U256, b: U256, m: U256): U256` | Constant-time modular addition & subtraction |

## Usage / 使用示例

```cp
import { U256, addMod, subMod } from "crypto-bigint";

const a = U256.fromU64(100 as u64);
const b = U256.fromU64(50 as u64);
const m = U256.fromU64(120 as u64);
const r = addMod(a, b, m); // 30
```
