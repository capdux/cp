# CP 标准库：纯 CP 时区转换与夏令时规则引擎 (`cap:tz`)

> [!NOTE]
> 本子模块已归并至统一主包 `cap:time`，导入请使用 `import { ... } from "cap:time"`。

## 一、 架构总览与设计准则

`cap:tz` 是 CP 语言官方内置的纯 CP、零 C 依赖 **时区转换与夏令时（DST）规则引擎**（零 `std.posix`、零 `@import("bun")`），专为全球化后端、日程排期（配合 `cap:ical` / `cap:time`）与日志分析设计。既支持 `import { ... } from "cap:time"` 具名导入，也支持 `import cap from "cap"; cap.tz.*` 动态懒挂载调用。

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/tz/tz.cp` | `// Source: cp/lib/tz/tz.cp (...)` | `TzError`、`DstRule`、`LocalDateTime`、`Timezone`、Howard Hinnant $O(1)$ 历法算法、POSIX TZ 解析、IANA 映射与 RFC 3339 格式化 |
| `lib/tz/index.cp` | `// Source: cp/lib/tz/index.cp (Unified Exports for cap:tz)` | `cap:tz` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集合与数据结构

- **`TzError`**：`UnknownTimezone`、`InvalidTzString`、`InvalidOffset`、`InvalidDateTime`、`OutOfMemory`。
- **`DstRule`**：POSIX `Mm.w.d[/time]` 夏令时切换规则：
  - 字段：`month: u8`（`1..=12`）、`week: u8`（`1..=5`，`5` 表示本月最后一个 `weekday`）、`weekday: u8`（`0=Sun..6=Sat`）、`time_sec: i32`（默认 `7200` 即 `02:00:00`）。
- **`LocalDateTime`**：
  - 字段：`year: i32`、`month: u8`、`day: u8`、`hour: u8`、`minute: u8`、`second: u8`、`offset_sec: i32`、`is_dst: bool`、`abbr: []const u8`。
  - 方法：`dayOfWeek(): u8`（`0=Sun..6=Sat`）、`formatRfc3339(alloc): ([]u8) ! TzError`。

### 2. `Timezone` 核心接口

- **构造与解析**：
  - `Timezone.utc(): Timezone`：返回 UTC 时区（偏移 `0`）。
  - `Timezone.fixedOffset(tz_name, offset_sec): Timezone`：构造固定偏移时区（东区为正，西区为负）。
  - `Timezone.parseOffset(s): i32 ! TzError`：解析 `"Z"`、`"UTC+08:00"`、`"+05:30"`、`"-05:00"`、`"-0530"` 等偏移字符串为秒数。
  - `Timezone.parsePosix(posix_str): Timezone ! TzError`：解析 POSIX TZ 规则字符串（如 `"EST5EDT,M3.2.0/2,M11.1.0/2"`、`"CET-1CEST,M3.5.0,M10.5.0/3"`、`"AEST-10AEDT,M10.1.0,M4.1.0/3"`），自动处理 POSIX 反向符号约定与南半球跨年 DST。
  - `Timezone.lookup(iana_name): Timezone ! TzError`：内置 20+ 全球主流 IANA 时区映射（`UTC`、`Asia/Shanghai`、`Asia/Tokyo`、`Asia/Kolkata`、`America/New_York`、`America/Los_Angeles`、`America/Chicago`、`Europe/London`、`Europe/Berlin`、`Australia/Sydney`、`Pacific/Auckland` 等）。
- **时区计算与转换**：
  - `tz.isDstAt(unix_sec: i64): bool`：精确判断给定 Unix 时间戳是否处于夏令时（支持北半球与南半球跨年区间）。
  - `tz.offsetAt(unix_sec: i64): i32`：返回该时刻相对 UTC 的偏移秒数。
  - `tz.toLocal(unix_sec: i64): LocalDateTime ! TzError`：基于 Howard Hinnant `civil_from_days` $O(1)$ 算法将 64 位 Unix 时间戳（支持 1970 前与 2038 后）转为本地 `LocalDateTime`。
  - `tz.toUnix(year, month, day, hour, minute, second): i64 ! TzError`：校验日历合法性并转换为 Unix 秒级时间戳。
  - `tz.formatRfc3339(alloc, unix_sec: i64): ([]u8) ! TzError`：格式化为带时区偏移的 RFC 3339 字符串（如 `2026-07-15T10:30:00-04:00` 或 `2026-01-15T18:30:00+08:00`）。

### 3. 历法辅助函数

- `isLeapYear(year: i32): bool`
- `daysInMonth(year: i32, month: u8): u8`
- `isValidDateTime(year, month, day, hour, minute, second): bool`
- `daysFromCivil(y: i32, m: u8, d: u8): i64`
- `weekdayFromDays(z: i64): u8`

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { Timezone } from "cap:time";
import cap from "cap";

pub fn main() {
    const alloc = std.heap.page_allocator;
    let ny = Timezone.lookup("America/New_York") catch unreachable;
    let ts = ny.toUnix(2026, 7, 15, 10, 30, 0) catch unreachable;
    let rfc = ny.formatRfc3339(alloc, ts) catch unreachable;
    defer alloc.free(rfc); // "2026-07-15T10:30:00-04:00"

    let sh = cap.tz.Timezone.lookup("Asia/Shanghai") catch unreachable;
    let local = sh.toLocal(ts) catch unreachable;
    std.debug.print("{d}-{d:0>2}-{d:0>2} {s}\n", .{ local.year, local.month, local.day, local.abbr });
}
```
