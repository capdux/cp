[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# d3-time

Zero-allocation UTC calendar time interval arithmetic (second, minute, hour, day, week, month, year) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:d3-time` ([https://github.com/d3/d3-time](https://github.com/d3/d3-time))

| 元信息 | 值 |
| --- | --- |
| 包名 | `d3-time` |
| 版本 | `1.0.0` |
| 许可证 | `ISC` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add d3-time
```

```cp
import {
    D3TmYmd,
    d3tmIsLeapYear,
    d3tmDaysInMonth,
    d3tmCivilFromDays,
    d3tmDaysFromCivil,
    utcFromYmd,
    timeSecondFloor,
    timeSecondCeil,
} from "d3-time";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `D3TmYmd` | `class` | 字段: `year: i64, month: i64, day: i64` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `d3tmIsLeapYear` | `(y: i64): bool` | 调用 `d3tmIsLeapYear` 并返回 `bool`。 |
| `d3tmDaysInMonth` | `(y: i64, m: i64): i64` | 调用 `d3tmDaysInMonth` 并返回 `i64`。 |
| `d3tmCivilFromDays` | `(z_in: i64): D3TmYmd` | 调用 `d3tmCivilFromDays` 并返回 `D3TmYmd`。 |
| `d3tmDaysFromCivil` | `(y_in: i64, m: i64, d: i64): i64` | 调用 `d3tmDaysFromCivil` 并返回 `i64`。 |
| `utcFromYmd` | `(year: i64, month: i64, day: i64): i64` | 调用 `utcFromYmd` 并返回 `i64`。 |
| `timeSecondFloor` | `(ms: i64): i64` | 调用 `timeSecondFloor` 并返回 `i64`。 |
| `timeSecondCeil` | `(ms: i64): i64` | 调用 `timeSecondCeil` 并返回 `i64`。 |
| `timeSecondOffset` | `(ms: i64, step: i64): i64` | 调用 `timeSecondOffset` 并返回 `i64`。 |
| `timeSecondCount` | `(start_ms: i64, end_ms: i64): i64` | 调用 `timeSecondCount` 并返回 `i64`。 |
| `timeMinuteFloor` | `(ms: i64): i64` | 调用 `timeMinuteFloor` 并返回 `i64`。 |
| `timeMinuteOffset` | `(ms: i64, step: i64): i64` | 调用 `timeMinuteOffset` 并返回 `i64`。 |
| `timeHourFloor` | `(ms: i64): i64` | 调用 `timeHourFloor` 并返回 `i64`。 |
| `timeHourOffset` | `(ms: i64, step: i64): i64` | 调用 `timeHourOffset` 并返回 `i64`。 |
| `timeDayFloor` | `(ms: i64): i64` | 调用 `timeDayFloor` 并返回 `i64`。 |
| `timeDayCeil` | `(ms: i64): i64` | 调用 `timeDayCeil` 并返回 `i64`。 |
| `timeDayOffset` | `(ms: i64, step: i64): i64` | 调用 `timeDayOffset` 并返回 `i64`。 |
| `timeDayCount` | `(start_ms: i64, end_ms: i64): i64` | 调用 `timeDayCount` 并返回 `i64`。 |
| `timeWeekFloor` | `(ms: i64): i64` | 调用 `timeWeekFloor` 并返回 `i64`。 |
| `timeWeekOffset` | `(ms: i64, step: i64): i64` | 调用 `timeWeekOffset` 并返回 `i64`。 |
| `timeMonthFloor` | `(ms: i64): i64` | 调用 `timeMonthFloor` 并返回 `i64`。 |
| `timeMonthOffset` | `(ms: i64, step: i64): i64` | 调用 `timeMonthOffset` 并返回 `i64`。 |
| `timeYearFloor` | `(ms: i64): i64` | 调用 `timeYearFloor` 并返回 `i64`。 |
| `timeYearOffset` | `(ms: i64, step: i64): i64` | 调用 `timeYearOffset` 并返回 `i64`。 |

## 最小调用示例

```cp
import { d3tmIsLeapYear } from "d3-time";

pub fn main() {
    let result = d3tmIsLeapYear(0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
