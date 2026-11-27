[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# rand_pcg

纯 CP 实现的零堆确定性伪随机数生成器，提供 `Pcg32`（PCG-XSH-RR 64/32）与 `Pcg64Mcg`（128 位 MCG PCG-XSL-RR），支持多独立随机流与 $O(\log N)$ 状态快进。

- **源包对齐**：参考 Rust [`rand_pcg`](https://github.com/rust-random/rand)（`crates.io`，`MIT`）。
- **包版本**：`1.0.0`

## 安装与导入

```bash
cap add rand_pcg
```

在 `cap.json` 中声明依赖：

```json
{
  "dependencies": {
    "rand_pcg": "^1.0.0"
  }
}
```

```typescript
import { Pcg32, Pcg64Mcg, U128Pair, splitmix64Next } from "rand_pcg";
```

## 核心 API 参考

### `class Pcg32`（PCG-XSH-RR 64/32）

字段：`state: u64`、`inc: u64`。

| 方法 | 签名 | 说明 |
| :--- | :--- | :--- |
| `init` | `Pcg32.init(seed: u64, stream_id: u64): Pcg32` | 标准 PCG32 种子与流编号初始化 |
| `new` | `Pcg32.new(state: u64, stream: u64): Pcg32` | 对齐 Rust `Pcg32::new(state, stream)` 的构造方法 |
| `seedFromU64` | `Pcg32.seedFromU64(seed: u64): Pcg32` | 经 `SplitMix64` 扩展单 `u64` 种子初始化 |
| `nextU32` / `nextU64` | `nextU32(*_): u32` / `nextU64(*_): u64` | 生成均匀分布的 32 位 / 64 位无符号整数 |
| `nextBoundedU32` | `nextBoundedU32(*_, bound: u32): u32` | 无偏拒绝采样，返回 `[0, bound)` 整数 |
| `nextF32` / `nextF64` | `nextF32(*_): f32` / `nextF64(*_): f64` | 生成 `[0.0, 1.0)` 均匀分布浮点数 |
| `nextBool` | `nextBool(*_): bool` | 生成均匀随机布尔值 |
| `fillBytes` | `fillBytes(*_, dst: []u8)` | 向调用方缓冲区 `dst` 填充随机字节 |
| `advance` | `advance(*_, delta_in: u64)` | 以 $O(\log N)$ 复杂度向前跳过 `delta_in` 步 |

### `class Pcg64Mcg`（128 位 MCG PCG-XSL-RR）

字段：`state_hi: u64`、`state_lo: u64`。

| 方法 | 签名 | 说明 |
| :--- | :--- | :--- |
| `new` | `Pcg64Mcg.new(state_hi: u64, state_lo: u64): Pcg64Mcg` | 由 128 位状态对构造（自动保证奇数状态） |
| `seedFromU64` | `Pcg64Mcg.seedFromU64(seed: u64): Pcg64Mcg` | 通过 `SplitMix64` 从单个 `u64` 初始化 128 位状态 |
| `nextU64` / `nextU32` | `nextU64(*_): u64` / `nextU32(*_): u32` | 生成 64 位 / 32 位无符号整数 |
| `nextF64` | `nextF64(*_): f64` | 生成 `[0.0, 1.0)` 均匀分布 `f64` |
| `fillBytes` | `fillBytes(*_, dst: []u8)` | 填充随机字节至 `dst` |
| `advance` | `advance(*_, delta_in: u64)` | $O(\log N)$ 步进快进 128 位 MCG 状态 |

### 底层位运算辅助函数

同时导出：`U128Pair`、`wrapAdd64`、`wrapMul64`、`mul64Full`、`wrapMul128`、`rotr32`、`rotr64`、`splitmix64Next`。

## 使用示例

### 1. 确定性蒙特卡洛采样

```typescript
import { Pcg32 } from "rand_pcg";

fn monteCarlo(seed: u64): f64 {
    let mut rng = Pcg32.new(seed, 0xa02bdbf7bb3c0a7);
    let mut hits: u64 = 0;
    let total: u64 = 1000;
    let mut i: u64 = 0;
    while (i < total) {
        let x = rng.nextF64();
        let y = rng.nextF64();
        if (x * x + y * y <= 1.0) { hits += 1; }
        i += 1;
    }
    return 4.0 * (hits as f64) / (total as f64);
}
```

### 2. 使用 `Pcg64Mcg` 跳转流并填充字节缓冲

```typescript
import { Pcg64Mcg } from "rand_pcg";

fn fillNonce(seed: u64, out: []u8) {
    let mut rng = Pcg64Mcg.seedFromU64(seed);
    rng.advance(1024);
    rng.fillBytes(out);
}
```

## 与源包差异与 CP 安全亮点

- **极简栈上状态**：`Pcg32` 与 `Pcg64Mcg` 均仅占 16 字节栈内存，零依赖、零堆分配。
- **$O(\log N)$ 并行子流切分**：通过 `advance` 可为多个并发 Actor 分配互不重叠的确定性随机区间。
- **非密码学用途**：与源包一致，适用于仿真、游戏、采样与随机化算法，不可用于密钥或安全令牌生成。
