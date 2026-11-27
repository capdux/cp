[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# cron-parser

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap 5-field standard Cron expression parser, bitmask scheduler, and time matcher (clean-room port of cron-parser v4)

## 安装与导入

```bash
cap add cron-parser
```

```cp
import { crnParseUint } from "cron-parser";
```

## 核心导出 API

- `export fn crnParseUint(s: string): i32;`
- `export fn crnDaysInMonth(month: u8): u8;`
- `export fn crnParseField(field: string, min_val: i32, max_val: i32, is_dow: bool, ok_out: []bool): u64;`
- `export fn parseCron(expr: string): CronSchedule;`
- `export fn matches(sched: CronSchedule, month: u8, dom: u8, dow: u8, hour: u8, minute: u8): bool;`
- `export fn nextMinuteOffset(sched: CronSchedule, month: u8, dom: u8, dow: u8, hour: u8, minute: u8): u32?;`
- `export fn isValidCron(expr: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`cron-parser@1.0.0`)，许可证：MIT
