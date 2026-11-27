[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# rrule

Zero-allocation RFC 5545 iCalendar RRULE parser, validator & formatter (npm rrule v2 parity) composing unicase for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:rrule` ([https://github.com/jkbrzt/rrule](https://github.com/jkbrzt/rrule))（原库名 `rrule` → CP 包名 `rrule`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `rrule` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add rrule
```

```cp
import {
    RRule,
    rrlInvalid,
    rrlParsePosU32,
    rrlParseFreq,
    rrlParseDayCode,
    rrlParseByDayItem,
    rrlParse2Digits,
    rrlIsValidUntil,
} from "rrule";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `RRule` | `class` | 字段: `valid: bool, freq: u8, interval: u32, count: u32, has_count: bool, until: string, +3 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `rrlInvalid` | `(): RRule` | 调用 `rrlInvalid` 并返回 `RRule`。 |
| `rrlParsePosU32` | `(s: string): u32` | 调用 `rrlParsePosU32` 并返回 `u32`。 |
| `rrlParseFreq` | `(v: string): u8` | 调用 `rrlParseFreq` 并返回 `u8`。 |
| `rrlParseDayCode` | `(code: string): u8` | 调用 `rrlParseDayCode` 并返回 `u8`。 |
| `rrlParseByDayItem` | `(item: string): u8` | 调用 `rrlParseByDayItem` 并返回 `u8`。 |
| `rrlParse2Digits` | `(s: string, pos: usize): u32` | 调用 `rrlParse2Digits` 并返回 `u32`。 |
| `rrlIsValidUntil` | `(v: string): bool` | 调用 `rrlIsValidUntil` 并返回 `bool`。 |
| `parseRRule` | `(input: string): RRule` | 通过 `parseRRule` 解析输入数据，返回 `RRule`。 |
| `rrlFreqName` | `(freq: u8): string` | 调用 `rrlFreqName` 并返回 `string`。 |
| `rrlDayName` | `(mask: u8): string` | 调用 `rrlDayName` 并返回 `string`。 |
| `formatRRule` | `(r: RRule, out: []u8): usize` | 通过 `formatRRule` 编码或格式化输出，返回 `usize`。 |
| `matchesWeekday` | `(r: RRule, dow: u8): bool` | 调用 `matchesWeekday` 并返回 `bool`。 |
| `isValidRRule` | `(input: string): bool` | 执行 `isValidRRule` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { parseRRule } from "rrule";

pub fn main() {
    let result = parseRRule("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
