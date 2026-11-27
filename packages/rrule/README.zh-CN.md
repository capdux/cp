[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# rrule

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation RFC 5545 iCalendar RRULE parser, validator & formatter (npm rrule v2 parity) composing @cap/unicase for CP

## 安装与导入

```bash
cap add rrule
```

```cp
import { parseRRule } from "rrule";
```

## 核心导出 API

- `export fn parseRRule(input: string): RRule;`
- `export fn formatRRule(r: RRule, out: []u8): usize;`
- `export fn matchesWeekday(r: RRule, dow: u8): bool;`
- `export fn isValidRRule(input: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`rrule@1.0.0`)，许可证：BSD-3-Clause
