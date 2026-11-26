# `@cap/cron-desc` — 零分配 Cron 表达式自然语言翻译引擎（中/英双语）

> Zero-allocation Cron expression to human-readable English & Chinese natural language translator for CP (Clean-room MIT implementation modeled after npm `cronstrue` and Rust `cron-descriptor`).

## 概述 / Overview

`@cap/cron-desc` 将标准 5 字段 Cron 表达式及常用预定义宏翻译成清晰易读的英文或中文自然语言描述，纯 CP 实现、零外部依赖、零堆分配：

- **标准 5 字段全语法支持**：`minute hour dom month dow`，支持 `*`、固定值、`*/step` 步长、`a-b` 范围、`a,b,c` 列表、月份英文缩写 `JAN`-`DEC`、星期英文缩写 `SUN`-`SAT` 及 `0-7`（`0` 与 `7` 均表示周日）。
- **预定义宏支持**：`@yearly`、`@annually`、`@monthly`、`@weekly`、`@daily`、`@midnight`、`@hourly`（大小写不敏感）。
- **双语 & 时制切换**：内置 `Locale.En` 与 `Locale.Zh`，支持 24 小时制与 12 小时制切换。
- **零堆分配输出**：直接写入调用方栈缓冲 `out: []u8`，无内存分配开销。

## 安装 / Installation

```bash
cap add @cap/cron-desc
```

## 导出 API / API Reference

| 符号 / 签名 | 说明 |
| :--- | :--- |
| `Locale` | 语言枚举：`Locale.En`（英文）、`Locale.Zh`（中文） |
| `CronDescOptions` | 配置结构体 `{ locale: Locale, use_24h: bool }`，提供快捷工厂 `english()` / `chinese()` |
| `isValidCron(expr: string): bool` | 严格校验 5 字段 Cron 表达式或预定义宏是否合法 |
| `describeCron(expr: string, out: []u8): string` | 翻译为默认英文 24 小时制描述（非法返回 `""`） |
| `describeCronWith(expr: string, opts: CronDescOptions, out: []u8): string` | 按指定语言与 12h/24h 选项翻译 Cron 表达式 |

## 对照示例 / Examples

| Cron 表达式 | 英文输出 (`Locale.En`, 24h) | 中文输出 (`Locale.Zh`, 24h) |
| :--- | :--- | :--- |
| `* * * * *` | `Every minute` | `每分钟` |
| `*/15 * * * *` | `Every 15 minutes` | `每 15 分钟` |
| `0 9 * * 1-5` | `At 09:00, Monday through Friday` | `在 09:00，周一至周五` |
| `30 14 1 * *` | `At 14:30, on day 1 of the month` | `在 14:30，每月第 1 天` |
| `@daily` | `At 00:00, every day` | `每天 00:00` |
| `@hourly` | `Every hour` | `每小时` |
| `@weekly` | `At 00:00, only on Sunday` | `在 00:00，仅在周日` |
| `0 8 1,15 JAN,JUN *` | `At 08:00, on days 1, 15 of the month, only in January, June` | `在 08:00，每月第 1、15 天，仅在 1 月、6 月` |

## 代码示例 / Usage

```cp
import {
    Locale,
    CronDescOptions,
    isValidCron,
    describeCron,
    describeCronWith,
} from "@cap/cron-desc";

fn demoCronDesc() {
    let buf: [128]u8 = [0; 128];
    const en = describeCron("0 9 * * 1-5", buf[0..128]);
    // => "At 09:00, Monday through Friday"

    const zh = describeCronWith("0 9 * * 1-5", CronDescOptions.chinese(), buf[0..128]);
    // => "在 09:00，周一至周五"

    const ok = isValidCron("*/15 * * * *"); // true
    _ = en;
    _ = zh;
    _ = ok;
}
```

## 许可协议 / License

MIT
