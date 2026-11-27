[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# num-integer

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation integer number theory, overflow-safe GCD/LCM/extended-GCD, floor/ceil division, isqrt/icbrt, and binomial coefficients for CP.

## 安装与导入

```bash
cap add num-integer
```

```cp
import { nintAbsU64 } from "num-integer";
```

## 核心导出 API

- `export fn nintAbsU64(v: i64): u64;`
- `export fn nintDivTruncRem(a: i64, b: i64, q_out: &mut i64, r_out: &mut i64);`
- `export fn gcdU64(a: u64, b: u64): u64;`
- `export fn lcmU64(a: u64, b: u64): u64;`
- `export fn extendedGcd(a: i64, b: i64): ExtGcdResult;`
- `export fn divFloor(a: i64, b: i64): i64;`
- `export fn modFloor(a: i64, b: i64): i64;`
- `export fn divCeil(a: i64, b: i64): i64;`
- `export fn isqrtU64(n: u64): u64;`
- `export fn icbrtU64(n: u64): u64;`
- `export fn binomialU64(n: u64, k: u64): u64;`
- `export fn isEven(n: i64): bool;`
- `export fn isOdd(n: i64): bool;`
- `export fn nextMultipleOfU64(n: u64, step: u64): u64;`
- `export fn prevMultipleOfU64(n: u64, step: u64): u64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`num-integer@1.0.0`)，许可证：MIT OR Apache-2.0
