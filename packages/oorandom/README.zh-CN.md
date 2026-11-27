[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# oorandom

纯 CP 实现的零堆极简确定性伪随机数库，提供 `Rand32`（PCG32 XSH-RR 64/32）与双流 `Rand64`，支持状态快照回放、无偏区间采样与 $O(\log N)$ 快进。

- **源包对齐**：参考 Rust [`oorandom`](https://sr.ht/~icefox/oorandom/)（`crates.io`，`MIT`）。
- **包版本**：`1.0.0`

## 安装与导入

```bash
cap add oorandom
```

在 `cap.json` 中声明依赖：

```json
{
  "dependencies": {
    "oorandom": "^1.0.0"
  }
}
```

```typescript
import { Rand32, Rand64, oorStep, oorOutput, oorAdvanceState } from "oorandom";
```

## 核心 API 参考

### `class Rand32`（PCG32 XSH-RR 64/32）

字段：`st: u64`、`inc: u64`。

| 方法 | 签名 | 说明 |
| :--- | :--- | :--- |
| `new` | `Rand32.new(seed: u64): Rand32` | 使用默认流增量初始化 |
| `newInc` | `Rand32.newInc(seed: u64, inc: u64): Rand32` | 指定种子与流增量 `inc` 初始化 |
| `fromState` | `Rand32.fromState(st: u64, inc: u64): Rand32` | 从快照 `(st, inc)` 精确恢复生成器状态 |
| `randU32` / `randI32` | `randU32(*_): u32` / `randI32(*_): i32` | 生成 32 位无符号 / 有符号均匀随机整数 |
| `randFloat` | `randFloat(*_): f32` | 生成 `[0.0, 1.0)` 均匀分布 `f32` |
| `randBool` | `randBool(*_): bool` | 生成均匀随机布尔值 |
| `randRange` / `randRangeI32` | `randRange(*_, start: u32, end: u32): u32` / `randRangeI32(*_, start: i32, end: i32): i32` | 无偏半开区间 `[start, end)` 随机整数 |
| `fillBytes` | `fillBytes(*_, dst: []u8)` | 向调用方缓冲区 `dst` 填充随机字节 |
| `advance` | `advance(*_, delta: u64)` | $O(\log N)$ 快进 `delta` 步 |
| `state` / `statePair` | `state(): u64` / `statePair(): [2]u64` | 获取状态快照 `st` 或 `[st, inc]` |

### `class Rand64`（双流 PCG64）

字段：`lo: Rand32`、`hi: Rand32`。

| 方法 | 签名 | 说明 |
| :--- | :--- | :--- |
| `new` | `Rand64.new(seed: u64): Rand64` | 使用单个 `u64` 种子初始化 64 位生成器 |
| `newInc` | `Rand64.newInc(seed_lo: u64, seed_hi: u64, inc_lo: u64, inc_hi: u64): Rand64` | 指定双流种子与增量初始化 |
| `randU64` / `randI64` | `randU64(*_): u64` / `randI64(*_): i64` | 生成 64 位无符号 / 有符号均匀随机整数 |
| `randFloat` | `randFloat(*_): f64` | 生成 `[0.0, 1.0)` 均匀分布 `f64` |
| `randBool` | `randBool(*_): bool` | 生成均匀随机布尔值 |
| `randRange` / `randRangeI64` | `randRange(*_, start: u64, end: u64): u64` / `randRangeI64(*_, start: i64, end: i64): i64` | 无偏半开 64 位区间 `[start, end)` |
| `fillBytes` | `fillBytes(*_, dst: []u8)` | 填充随机字节至 `dst` |
| `advance` | `advance(*_, delta: u64)` | 双流同步 $O(\log N)$ 快进 `delta` 步 |
| `state` | `state(): [2]u64` | 获取双流状态快照 `[lo.st, hi.st]` |

### 底层函数

`oorStep(st: u64, inc: u64): u64`、`oorOutput(oldstate: u64): u32`、`oorAdvanceState(st_in: u64, inc: u64, delta_in: u64): u64`。

## 使用示例

### 1. 游戏确定性掷骰与状态回放

```typescript
import { Rand32 } from "oorandom";

fn rollAndReplay(): bool {
    let mut rng = Rand32.new(12345);
    let snap = rng.statePair();
    let d20 = rng.randRange(1, 21);

    let mut replay = Rand32.fromState(snap[0], snap[1]);
    return replay.randRange(1, 21) == d20;
}
```

### 2. 64 位有符号区间采样与字节填充

```typescript
import { Rand64 } from "oorandom";

fn sampleWorld(seed: u64, out: []u8): i64 {
    let mut rng = Rand64.new(seed);
    rng.fillBytes(out);
    return rng.randRangeI64(-1000, 1000);
}
```

## 与源包差异与 CP 安全亮点

- **零依赖极简设计**：延续 `oorandom` 小巧自包含理念，无复杂 trait 依赖、无堆分配。
- **内置回放与快进**：原生提供 `fromState`、`statePair`、`fillBytes` 与 `advance`，方便帧同步与并行仿真。
- **非密码学用途**：仅用于仿真、程序化生成与游戏逻辑，不可用于安全密钥生成。
