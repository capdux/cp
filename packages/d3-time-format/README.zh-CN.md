[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# d3-time-format

> 版本：`1.0.0` | 许可证：`ISC` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap UTC strftime/strptime date formatter and parser for CP (clean-room ISC implementation of npm d3-time-format).

## 安装与导入

```bash
cap add d3-time-format
```

```cp
import { d3tfIsLeapYear } from "d3-time-format";
```

## 核心导出 API

- `export fn d3tfIsLeapYear(y: u64): bool;`
- `export fn d3tfDaysInMonth(y: u64, m: u64): u64;`
- `export fn d3tfDayOfYear(y: u64, m: u64, d: u64): u64;`
- `export fn d3tfFromUnixMs(unix_ms: i64): D3tfDateTime;`
- `export fn d3tfToUnixMs(year: u64, month: u64, day: u64, hour: u64, minute: u64, second: u64, ms: u64): i64;`
- `export fn d3tfMonthShort(m: u64): string;`
- `export fn d3tfMonthFull(m: u64): string;`
- `export fn d3tfWeekdayShort(w: u64): string;`
- `export fn d3tfWeekdayFull(w: u64): string;`
- `export fn d3tfWriteStr(out: []u8, pos: usize, s: string): usize;`
- `export fn d3tfWritePadded(out: []u8, pos: usize, val: u64, width: usize): usize;`
- `export fn utcFormat(spec: string, unix_ms: i64, out: []u8): usize;`
- `export fn d3tfToLower(ch: u8): u8;`
- `export fn d3tfMatchWordCI(input: string, pos: usize, word: string): bool;`
- `export fn d3tfParseDigits(input: string, pos: usize, min_d: usize, max_d: usize): D3tfUintParse;`
- `export fn utcParse(spec: string, input: string): i64?;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`d3-time-format@1.0.0`)，许可证：ISC
