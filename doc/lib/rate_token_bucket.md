# `cap:rate_token_bucket` — 纯 CP 令牌桶与 GCRA 精确限流引擎

## 概述

`cap:rate_token_bucket`（亦支持 `cap.rate_token_bucket` 动态挂载）提供零堆分配、纳秒级时间注入的 `TokenBucket`（支持突发容量 `burst`、非均匀消费 `cost` 与 `retryAfterNs` 退避计算）和 `GcraLimiter`（Generic Cell Rate Algorithm，单变量 TAT 实现零漂移等效漏桶限流）。

## 核心类型与 API

- `RateLimitError`: `InvalidRate`、`InvalidBurst`、`InvalidCost`
- `TokenBucket`: `init(rate_per_sec, burst, now_ns)`、`allow(&self, cost, now_ns)`、`available(&self, now_ns)`、`retryAfterNs(&self, cost, now_ns)`
- `GcraLimiter`: `init(rate_per_sec, burst)`、`allow(&self, now_ns) -> bool`

## 示例

```cp
import { TokenBucket } from "cap:rate_token_bucket";

let tb = try TokenBucket.init(10.0, 5.0, 0);
const ok = try tb.allow(1.0, 100_000_000);
```
