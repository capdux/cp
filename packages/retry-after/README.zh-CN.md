[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# retry-after

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap RFC 9110 HTTP Retry-After header parser and formatter (delay-seconds & HTTP-date) in pure CP

## 安装与导入

```bash
cap add retry-after
```

```cp
import { parseUint } from "retry-after";
```

## 核心导出 API

- `export fn parseUint(s: &[]u8): i64;`
- `export fn parseMonth(s: &[]u8): i32;`
- `export fn isLeapYear(y: i64): bool;`
- `export fn daysInMonth(y: i64, m: i32): i32;`
- `export fn daysFromCivil(y_in: i64, m_in: i64, d: i64): i64;`
- `export fn parseHttpDate(s: &[]u8): i64;`
- `export fn parseRetryAfter(header: &[]u8): RetryAfter;`
- `export fn waitSeconds(ra: RetryAfter, now_epoch_sec: i64): i64;`
- `export fn formatDelaySeconds(seconds: u64, dst: []u8): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`retry-after@1.0.0`)，许可证：MIT
