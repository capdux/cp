[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# d3-time

> 版本：`1.0.0` | 许可证：`ISC` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation UTC calendar time interval arithmetic (second, minute, hour, day, week, month, year) in pure CP

## 安装与导入

```bash
cap add d3-time
```

```cp
import { d3tmIsLeapYear } from "d3-time";
```

## 核心导出 API

- `export fn d3tmIsLeapYear(y: i64): bool;`
- `export fn d3tmDaysInMonth(y: i64, m: i64): i64;`
- `export fn d3tmCivilFromDays(z_in: i64): D3TmYmd;`
- `export fn d3tmDaysFromCivil(y_in: i64, m: i64, d: i64): i64;`
- `export fn utcFromYmd(year: i64, month: i64, day: i64): i64;`
- `export fn timeSecondFloor(ms: i64): i64;`
- `export fn timeSecondCeil(ms: i64): i64;`
- `export fn timeSecondOffset(ms: i64, step: i64): i64;`
- `export fn timeSecondCount(start_ms: i64, end_ms: i64): i64;`
- `export fn timeMinuteFloor(ms: i64): i64;`
- `export fn timeMinuteOffset(ms: i64, step: i64): i64;`
- `export fn timeHourFloor(ms: i64): i64;`
- `export fn timeHourOffset(ms: i64, step: i64): i64;`
- `export fn timeDayFloor(ms: i64): i64;`
- `export fn timeDayCeil(ms: i64): i64;`
- `export fn timeDayOffset(ms: i64, step: i64): i64;`
- `export fn timeDayCount(start_ms: i64, end_ms: i64): i64;`
- `export fn timeWeekFloor(ms: i64): i64;`
- `export fn timeWeekOffset(ms: i64, step: i64): i64;`
- `export fn timeMonthFloor(ms: i64): i64;`
- `export fn timeMonthOffset(ms: i64, step: i64): i64;`
- `export fn timeYearFloor(ms: i64): i64;`
- `export fn timeYearOffset(ms: i64, step: i64): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`d3-time@1.0.0`)，许可证：ISC
