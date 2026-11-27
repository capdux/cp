[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# crypto-bigint

> 版本：`1.0.0` | 许可证：`Apache-2.0 OR MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation constant-time fixed-width 256-bit unsigned integer (U256) arithmetic and modular operations (RustCrypto crypto-bigint parity) for CP

## 安装与导入

```bash
cap add crypto-bigint
```

```cp
import { cbiMask64 } from "crypto-bigint";
```

## 核心导出 API

- `export fn cbiMask64(): u128;`
- `export fn cbiAllOnes64(): u64;`
- `export fn cbiCtMask(cond: bool): u64;`
- `export fn cbiReadBeU64(b: &[]u8, off: usize): u64;`
- `export fn cbiReadLeU64(b: &[]u8, off: usize): u64;`
- `export fn cbiWriteBeU64(v: u64, out: []u8, off: usize);`
- `export fn cbiWriteLeU64(v: u64, out: []u8, off: usize);`
- `export fn cbiHexNibble(ch: u8): i32;`
- `export fn cbiHexChar(nibble: u8): u8;`
- `export fn cbiBitLen64(v_in: u64): usize;`
- `export fn cbiAddWithCarry(a: U256, b: U256): CbiAddRes;`
- `export fn cbiSubWithBorrow(a: U256, b: U256): CbiSubRes;`
- `export fn addMod(a: U256, b: U256, m: U256): U256;`
- `export fn subMod(a: U256, b: U256, m: U256): U256;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`crypto-bigint@1.0.0`)，许可证：Apache-2.0 OR MIT
