[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:pure-rand` ([https://github.com/dubzzz/pure-rand](https://github.com/dubzzz/pure-rand)) | 许可：`MIT`

# `pure-rand`

零堆分配纯 CP 不可变函数式伪随机数生成器（`Xoroshiro128Plus` + `SplitMix64` 播种 + $2^{64}$ `jump()` + 无偏拒绝采样 `uniformIntDistribution`），对标 npm **#443** `pure-rand`（真实周下载 **1.24 亿/周**，`fast-check` 底层核心）。

## 核心 API

- `Xoroshiro128Plus.fromSeed(seed)` / `.nextU64()` / `.nextI32()` / `.jump()`
- `uniformIntDistribution(min, max, rng): NextIntRangeResult`
