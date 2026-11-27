[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# oorandom

零堆分配的确定性 PCG32（`Rand32`）与双流 PCG64（`Rand64`）伪随机数生成器，支持有符号/无符号区间采样、浮点生成、字节填充、状态快照恢复与 O(log N) 快速前跳。参考 Rust `oorandom` 设计。

## 源包溯源与零成本迁移

| 项目 | 说明 |
| :--- | :--- |
| **源平台** | crates.io |
| **上游原包** | `oorandom` (`~icefox/oorandom`) |
| **CP 包名** | `oorandom` (`v1.0.0`) |
| **许可证** | MIT |

CP 生态保持原包名 `oorandom` 不变，无需 `@cap/*` 前缀或改名：

```rust
// Rust (crates.io: oorandom)
use oorandom::Rand32;
let mut rng = Rand32::new(42);
let roll = rng.rand_range(1..7);
```

```typescript
// CP（保持原包名 "oorandom"）
import { Rand32 } from "oorandom";
let mut rng = Rand32.new(42);
let roll = rng.randRange(1, 7);
```

## 安装

```bash
cap add oorandom
```

## 快速上手与实战示例

### 1. `Rand32` — 32 位确定性 PRNG

```typescript
import { Rand32 } from "oorandom";

fn demoRand32(): void {
    let mut rng = Rand32.new(42);
    let mut with_stream = Rand32.newInc(42, 7);

    let u = rng.randU32();
    let i = rng.randI32();
    let f = rng.randFloat();              // f32，区间 [0.0, 1.0)
    let b = rng.randBool();
    let dice = rng.randRange(1, 7);       // u32，半开区间 [1, 7)
    let temp = rng.randRangeI32(-10, 11); // i32，半开区间 [-10, 11)

    let mut buf: [16]u8 = undefined;
    rng.fillBytes(buf[0..16]);

    // 状态快照与精确恢复
    let pair = rng.statePair();           // [st, inc]
    let mut restored = Rand32.fromState(pair[0], pair[1]);

    // O(log N) 快速前跳
    with_stream.advance(5000);
}
```

### 2. `Rand64` — 双流 64 位 PRNG

```typescript
import { Rand64 } from "oorandom";

fn demoRand64(): void {
    let mut rng = Rand64.new(123456789);
    let mut custom = Rand64.newInc(1, 2, 3, 4);

    let u = rng.randU64();
    let i = rng.randI64();
    let f = rng.randFloat();              // f64，区间 [0.0, 1.0)
    let b = rng.randBool();
    let slot = rng.randRange(100, 200);   // u64，[100, 200)
    let delta = rng.randRangeI64(-50, 50);

    let mut buf: [32]u8 = undefined;
    rng.fillBytes(buf[0..32]);

    rng.advance(100000);
    let st = rng.state(); // [lo.st, hi.st]
}
```

### 3. 底层 PCG 状态原语

```typescript
import { oorStep, oorOutput, oorAdvanceState } from "oorandom";

fn manualStep(): u32 {
    let st0: u64 = 12345;
    let inc: u64 = 1442695040888963407;
    let st1 = oorStep(st0, inc);
    let st100 = oorAdvanceState(st0, inc, 100);
    return oorOutput(st100);
}
```

## API 速览

### `Rand32`
- `Rand32.new(seed)` / `Rand32.newInc(seed, inc)` / `Rand32.fromState(st, inc)`
- `randU32(): u32`、`randI32(): i32`、`randFloat(): f32`、`randBool(): bool`
- `randRange(start: u32, end: u32): u32`、`randRangeI32(start: i32, end: i32): i32`
- `fillBytes(dst: []u8)`、`advance(delta: u64)`、`state(): u64`、`statePair(): [2]u64`

### `Rand64`
- `Rand64.new(seed)` / `Rand64.newInc(seed_lo, seed_hi, inc_lo, inc_hi)`
- `randU64(): u64`、`randI64(): i64`、`randFloat(): f64`、`randBool(): bool`
- `randRange(start: u64, end: u64): u64`、`randRangeI64(start: i64, end: i64): i64`
- `fillBytes(dst: []u8)`、`advance(delta: u64)`、`state(): [2]u64`

### 底层原语
- `oorStep(st, inc): u64`、`oorOutput(oldstate): u32`、`oorAdvanceState(st_in, inc, delta_in): u64`
