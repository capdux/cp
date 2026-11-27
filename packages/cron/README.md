[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **crates.io** [`cron`](https://github.com/zslayton/cron)（`MIT OR Apache-2.0`）（原库名 `cron`，CP 包名 `@cap/cron-next`）。

# @cap/cron-next

Zero-allocation 5-field POSIX Cron bitmask parser and next UTC trigger timestamp calculator for CP — clean-room MIT parity with npm `cron-parser` (25M+/wk) and Rust `cron`.

纯 CP 零堆分配标准 5 段 POSIX Cron 位掩码解析与下次触发 UTC 时间戳计算引擎，支持 `*`、`*/n`、`a-b`、`a-b/n`、`a,b,c`、`@daily`/`@hourly` 宏别名与闰年 2 月 29 日精确跳转。

## Install / 安装

```bash
cap add @cap/cron-next
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `CronSchedule.parse` | `(expr: []const u8) -> CronSchedule` | Parse 5-field POSIX Cron or `@hourly`/`@daily`/`@weekly`/`@monthly`/`@yearly` |
| `isValid` | `() -> bool` | Check parsed validity |
| `matchesUtc` | `(epoch_sec: i64) -> bool` | Check whether UTC Unix timestamp matches schedule |
| `nextAfter` | `(epoch_sec: i64) -> i64` | Next strictly-greater minute-aligned UTC trigger timestamp (`-1` if none) |

## Usage / 使用示例

```cp
import { CronSchedule } from "@cap/cron-next";

const sched = CronSchedule.parse("0 0 29 2 *");
const nextLeap = sched.nextAfter(1735689600 as i64); // 2028-02-29T00:00:00Z (1835395200)
```
