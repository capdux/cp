[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# rfc3339

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap strict RFC 3339 timestamp parser, Unix epoch converter, and UTC formatter (reuses @cap/iso8601)

## 安装与导入

```bash
cap add rfc3339
```

```cp
import { rfcIsDigit } from "rfc3339";
```

## 核心导出 API

- `export fn rfcIsDigit(c: u8): bool;`
- `export fn daysFromCivil(year: i32, month: u8, day: u8): i64;`
- `export fn civilFromDays(z_in: i64): IsoDate;`
- `export fn isValidRfc3339(s: string): bool;`
- `export fn parseRfc3339(s: string): Rfc3339Timestamp?;`
- `export fn fromUnixSeconds(unix_secs: i64): IsoDateTime;`
- `export fn formatRfc3339Utc(unix_secs: i64, out: []u8): usize;`
- `export fn isRfcLeapYear(year: i32): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`rfc3339@1.0.0`)，许可证：MIT
