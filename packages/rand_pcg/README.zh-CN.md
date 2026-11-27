[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# rand_pcg

零堆分配的 PCG32（XSH-RR）与 PCG64Mcg（128 位 MCG）伪随机数生成器，支持独立随机流、无偏区间采样、字节填充以及 O(log N) 快速前跳（`advance`）。移植自 Rust `rand_pcg`。

## 源包溯源与零成本迁移

| 项目 | 说明 |
| :--- | :--- |
| **源平台** | crates.io |
| **上游原包** | `rand_pcg` (`rust-random/rand`) |
| **CP 包名** | `rand_pcg` (`v1.0.0`) |
| **许可证** | MIT |

CP 生态沿用原包名 `rand_pcg`，无需 `@cap/*` 前缀或改名：

```rust
// Rust (crates.io: rand_pcg)
use rand_pcg::{Pcg32, Pcg64Mcg};
use rand_core::{RngCore, SeedableRng};
let mut rng = Pcg32::seed_from_u64(42);
let v = rng.next_u32();
```

```typescript
// CP（保持原包名 "rand_pcg"）
import { Pcg32, Pcg64Mcg } from "rand_pcg";
let mut rng = Pcg32.seedFromU64(42);
let v = rng.nextU32();
```

## 安装

```bash
cap add rand_pcg
```

## 快速上手与实战示例

### 1. `Pcg32` — 独立流、无偏范围采样、浮点与快速前跳

```typescript
import { Pcg32 } from "rand_pcg";

fn runPcg32(): void {
    // 3 种构造方式：
    let mut r1 = Pcg32.seedFromU64(42);       // SplitMix64 扩展种子
    let mut r2 = Pcg32.init(12345, 1);        // (seed, stream_id)
    let mut r3 = Pcg32.new(0x1234, 0x5678);   // (raw state, raw stream)

    let u32_val = r1.nextU32();
    let u64_val = r1.nextU64();
    let die_roll = r1.nextBoundedU32(6);      // 无偏 [0, 6)
    let f32_val = r1.nextF32();               // [0.0, 1.0)
    let f64_val = r1.nextF64();               // [0.0, 1.0)
    let coin = r1.nextBool();

    let mut bytes: [16]u8 = undefined;
    r1.fillBytes(bytes[0..16]);

    // O(log N) 快速前跳，适合并行蒙特卡洛仿真切片
    r2.advance(1000000);
}
```

### 2. `Pcg64Mcg` — 128 位状态高性能 64 位生成器

```typescript
import { Pcg64Mcg } from "rand_pcg";

fn runPcg64Mcg(): void {
    let mut rng = Pcg64Mcg.seedFromU64(2026);
    let mut manual = Pcg64Mcg.new(0, 1);

    let x = rng.nextU64();
    let y = rng.nextU32();
    let p = rng.nextF64(); // [0.0, 1.0)

    let mut buf: [32]u8 = undefined;
    rng.fillBytes(buf[0..32]);

    // 128 位状态空间 O(log N) 步进
    rng.advance(10000);
}
```

### 3. 底层位运算与 128 位算术原语

```typescript
import {
    wrapAdd64,
    wrapMul64,
    mul64Full,
    wrapMul128,
    rotr32,
    rotr64,
    splitmix64Next,
} from "rand_pcg";

fn lowLevelHelpers(): void {
    let mixed = splitmix64Next(42);
    let prod = mul64Full(0x100000000, 0x100000000); // U128Pair { hi, lo }
    let p128 = wrapMul128(prod.hi, prod.lo, 0, 3);
    let r = rotr32(0x12345678, 8);
    let r64 = rotr64(mixed, 13);
    let s = wrapAdd64(wrapMul64(mixed, 6364136223846793005), 1);
}
```

## API 速览

### `Pcg32`
- `Pcg32.init(seed: u64, stream_id: u64)` / `Pcg32.new(state: u64, stream: u64)` / `Pcg32.seedFromU64(seed: u64)`
- `nextU32(): u32`、`nextU64(): u64`、`nextBoundedU32(bound: u32): u32`
- `nextF32(): f32`、`nextF64(): f64`、`nextBool(): bool`
- `fillBytes(dst: []u8)`、`advance(delta_in: u64)`

### `Pcg64Mcg`
- `Pcg64Mcg.new(state_hi: u64, state_lo: u64)` / `Pcg64Mcg.seedFromU64(seed: u64)`
- `nextU64(): u64`、`nextU32(): u32`、`nextF64(): f64`
- `fillBytes(dst: []u8)`、`advance(delta_in: u64)`

### 底层辅助函数
- `U128Pair { hi: u64, lo: u64 }`
- `wrapAdd64`、`wrapMul64`、`mul64Full`、`wrapMul128`、`rotr32`、`rotr64`、`splitmix64Next`
