<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **crates.io** [`governor`](https://github.com/boinkor-net/governor)（`MIT`）（包名 `@cap/governor`）。

# @cap/governor

Zero-allocation deterministic GCRA (Generic Cell Rate Algorithm) and Token Bucket rate limiters for CP — clean-room MIT parity with Rust `governor` and npm `rate-limiter-flexible`.

零堆分配、零系统时钟依赖的 CP 确定性限流引擎，内置单字段状态 GCRA（`GcraLimiter`，支持按权重/Token 批量扣费与精确 `retry_after_ms`）与经典浮点令牌桶（`TokenBucket`），适用于 API 网关、多租户配额与高并发 Agent 速率控制（纯 CP 洁净室 MIT 实现）。

## Install / 安装

```bash
cap add @cap/governor
```

## API Reference / 接口一览

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `RateDecision` | `pub struct { allowed: bool, remaining, retry_after_ms, reset_after_ms: u64 }` | 限流判定结果（可直接映射 HTTP `Retry-After` / `X-RateLimit-Remaining`） |
| `GcraLimiter` | `init(rate_per_sec, burst)`, `perPeriod(limit, period_ms, burst)`, `checkAt(*_, now_ms)`, `checkNAt(*_, cost, now_ms)`, `reset(*_)` | 仅占 1 个 `tat_ms` 状态的 GCRA 限流器 |
| `TokenBucket` | `init(capacity, refill_per_sec, now_ms)`, `tryAcquire(*_, tokens, now_ms)`, `available(*_, now_ms)`, `reset(*_, now_ms)` | 支持小数速率与突发容量的经典令牌桶 |

## Usage / 使用示例

```cp
import { GcraLimiter, TokenBucket } from "@cap/governor";

// 10 req/s with burst of 3
let lim = GcraLimiter.init(10, 3);
const d1 = lim.checkAt(1000); // allowed=true, remaining=2
const d4 = lim.checkNAt(3, 1000); // allowed=false, retry_after_ms=100

let tb = TokenBucket.init(5.0, 2.0, 0);
const ok = tb.tryAcquire(2.5, 0); // allowed=true
```

## License

MIT
