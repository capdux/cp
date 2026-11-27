[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/rrule-parse

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:rrule` ([https://github.com/jkbrzt/rrule](https://github.com/jkbrzt/rrule))（原库名 `rrule` → CP 包名 `@cap/rrule-parse`）

Zero-allocation RFC 5545 iCalendar RRULE parser, validator, and canonical formatter for CP — clean-room MIT parity with npm `rrule` v2. Composes `@cap/unicase` for case-insensitive key and enum matching.

纯 CP 零堆分配 RFC 5545 iCalendar 重复规则（RRULE）解析与格式化引擎（对标 npm `rrule` v2），跨包复用 `@cap/unicase` 实现大小写无关的键名与枚举匹配。

## Install / 安装

```bash
cap add @cap/rrule-parse
```

## Dependencies / 跨包复用

- `@cap/unicase`: 复用 `eqAscii` 进行 RFC 5545 键名（`FREQ`/`INTERVAL`/`BYDAY` 等）与枚举值大小写无关匹配

## API

### Frequency & Weekday Constants / 频率与星期掩码常量

- Frequencies: `FREQ_YEARLY (0)`, `FREQ_MONTHLY (1)`, `FREQ_WEEKLY (2)`, `FREQ_DAILY (3)`, `FREQ_HOURLY (4)`, `FREQ_MINUTELY (5)`, `FREQ_SECONDLY (6)`
- Weekday Bitmasks: `DAY_SU (1)`, `DAY_MO (2)`, `DAY_TU (4)`, `DAY_WE (8)`, `DAY_TH (16)`, `DAY_FR (32)`, `DAY_SA (64)`

### Struct & Functions / 核心类型与函数

| Symbol | Signature | Description |
| --- | --- | --- |
| `RRule` | `struct { valid: bool, freq: u8, interval: u32, count: u32, has_count: bool, until: string, byday_mask: u8, bymonth_mask: u16, wkst: u8 }` | Parsed recurrence rule |
| `parseRRule` | `(input: string): RRule` | Parse optional `RRULE:` prefix + `FREQ`/`INTERVAL`/`COUNT`/`UNTIL`/`BYDAY`/`BYMONTH`/`WKST`; strictly rejects missing `FREQ` or `COUNT`+`UNTIL` conflict |
| `formatRRule` | `(r: RRule, out: []u8): usize` | Serialize valid `RRule` to canonical RFC 5545 string |
| `matchesWeekday` | `(r: RRule, dow: u8): bool` | Check if weekday `dow` (`0=Sun..6=Sat`) matches `BYDAY` mask |
| `isValidRRule` | `(input: string): bool` | Validate an RFC 5545 RRULE string |

## Usage / 使用示例

```cp
import {
    parseRRule,
    formatRRule,
    matchesWeekday,
} from "@cap/rrule-parse";

const r = parseRRule("RRULE:FREQ=WEEKLY;INTERVAL=2;BYDAY=MO,WE,FR;COUNT=10");
const isMon = matchesWeekday(r, 1); // true

let buf: [128]u8 = undefined;
const n = formatRRule(r, buf[0..128]);
// "FREQ=WEEKLY;INTERVAL=2;COUNT=10;BYDAY=MO,WE,FR"
```
