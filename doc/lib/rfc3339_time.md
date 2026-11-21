# `cap:rfc3339_time` — RFC 3339 / ISO 8601 时间戳零分配解析与格式化库

> 来源：吸收自 `/data/dev/bun/src/time.zig`（RFC 3339 / ISO 8601 Timestamp Parser & Formatter）

`cap:rfc3339_time` 提供纯 CP、零外部依赖、零堆分配的 RFC 3339 / ISO 8601 时间戳解析、闰年与日期合法性校验、Unix 秒级时间戳互转（Howard Hinnant 纯整数算法，完整支持 1970 年前负时间戳）及规范化格式化能力。

## 核心函数与类型

### 日历辅助函数

```cp
pub fn isLeapYear(year: u16): bool
pub fn daysInMonth(year: u16, month: u8): u8
```

### `Rfc3339Time`

```cp
pub struct Rfc3339Time {
    year: u16 = 1970,
    month: u8 = 1,
    day: u8 = 1,
    hour: u8 = 0,
    minute: u8 = 0,
    second: u8 = 0,
    nanos: u32 = 0,
    offset_minutes: i16 = 0, // 东区为正、西区为负，Z 为 0

    static fn parse(s: []const u8): Rfc3339Time?
    fn toUnixSec(): i64
    static fn fromUnixSecUtc(unix_sec: i64, nanos: u32): Rfc3339Time?
    fn format(out: []u8): ([]const u8)?
}
```

- `parse`：支持 `T`/`t`/` ` 分隔符、可选小数秒 `.1`..`.9` 自动右补零至纳秒、`Z`/`z`/`+HH:MM`/`-HH:MM`；严格校验闰年 2 月 29 日、月日范围、时分秒与时区偏移范围。
- `toUnixSec` / `fromUnixSecUtc`：基于 Howard Hinnant `daysFromCivil` / `civilFromDays` 纯整数算法，支持 1970 年前负时间戳（如 `1969-12-31T23:59:59Z` ↔ `-1`）。
- `format`：规范化输出 RFC 3339 字符串（去除小数秒尾随零，`offset_minutes == 0` 输出 `Z`）。

## 使用示例

```cp
import { Rfc3339Time } from "cap:rfc3339_time";

const t = Rfc3339Time.parse("2026-11-22T01:55:00.25+08:00").?;
const unix = t.toUnixSec();
const utc = Rfc3339Time.fromUnixSecUtc(unix, t.nanos).?;
let buf: [64]u8 = [0; 64];
const s = utc.format(buf[0..]).?; // "2026-11-21T17:55:00.25Z"
```
