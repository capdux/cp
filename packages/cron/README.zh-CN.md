[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 快速参考](README_AGENT.md)

# cron

纯 CP 零堆分配 5 字段 POSIX Cron 位掩码解析与下次 UTC 触发时间戳计算引擎 —— 洁净室 MIT 实现，对标 Rust `cron` 与 npm `cron-parser`。

将标准 5 字段 POSIX 表达式（`分 时 日 月 周`）及预定义宏（`@hourly`、`@daily`、`@weekly`、`@monthly`、`@yearly`）直接编译为紧凑整型位掩码（`u64`/`u32`/`u16`/`u8`），支持闰年 2 月 29 日精确跳转与 O(1) 触发判定，全程零堆分配。

## 源包溯源与零成本迁移

| 属性 | 说明 |
| --- | --- |
| 源平台 | **crates.io** |
| 原包名 | `cron`（CP 版 `v1.0.0`） |
| 上游仓库 | `https://github.com/zslayton/cron` |
| 许可证 | 上游 `MIT OR Apache-2.0`（CP 洁净室实现：`MIT`） |

从 Rust 迁移至 CP 时无需改名，直接沿用 `cron` 包名：

```rust
// Rust (crates.io: cron)
use cron::Schedule;
use std::str::FromStr;

let schedule = Schedule::from_str("0 0 9 * * 1-5").unwrap();
let next = schedule.upcoming(chrono::Utc).next();
```

```cp
// CP (cap: cron) — 零分配位掩码解析与 UTC 下次触发计算
import { CronSchedule } from "cron";

const expr: []const u8 = "0 9 * * 1-5";
const sched = CronSchedule.parse(&expr);
const next_ts = sched.nextAfter(1735689600 as i64);
```

## 安装

```bash
cap add cron
```

## 使用示例

### 基础用法：解析、校验与计算下次触发时间

```cp
import { CronSchedule, epochSecToUtc, utcToEpochSec } from "cron";

fn demoSchedule() {
    const expr: []const u8 = "*/15 9-17 * * 1-5";
    const sched = CronSchedule.parse(&expr);
    if (!sched.isValid()) {
        return;
    }

    // 2025-01-01T08:50:00Z
    const now = utcToEpochSec(2025, 1, 1, 8, 50) as i64;
    const is_now = sched.matchesUtc(now);    // false
    const next = sched.nextAfter(now);       // 2025-01-01T09:00:00Z
    const dt = epochSecToUtc(next as u64);
    // dt.year == 2025, dt.month == 1, dt.day == 1, dt.hour == 9, dt.minute == 0
    _ = is_now;
    _ = dt;
}
```

### 边界用法：闰年 2 月 29 日、宏别名与非法表达式处理

```cp
import { CronSchedule, isLeapYear, daysInMonth } from "cron";

fn demoEdgeCases() {
    // 1. 闰年精确跳转：2025-01-01 -> 2028-02-29T00:00:00Z (1835395200)
    const leap_expr: []const u8 = "0 0 29 2 *";
    const leap_sched = CronSchedule.parse(&leap_expr);
    const next_leap = leap_sched.nextAfter(1735689600 as i64);

    // 2. 预定义宏（@hourly / @daily / @weekly / @monthly / @yearly）
    const daily_expr: []const u8 = "@daily";
    const daily = CronSchedule.parse(&daily_expr);

    // 3. 非法范围或不可能出现的日期（2 月 31 日）安全返回 -1
    const bad_expr: []const u8 = "60 * * * *";
    const bad = CronSchedule.parse(&bad_expr);
    // bad.isValid() == false; bad.nextAfter(1735689600) == -1

    const feb31_expr: []const u8 = "0 0 31 2 *";
    const feb31 = CronSchedule.parse(&feb31_expr);
    // feb31.nextAfter(1735689600) == -1

    const ly = isLeapYear(2028);      // true
    const dim = daysInMonth(2028, 2); // 29
    _ = next_leap;
    _ = daily;
    _ = bad;
    _ = feb31;
    _ = ly;
    _ = dim;
}
```

## 核心 API 速查

| 符号 | 签名 | 说明 |
| --- | --- | --- |
| `CronSchedule.parse` | `fn(expr_in: &[]u8): CronSchedule` | 解析 5 字段 POSIX Cron 或宏别名，失败时 `valid == false` |
| `CronSchedule.isValid` | `fn(): bool` | 检查解析结果是否合法 |
| `CronSchedule.matchesDay` | `fn(day: u32, dow: u32): bool` | 按 POSIX 语义校验日/周匹配（均受限时取 OR，任一为 `*` 时取 AND） |
| `CronSchedule.matchesUtc` | `fn(epoch_sec: i64): bool` | 判断给定 UTC Unix 秒时间戳是否命中调度 |
| `CronSchedule.nextAfter` | `fn(epoch_sec: i64): i64` | 计算严格大于 `epoch_sec` 的下一次分钟对齐 UTC 时间戳（无解返回 `-1`） |
| `epochSecToUtc` | `fn(sec: u64): UtcDateTime` | UTC Unix 秒时间戳转 `{ year, month, day, hour, minute, dow }` |
| `utcToEpochSec` | `fn(year: u32, month: u32, day: u32, hour: u32, minute: u32): u64` | UTC 日期时间分量转 Unix 秒时间戳 |
| `isLeapYear` / `daysInMonth` / `dowForDate` | `fn(...) -> bool / u32` | 格里高利历闰年、月天数与星期几（`0..6`）工具函数 |
| `parseField` | `fn(field: &[]u8, min_val: u32, max_val: u32, is_dow: bool): FieldResult` | 底层单字段位掩码解析器（支持 `*`、`*/n`、`a-b`、`a-b/n`、`a,b,c`） |

## 性能与安全亮点

- **16-lane SIMD 加速**：底层接入 `cap:ascii`（`isSpace`、`isDigit`、`trim`）与 `cap:bin`（`bytesEq`），实现高速词法切分与宏匹配。
- **零堆分配与位掩码查表**：`CronSchedule` 全部驻留栈上，时间匹配均为 O(1) 位运算，无任何动态内存分配。
- **`undefined` 零清零优化**：内部字段边界数组（`[5]usize = undefined`）跳过冗余清零开销，降低热路径延迟。
- **安全失败语义**：面对越界数值、畸形表达式或 2 月 31 日等无解日期绝不 Panic，统一返回 `valid == false` 或 `-1`。

## 许可证

MIT — 洁净室纯 CP 实现，灵感源自 crates.io `cron`。
