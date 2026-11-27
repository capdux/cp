[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# cron-parser

Zero-heap 5-field standard Cron expression parser, bitmask scheduler, and time matcher (clean-room port of cron-parser v4)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:cron-parser` ([https://github.com/harrisiirak/cron-parser](https://github.com/harrisiirak/cron-parser))

| 元信息 | 值 |
| --- | --- |
| 包名 | `cron-parser` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add cron-parser
```

```cp
import {
    CronSchedule,
    crnParseUint,
    crnDaysInMonth,
    crnParseField,
    parseCron,
    matches,
    nextMinuteOffset,
    isValidCron,
} from "cron-parser";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `CronSchedule` | `class` | 字段: `valid: bool, min_mask: u64, hour_mask: u32, dom_mask: u32, month_mask: u16, dow_mask: u8` · 方法: `invalid` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `crnParseUint` | `(s: string): i32` | 调用 `crnParseUint` 并返回 `i32`。 |
| `crnDaysInMonth` | `(month: u8): u8` | 调用 `crnDaysInMonth` 并返回 `u8`。 |
| `crnParseField` | `(field: string, min_val: i32, max_val: i32, is_dow: bool, ok_out: []bool): u64` | 调用 `crnParseField` 并返回 `u64`。 |
| `parseCron` | `(expr: string): CronSchedule` | 通过 `parseCron` 解析输入数据，返回 `CronSchedule`。 |
| `matches` | `(sched: CronSchedule, month: u8, dom: u8, dow: u8, hour: u8, minute: u8): bool` | 调用 `matches` 并返回 `bool`。 |
| `nextMinuteOffset` | `(sched: CronSchedule, month: u8, dom: u8, dow: u8, hour: u8, minute: u8): u32?` | 调用 `nextMinuteOffset` 并返回 `u32?`。 |
| `isValidCron` | `(expr: string): bool` | 执行 `isValidCron` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { parseCron } from "cron-parser";

pub fn main() {
    let result = parseCron("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
