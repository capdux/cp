# CP 标准库：5/6 字段 Cron 表达式解析与触发时间计算手册 (`cap:cron`)

## 一、 架构总览与设计准则

`cap:cron` 是 CP 语言官方内置的纯 CP **5/6 字段 Cron 表达式解析与 UTC 触发时间计算库**，**零 C 依赖、零 `std.posix`、零 `@import("bun")`**。内部采用位掩码（bitmask）紧凑表示与 Howard Hinnant O(1) 纯 UTC 日历换算，不依赖系统时钟或 libc，支持 `import { ... } from "cap:cron"` 与 `import cap from "cap"; cap.cron.*` 动态挂载调用。

### 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/cron/cron.cp` | `// Source: cp/lib/cron/cron.cp (...)` | `CronError`、`UtcDateTime`、`CronSchedule`（解析、匹配、逐字段跳跃 `nextAfter` / `prevBefore` / `nextN`）与 UTC 日历工具函数 |
| `lib/cron/index.cp` | `// Source: cp/lib/cron/index.cp (...)` | `cap:cron` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集与数据类型

- **`CronError`**：
  - `InvalidFieldCount`：字段数量不是 5 或 6。
  - `InvalidFieldValue`：字段值非法、越界、空列表项或未知 `@macro` 别名。
  - `InvalidStep`：步长语法错误或步长为 `0`。
  - `InvalidRange`：区间语法错误或 `start > end`。
  - `EmptyExpression`：空表达式或仅含空白字符。
  - `NoMatchingTime`：在合理搜索窗口（28 年）内不存在匹配的触发时间（如 `0 0 31 2 *`）。
  - `OutOfMemory`：`nextN` 分配结果切片失败。
- **`UtcDateTime`**：`{ year: i32, month: u8, day: u8, hour: u8, minute: u8, second: u8, dow: u8 }`。
- **`CronSchedule`**：
  - 位掩码字段：`seconds: u64`、`minutes: u64`、`hours: u32`、`days_of_month: u32`、`months: u16`、`days_of_week: u8`、`dom_wildcard: bool`、`dow_wildcard: bool`、`has_seconds: bool`。

### 2. 解析与触发计算方法

- **`CronSchedule.parse(expr: []const u8): CronSchedule ! CronError`**：
  - 支持 **5 字段**（`min hour dom mon dow`，秒固定为 `0`）与 **6 字段**（`sec min hour dom mon dow`）；
  - 支持 `@yearly` / `@annually`、`@monthly`、`@weekly`、`@daily` / `@midnight`、`@hourly` 宏别名；
  - 支持 `*`、`*/step`、`a-b`、`a-b/step`、`a,b,c`、`a/step`；
  - 支持英文月份 `JAN..DEC`（大小写不敏感）与星期 `SUN..SAT`（`0` 与 `7` 均视为 `SUN`）；
  - 遵循 **标准 Vixie Cron 语义**：当 `dom` 与 `dow` 均不为通配符 `*` 时，满足其中之一即触发（OR 语义）；若其中之一为 `*`，则仅检查另一字段（AND 语义）。
- **`matches(*_, epoch_sec: i64): bool`**：判断给定 UTC 秒时间戳是否匹配该调度计划。
- **`nextAfter(*_, epoch_sec: i64): i64 ! CronError`**：计算严格大于 `epoch_sec` 的下一次触发 UTC 秒时间戳（逐字段跳跃加速）。
- **`nextN(*_, alloc: std.mem.Allocator, epoch_sec: i64, count: usize): ([]i64) ! CronError`**：连续计算严格大于 `epoch_sec` 的下 `count` 次触发时间戳，返回堆分配切片（调用方负责 `alloc.free`）。
- **`prevBefore(*_, epoch_sec: i64): i64 ! CronError`**：计算严格小于 `epoch_sec` 的上一次触发 UTC 秒时间戳。
- **辅助函数**：`decomposeUtc(epoch_sec: i64): UtcDateTime`、`toEpochSec(year, month, day, hour, minute, second): i64`。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { CronSchedule, toEpochSec } from "cap:cron";

pub fn demoCron(): ! {
    const sched = CronSchedule.parse("0 30 9 * JAN-DEC MON-FRI")?;
    const base = toEpochSec(2024, 2, 28, 0, 0, 0);
    const next_ts = sched.nextAfter(base)?;
    const list = sched.nextN(std.heap.page_allocator, base, 3)?;
    defer std.heap.page_allocator.free(list);
    * = next_ts;
}
```

---

## Vixie Cron 快捷预设子模块（`preset.cp`，收敛自 `cron_preset`）

- `CronPreset`、`parseCronPreset`、`presetToCron5`、`normalizeCron5`。
