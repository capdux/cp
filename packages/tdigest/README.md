[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# tdigest

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:tdigest` ([https://github.com/MnO2/t-digest](https://github.com/MnO2/t-digest))

Zero-allocation Merging T-Digest streaming quantile (`P50`/`P95`/`P99`/`P99.9`) and CDF estimator with distributed `merge` for CP — clean-room MIT parity with Ted Dunning's T-Digest algorithm and Rust `tdigest`.

零堆分配的 CP 流式分位数估算引擎（固定质心池 `MAX_CENTROIDS = 128`），在恒定栈内存下提供高精度 P95 / P99 / P99.9 尾部延迟估算、累积分布函数 `cdf` 与跨节点分布式 `merge`（纯 CP 洁净室 MIT 实现）。

## Install / 安装

```bash
cap add tdigest
```

## API Reference / 接口一览

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `Centroid` | `pub struct { mean: f64, weight: f64 }` | 加权质心 |
| `TDigest.init` | `(comp: f64) -> TDigest` | 初始化 T-Digest（默认压缩因子 `50.0`） |
| `add` / `addWeighted` | `(*_, value, ...)` | 添加单样本或加权样本（池满自动压缩） |
| `compress` | `(*_)` | 排序并按 `4 * N * q * (1 - q) / compression` 合并质心 |
| `quantile` / `cdf` | `(*_, q / x: f64) -> f64` | 线性插值估算分位数与累积分布函数 `P(X <= x)` |
| `merge` | `(*_, other: *const TDigest)` | 分布式合并另一节点的 T-Digest |
| `count` / `min` / `max` / `centroidCount` / `isEmpty` / `reset` | 统计与重置 | 精确样本总数与极值追踪 |

## License

MIT
