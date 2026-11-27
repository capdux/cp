[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# num-traits

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap overflow-safe numeric operations and bit manipulation primitives: checked/saturating arithmetic, clamp, signum, absDiff, checkedPow, powers of two, popcount, clz, ctz, and rotations (clean-room port of Rust num-traits)

## 安装与导入

```bash
cap add num-traits
```

```cp
import { ntrtAbsI64ToU64 } from "num-traits";
```

## 核心导出 API

- `export fn ntrtAbsI64ToU64(x: i64): u64;`
- `export fn checkedAddI64(a: i64, b: i64): i64?;`
- `export fn checkedSubI64(a: i64, b: i64): i64?;`
- `export fn checkedMulI64(a: i64, b: i64): i64?;`
- `export fn checkedAddU64(a: u64, b: u64): u64?;`
- `export fn checkedSubU64(a: u64, b: u64): u64?;`
- `export fn checkedMulU64(a: u64, b: u64): u64?;`
- `export fn saturatingAddU64(a: u64, b: u64): u64;`
- `export fn saturatingSubU64(a: u64, b: u64): u64;`
- `export fn saturatingMulU64(a: u64, b: u64): u64;`
- `export fn saturatingAddI64(a: i64, b: i64): i64;`
- `export fn saturatingSubI64(a: i64, b: i64): i64;`
- `export fn clampI64(v: i64, lo: i64, hi: i64): i64;`
- `export fn clampF64(v: f64, lo: f64, hi: f64): f64;`
- `export fn signumI64(v: i64): i64;`
- `export fn absDiffU64(a: u64, b: u64): u64;`
- `export fn checkedPowU64(base: u64, exp: u32): u64?;`
- `export fn isPowerOfTwoU64(n: u64): bool;`
- `export fn nextPowerOfTwoU64(n: u64): u64?;`
- `export fn countOnesU64(n: u64): u32;`
- `export fn leadingZerosU64(n: u64): u32;`
- `export fn trailingZerosU64(n: u64): u32;`
- `export fn rotateLeftU64(n: u64, k: u32): u64;`
- `export fn rotateRightU64(n: u64, k: u32): u64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`num-traits@1.0.0`)，许可证：MIT OR Apache-2.0
