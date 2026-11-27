[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# iso8601

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap ISO-8601 calendar/week/ordinal date, time, timezone offset, datetime, and duration parser/formatter (crates.io iso8601 equivalent)

## 安装与导入

```bash
cap add iso8601
```

```cp
import { isoIsDigit } from "iso8601";
```

## 核心导出 API

- `export fn isoIsDigit(c: u8): bool;`
- `export fn isoParseDigits(s: string, start: usize, count: usize): i64?;`
- `export fn isLeapYear(year: i32): bool;`
- `export fn daysInMonth(year: i32, month: u8): u8;`
- `export fn parseDate(s: string): IsoDate?;`
- `export fn parseTime(s: string): IsoTime?;`
- `export fn parseDateTime(s: string): IsoDateTime?;`
- `export fn parseDuration(s: string): IsoDuration?;`
- `export fn isoWrite2(out: []u8, pos: usize, val: u8): usize;`
- `export fn isoWrite4(out: []u8, pos: usize, val: i32): usize;`
- `export fn formatDateTime(dt: IsoDateTime, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`iso8601@1.0.0`)，许可证：MIT
