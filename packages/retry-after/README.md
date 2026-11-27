[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `@cap/retry-after`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:retry-after` ([https://github.com/jwilm/retry-after](https://github.com/jwilm/retry-after))

Zero-heap RFC 9110 HTTP `Retry-After` header parser and formatter in pure CP. Supports both `delay-seconds` (e.g. `"120"`) and IMF-fixdate `HTTP-date` (e.g. `"Wed, 21 Oct 2015 07:28:00 GMT"`), with deterministic wait-duration calculation for HTTP `429 Too Many Requests` and `503 Service Unavailable`.

零堆分配纯 CP RFC 9110 `Retry-After` 响应头解析与格式化器，支持延迟秒数与 IMF-fixdate HTTP 日期双格式，适用于 API 429/503 限流退避与网关重试。

## Installation

```bash
cap add @cap/retry-after
```

## API

| Function | Description |
| --- | --- |
| `parseRetryAfter(header)` | Parse `delay-seconds` or `HTTP-date` into `RetryAfter` |
| `waitSeconds(ra, now_epoch_sec)` | Compute non-negative wait duration in seconds (`-1` if invalid) |
| `parseHttpDate(date_str)` | Parse RFC 9110 IMF-fixdate into Unix epoch seconds |
| `formatDelaySeconds(seconds, dst)` | Format `delay-seconds` header value into buffer |

## License

MIT
