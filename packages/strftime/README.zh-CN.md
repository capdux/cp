[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# strftime

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap POSIX strftime & ISO-8601 date/time formatter for Unix timestamps and broken-down calendar dates

## 安装与导入

```bash
cap add strftime
```

```cp
import { isLeapYear } from "strftime";
```

## 核心导出 API

- `export fn isLeapYear(y: i32): bool;`
- `export fn daysInMonth(y: i32, m: u8): u8;`
- `export fn dayOfYear(y: i32, m: u8, d: u8): u16;`
- `export fn fromUnixSeconds(unix_sec: i64, tz_offset_sec: i32): DateTime;`
- `export fn weekdayShort(w: u8): string;`
- `export fn weekdayFull(w: u8): string;`
- `export fn monthShort(m: u8): string;`
- `export fn monthFull(m: u8): string;`
- `export fn strftime(fmt: string, dt: DateTime, out: []u8): string;`
- `export fn formatUnix(fmt: string, unix_sec: i64, tz_offset_sec: i32, out: []u8): string;`
- `export fn formatIso8601Utc(unix_sec: i64, out: []u8): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`strftime@1.0.0`)，许可证：MIT
