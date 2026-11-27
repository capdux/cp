[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# fastrand

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:fastrand` ([https://github.com/smol-rs/fastrand](https://github.com/smol-rs/fastrand))

Zero-allocation WyRand 64-bit PRNG for CP — clean-room MIT parity with Rust `fastrand`.

零堆分配的 CP 超高速 WyRand 64-bit 伪随机数引擎，支持确定性种子、独立子流 `fork`、拒绝采样无偏区间、53-bit 精度 `[0.0, 1.0)` 浮点、字节填充、Fisher-Yates 原地洗牌与随机字母数字串生成（纯 CP 洁净室 MIT 实现）。

## Install / 安装

```bash
cap add fastrand
```

## API Reference / 接口一览

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `Rng.withSeed` | `(seed: u64) -> Rng` | 使用确定性 64-bit 种子构造 PRNG |
| `fork` | `(*_) -> Rng` | 派生独立随机子流 |
| `nextU64` / `nextU32` / `nextBool` | `(*_) -> u64 / u32 / bool` | WyRand 核心均匀随机原语 |
| `nextChance` | `(*_, numerator: u32, denominator: u32) -> bool` | 按 `numerator / denominator` 概率返回 `true` |
| `u64Range` / `i64Range` / `usizeRange` | 无偏区间采样 | 拒绝采样消除模偏差（`i64Range` 支持全 `i64` 范围含负数） |
| `f64Unit` | `(*_) -> f64` | 53-bit 精度均匀浮点，严格落在 `[0.0, 1.0)` |
| `fillBytes` | `(*_, buf: []u8)` | 原地填充随机字节 |
| `shuffleU64` | `(*_, slice: []u64)` | Fisher-Yates 原地无偏洗牌 |
| `alphanumeric` | `(*_, out: []u8) -> string` | 生成指定长度随机 ASCII 字母数字串 |

## Usage / 使用示例

```cp
import { Rng } from "fastrand";

let rng = Rng.withSeed(42);
const n = rng.u64Range(10, 100);
const p = rng.f64Unit();

let token_buf: [16]u8 = undefined;
const token = rng.alphanumeric(token_buf[0..16]);
```

## License

MIT
