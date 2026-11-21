# cap:retry_backoff

> 源码吸收自 `/data/dev/bun/src/http/HTTPContext.cp`（指数退避与 RFC 9110 §10.2.3 `Retry-After` 规范），提供纯 CP、零外部依赖、零堆分配的重试调度与 `Retry-After` 解析库。

## 导入方式

```cp
import {
    JitterMode,
    BackoffConfig,
    BackoffState,
    RetryAfter,
    parseRetryAfter,
    isRetryableHttpStatus,
} from "cap:retry_backoff";
```

## 核心类型与 API

### `JitterMode`

- `none`：确定性纯指数退避 `min(base * 2^attempt, max)`
- `full`：全抖动 `[0, cap]`
- `equal`：半抖动 `[cap/2, cap]`

### `BackoffConfig`

字段：`base_delay_ms: u64`、`max_delay_ms: u64`、`max_attempts: u32`、`jitter: JitterMode`。
- `BackoffConfig.defaultConfig(): BackoffConfig`（`100ms` 起步、`30_000ms` 封顶、`5` 次重试、`full` 抖动）。

### `BackoffState`

内置确定性 SplitMix64 PRNG，防止移位与乘法溢出。
- `BackoffState.init(seed: u64): BackoffState`
- `nextDelayMs(*_, cfg: BackoffConfig): u64?`：返回下一次等待毫秒数；超过 `max_attempts` 返回 `null`。
- `reset(*_)`：重置重试计数。
- `isExhausted(cfg: BackoffConfig): bool`：是否已耗尽重试配额。

### `RetryAfter` & HTTP 辅助

- `parseRetryAfter(header: []const u8, now_epoch_sec: i64): RetryAfter?`：同时支持十进制秒数（如 `"120"`）与 RFC 9110 IMF-fixdate（如 `"Wed, 21 Oct 2015 07:28:00 GMT"`），过去时间安全钳制为 `0` 秒。
- `isRetryableHttpStatus(status: u16): bool`：`408`、`425`、`429`、`500`、`502`、`503`、`504` 返回 `true`。

## 使用示例

```cp
import { BackoffConfig, BackoffState, parseRetryAfter, isRetryableHttpStatus } from "cap:retry_backoff";

fn example() {
    const cfg = BackoffConfig.defaultConfig();
    let state = BackoffState.init(42);
    if (isRetryableHttpStatus(503)) {
        if (ra := parseRetryAfter("120", 1700000000)) {
            * = ra.delay_secs;
        }
        * = state.nextDelayMs(cfg);
    }
}
```
