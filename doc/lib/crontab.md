# `cap:crontab` — 纯 CP 零堆 Cron 表达式解析与下次触发计算引擎

## 概述

`cap:crontab`（亦支持 `cap.crontab` 动态挂载）实现标准 5 段式 Cron 表达式（支持 `*`、`*/step`、`a-b`、`a-b/step`、枚举列表、英文月份 `JAN-DEC`、星期 `SUN-SAT` 及 `7->0`、宏 `@yearly`/`@monthly`/`@weekly`/`@daily`/`@hourly`）的零堆位掩码解析、POSIX DOM/DOW 联合匹配与 `nextAfter` 下次触发时间计算。

## 核心类型

- `CronError`: `InvalidFieldCount`、`InvalidNumber`、`OutOfRange`、`InvalidRange`、`InvalidStep`、`EmptyField`。
- `DateTime`: `year`, `month`, `day`, `hour`, `minute`，内置 `weekday()`、`isLeapYear()`、`daysInMonth()`、`addOneMinute()`、`fromUnixSeconds()`。
- `CronExpr`: `parse(expr)`、`matches(dt)`、`nextAfter(from)`。

## 示例

```cp
import { CronExpr, DateTime } from "cap:crontab";

const cron = try CronExpr.parse("*/15 9-17 * * MON-FRI");
const next = cron.nextAfter(DateTime{ .year = 2026, .month = 11, .day = 21, .hour = 9, .minute = 0 });
```
