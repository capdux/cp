# cap:http_ratelimit_header

> 源码吸收自 `/data/dev/bun/src/http/http.cp`（IETF Draft `RateLimit` / `RateLimit-Policy` 规范），提供纯 CP、零外部依赖、零堆分配的限流响应头解析与构造库。

## 导入方式

```cp
import { RateLimitHeader, RateLimitPolicy } from "cap:http_ratelimit_header";
```

## 核心类型与 API

### `RateLimitHeader`

字段：`limit: u64?`、`remaining: u64?`、`reset_sec: u64?`、`policy_name: ([]const u8)?`。

- `RateLimitHeader.parse(header): RateLimitHeader?`：支持 IETF 结构化格式（`"default";r=50;t=30`）与字典格式（`limit=100, remaining=50, reset=30`），拒绝空串与全无效字段。
- `format(out): ([]const u8)?`：规范化输出两种格式。

### `RateLimitPolicy`

字段：`quota: u64`、`window_sec: u64`、`policy_name: ([]const u8)?`、`comment: ([]const u8)?`。

- `RateLimitPolicy.parse(header): RateLimitPolicy?`：支持 `100;w=60` 与 `"burst";q=100;w=60;comment="sliding"`，强制校验 `window_sec > 0`。
- `format(out): ([]const u8)?`：规范化输出。

## 使用示例

```cp
import { RateLimitHeader, RateLimitPolicy } from "cap:http_ratelimit_header";

fn example() {
    let buf: [128]u8 = undefined;
    if (rl := RateLimitHeader.parse("\"default\";r=50;t=30")) {
        * = rl.format(&buf);
    }
    if (pol := RateLimitPolicy.parse("\"burst\";q=100;w=60")) {
        * = pol.format(&buf);
    }
}
```
