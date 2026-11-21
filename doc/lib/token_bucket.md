# cap:token_bucket

> 源码吸收自 `/data/dev/bun/src/http/HTTPContext.cp`（背压控制与标准令牌桶/固定窗口限流算法），提供纯 CP、零外部依赖、零堆分配的纯整数确定性限流器库。

## 导入方式

```cp
import { TokenBucket, FixedWindowLimiter } from "cap:token_bucket";
```

## 核心类型与 API

### `TokenBucket`

内部使用毫令牌（`tokens * 1000`）纯整数运算，保证亚秒级补充零精度损失，且防时钟回拨与乘法溢出。

| 方法 | 签名 | 说明 |
| --- | --- | --- |
| `init` | `fn init(capacity: u64, refill_per_sec: u64, now_ms: u64): TokenBucket` | 创建满桶令牌桶 |
| `initWithTokens` | `fn initWithTokens(capacity: u64, initial_tokens: u64, refill_per_sec: u64, now_ms: u64): TokenBucket` | 指定初始令牌数（自动钳制至容量） |
| `refill` | `fn refill(*_, now_ms: u64)` | 补充令牌并封顶至容量 |
| `available` | `fn available(*_, now_ms: u64): u64` | 返回当前可用整数令牌数 |
| `tryConsume` | `fn tryConsume(*_, tokens: u64, now_ms: u64): bool` | 尝试原子扣减令牌 |
| `timeUntilReadyMs` | `fn timeUntilReadyMs(*_, tokens: u64, now_ms: u64): u64?` | 计算等待毫秒数；永不可满足返回 `null` |

### `FixedWindowLimiter`

| 方法 | 签名 | 说明 |
| --- | --- | --- |
| `init` | `fn init(limit: u64, window_ms: u64, now_ms: u64): FixedWindowLimiter` | 创建固定窗口限流器 |
| `allow` | `fn allow(*_, now_ms: u64): bool` | 尝试放行 1 次请求（自动滚动过期窗口） |
| `remaining` | `fn remaining(now_ms: u64): u64` | 查询当前窗口剩余配额 |
| `resetAfterMs` | `fn resetAfterMs(now_ms: u64): u64` | 查询距离下次窗口重置的毫秒数 |

## 使用示例

```cp
import { TokenBucket, FixedWindowLimiter } from "cap:token_bucket";

fn example() {
    let bucket = TokenBucket.init(10, 5, 1000);
    * = bucket.tryConsume(4, 1000);       // true（剩 6）
    * = bucket.timeUntilReadyMs(8, 1000); // 400ms

    let win = FixedWindowLimiter.init(100, 60_000, 1000);
    * = win.allow(1000);
    * = win.remaining(1000); // 99
}
```
