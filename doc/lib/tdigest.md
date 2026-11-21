# `cap:tdigest` — 纯 CP 流式 T-Digest 分位数估算引擎

## 概述

`cap:tdigest`（亦支持 `cap.tdigest` 动态挂载）实现 Dunning T-Digest 流式分位数估算器，在极低内存占用的同时对 p50 / p95 / p99 / p99.9 等尾部分位数提供高精度估计，并支持分布式节点 `merge` 聚合。

## 核心类型与 API

- `TDigestError`: `EmptyDigest`、`InvalidCompression`、`InvalidQuantile`
- `Centroid`: `{ mean: f64, weight: f64 }`
- `TDigest.init(alloc, compression)` / `deinit(&self)`
- `observe(&self, value)` / `add(&self, value, weight)` / `compress(&self)`
- `quantile(&self, q) -> !f64` / `merge(&self, other)` / `count(self)`

## 示例

```cp
import { TDigest } from "cap:tdigest";

let td = try TDigest.init(alloc, 100.0);
defer td.deinit();
try td.observe(42.0);
const p99 = try td.quantile(0.99);
```
