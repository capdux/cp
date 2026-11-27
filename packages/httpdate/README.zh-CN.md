[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# httpdate

Zero-allocation RFC 9110 Section 5.6.7 HTTP-date parser (IMF-fixdate, RFC 850, asctime) and formatter with Hinnant calendar math in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:httpdate` ([https://github.com/pyfisch/httpdate](https://github.com/pyfisch/httpdate)) / `npm:http-date` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))（原库名 `httpdate` → CP 包名 `httpdate`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `httpdate` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add httpdate
```

```cp
import {
    HttpDateTime,
    invalidDateTime,
    isOws,
    isLeapYear,
    daysInMonth,
    daysFromCivil,
    buildValidated,
    parse2Digits,
} from "httpdate";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `HttpDateTime` | `class` | 字段: `year: u16, month: u8, day: u8, hour: u8, minute: u8, second: u8, +2 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `invalidDateTime` | `(): HttpDateTime` | 调用 `invalidDateTime` 并返回 `HttpDateTime`。 |
| `isOws` | `(b: u8): bool` | 执行 `isOws` 条件判定，返回 `bool`。 |
| `isLeapYear` | `(y: u16): bool` | 执行 `isLeapYear` 条件判定，返回 `bool`。 |
| `daysInMonth` | `(y: u16, m: u8): u8` | 调用 `daysInMonth` 并返回 `u8`。 |
| `daysFromCivil` | `(year: u16, month: u8, day: u8): u64` | 调用 `daysFromCivil` 并返回 `u64`。 |
| `buildValidated` | `(year: i32, month: i32, day: i32, hour: i32, minute: i32, second: i32): HttpDateTime` | 调用 `buildValidated` 并返回 `HttpDateTime`。 |
| `parse2Digits` | `(s: string, pos: usize): i32` | 通过 `parse2Digits` 解析输入数据，返回 `i32`。 |
| `parse4Digits` | `(s: string, pos: usize): i32` | 通过 `parse4Digits` 解析输入数据，返回 `i32`。 |
| `parseMonth` | `(s: string, pos: usize): i32` | 通过 `parseMonth` 解析输入数据，返回 `i32`。 |
| `isShortWeekday` | `(s: string, pos: usize): bool` | 执行 `isShortWeekday` 条件判定，返回 `bool`。 |
| `isLongWeekday` | `(s: string): bool` | 执行 `isLongWeekday` 条件判定，返回 `bool`。 |
| `parseImfFixdate` | `(s: string): HttpDateTime` | 通过 `parseImfFixdate` 解析输入数据，返回 `HttpDateTime`。 |
| `parseRfc850` | `(s: string): HttpDateTime` | 通过 `parseRfc850` 解析输入数据，返回 `HttpDateTime`。 |
| `parseAsctime` | `(s: string): HttpDateTime` | 通过 `parseAsctime` 解析输入数据，返回 `HttpDateTime`。 |
| `parseHttpDate` | `(input: string): HttpDateTime` | 通过 `parseHttpDate` 解析输入数据，返回 `HttpDateTime`。 |
| `toUnixSeconds` | `(dt: HttpDateTime): i64` | 调用 `toUnixSeconds` 并返回 `i64`。 |
| `fromUnixSeconds` | `(ts: i64): HttpDateTime` | 调用 `fromUnixSeconds` 并返回 `HttpDateTime`。 |
| `weekdayName` | `(wd: u8): string` | 调用 `weekdayName` 并返回 `string`。 |
| `monthName` | `(m: u8): string` | 调用 `monthName` 并返回 `string`。 |
| `formatImfFixdate` | `(dt: HttpDateTime, out: []u8): usize` | 通过 `formatImfFixdate` 编码或格式化输出，返回 `usize`。 |
| `formatUnixHttpDate` | `(ts: i64, out: []u8): usize` | 通过 `formatUnixHttpDate` 编码或格式化输出，返回 `usize`。 |
| `parseUnixSeconds` | `(input: string): i64` | 通过 `parseUnixSeconds` 解析输入数据，返回 `i64`。 |

## 最小调用示例

```cp
import { parse2Digits } from "httpdate";

pub fn main() {
    let result = parse2Digits("example", 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
