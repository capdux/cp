[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# rand_xoshiro

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation Blackman & Vigna xoshiro256** PRNG with SplitMix64 seeding, unbiased nextRange, 2^128 jump, and Fisher-Yates shuffle in pure CP

## 安装与导入

```bash
cap add rand_xoshiro
```

```cp
import { makeU64 } from "rand_xoshiro";
```

## 核心导出 API

- `export fn makeU64(hi: u32, lo: u32): u64;`
- `export fn rotl7(x: u64): u64;`
- `export fn rotl45(x: u64): u64;`
- `export fn splitmix64Next(state: &mut u64): u64;`
- `export fn seedFromU64(seed: u64): Xoshiro256;`
- `export fn nextU64(rng: &mut Xoshiro256): u64;`
- `export fn nextF64(rng: &mut Xoshiro256): f64;`
- `export fn nextRange(rng: &mut Xoshiro256, upper_exclusive: u64): u64;`
- `export fn jump(rng: &mut Xoshiro256);`
- `export fn shuffleU32(rng: &mut Xoshiro256, data: []u32);`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`rand_xoshiro@1.0.0`)，许可证：MIT
