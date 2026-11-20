# CP 标准库：RFC 5545 iCalendar 解析生成与 RRULE 展开引擎 (`cap:ical`)

## 一、 架构总览与设计准则

`cap:ical` 是 CP 语言官方内置的纯 CP **RFC 5545 iCalendar (`.ics`) 解析、生成与 `RRULE` 重复规则展开引擎**，**零外部 C 依赖、零 `std.posix`、零 `@import("bun")`**，专为个人助理 AI Agent、定时调度系统与日历集成场景设计。既支持 `import { ... } from "cap:ical"` 具名导入，也支持 `import cap from "cap"; cap.ical.*` 动态懒挂载调用。

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/ical/ical.cp` | `// Source: cp/lib/ical/ical.cp (...)` | `IcalError`、`RRuleFreq`、`Weekday`、`IcalDateTime`、`RRule`、`IcalEvent`、`IcalCalendar` |
| `lib/ical/index.cp` | `// Source: cp/lib/ical/index.cp (Unified Exports for cap:ical)` | `cap:ical` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集合与枚举类型

- **`IcalError`**：`InvalidFormat`、`InvalidDateTime`、`InvalidRRule`、`MissingRequiredField`、`OutOfMemory`。
- **`RRuleFreq`**：`Daily`、`Weekly`、`Monthly`、`Yearly`。
- **`Weekday: u8`**：`Su = 0`、`Mo = 1`、`Tu = 2`、`We = 3`、`Th = 4`、`Fr = 5`、`Sa = 6`。

### 2. 日历日期时间 (`IcalDateTime`)

- 字段：`year: i32`、`month: u8`、`day: u8`、`hour: u8`、`minute: u8`、`second: u8`、`is_utc: bool`、`is_date_only: bool`。
- 核心方法：
  - `IcalDateTime.parse(s): IcalDateTime!`：解析 `YYYYMMDD` 与 `YYYYMMDDTHHMMSS[Z]`，严格校验闰年与每月天数。
  - `dt.format(alloc): ([]u8)!`：格式化为 RFC 5545 日期或日期时间字符串。
  - `dt.dayOfWeek(): Weekday`：基于 Tomohiko Sakamoto 算法精准计算星期几。
  - `dt.addDays(days: i64): IcalDateTime`：支持正负跨度、跨月、跨闰年（如 `2024-02-29`）及世纪非闰年（如 `2100-02-28`）的 $O(1)$ 精确日历加减。
  - `dt.compare(other): i32`：返回 `-1` / `0` / `1`。

### 3. 重复规则展开引擎 (`RRule`)

- `RRule.parse(alloc, rule_str): RRule!`：解析 `FREQ=...;INTERVAL=...;BYDAY=...;BYMONTHDAY=...;COUNT=...;UNTIL=...`，校验 `FREQ` 合法性与 `INTERVAL >= 1`。
- `rule.deinit()`：释放内部分配列表。
- `rule.expand(alloc, dtstart, range_start, range_end, exdates, max_instances): ([]IcalDateTime)!`：支持 `DAILY`、`WEEKLY`（含 `BYDAY`）、`MONTHLY`（含 `BYMONTHDAY` 正负日与 `BYDAY`）及 `YEARLY`，严格执行 `COUNT`、`UNTIL`、`EXDATE` 排除与 `max_instances` 死循环防护。

### 4. 日程事件与日历文档 (`IcalEvent` / `IcalCalendar`)

- `IcalCalendar.parse(alloc, ics_text): IcalCalendar!`：自动处理 RFC 5545 行折行展开（`CRLF` / `LF` + 空格/制表符续行）与 `\\` / `\,` / `\;` / `\n` 文本反转义，解析 `VCALENDAR` / `VEVENT` / `DTSTART` / `DTEND` / `RRULE` / `EXDATE`。
- `cal.occurrences(alloc, range_start, range_end, max_instances): ([]IcalDateTime)!`：展开全部事件并按时间升序返回实例切片。
- `cal.toIcs(alloc): ([]u8)!`：按 RFC 5545 规范输出带 `BEGIN:VCALENDAR`...`END:VCALENDAR`（`\r\n` 换行与规范文本转义）的 `.ics` 字符串。
- `cal.deinit()`：释放全部事件、规则与字符串内存。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { IcalCalendar, IcalDateTime } from "cap:ical";
import cap from "cap";

pub fn main() {
    const alloc = std.heap.page_allocator;
    const ics =
        "BEGIN:VCALENDAR\r\n" ++
        "VERSION:2.0\r\n" ++
        "BEGIN:VEVENT\r\n" ++
        "UID:standup-001@cp-lang.org\r\n" ++
        "DTSTART:20260302T090000Z\r\n" ++
        "RRULE:FREQ=WEEKLY;BYDAY=MO,FR;COUNT=4\r\n" ++
        "SUMMARY:Team Standup\\, Core\r\n" ++
        "END:VEVENT\r\n" ++
        "END:VCALENDAR\r\n";

    let cal = IcalCalendar.parse(alloc, ics) catch unreachable;
    defer cal.deinit();

    let start = cap.ical.IcalDateTime.parse("20260301T000000Z") catch unreachable;
    let end = cap.ical.IcalDateTime.parse("20260331T235959Z") catch unreachable;
    let occs = cal.occurrences(alloc, start, end, 10) catch unreachable;
    defer alloc.free(occs);
}
```
