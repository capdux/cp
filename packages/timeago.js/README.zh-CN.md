[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# timeago.js

Zero-allocation deterministic relative time formatter (EN/ZH, Long/Short/Compact) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:timeago.js` ([https://github.com/hustcc/timeago.js](https://github.com/hustcc/timeago.js))（原库名 `timeago.js` → CP 包名 `timeago.js`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `timeago.js` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add timeago.js
```

```cp
import {
    TimeagoLocale,
    TimeagoStyle,
    TimeUnit,
    ScaledDelta,
    absU64,
    appendByte,
    appendStr,
    appendU64,
} from "timeago.js";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `TimeagoLocale` | `En, Zh` |
| `enum` | `TimeagoStyle` | `Long, Short` |
| `enum` | `TimeUnit` | `Second, Minute, Hour, Day, Week, Month, Year` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ScaledDelta` | `class` | 字段: `value: u64, unit: TimeUnit` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `absU64` | `(n: i64): u64` | 调用 `absU64` 并返回 `u64`。 |
| `appendByte` | `(out: []u8, pos: usize, c: u8): usize` | 调用 `appendByte` 并返回 `usize`。 |
| `appendStr` | `(out: []u8, pos: usize, s: string): usize` | 调用 `appendStr` 并返回 `usize`。 |
| `appendU64` | `(out: []u8, pos: usize, val: u64): usize` | 调用 `appendU64` 并返回 `usize`。 |
| `scaleDelta` | `(abs_sec: u64): ScaledDelta` | 调用 `scaleDelta` 并返回 `ScaledDelta`。 |
| `enLongUnit` | `(unit: TimeUnit, plural: bool): string` | 调用 `enLongUnit` 并返回 `string`。 |
| `enShortUnit` | `(unit: TimeUnit): string` | 调用 `enShortUnit` 并返回 `string`。 |
| `zhLongUnit` | `(unit: TimeUnit): string` | 调用 `zhLongUnit` 并返回 `string`。 |
| `zhShortUnit` | `(unit: TimeUnit): string` | 调用 `zhShortUnit` 并返回 `string`。 |
| `formatTimeago` | `(delta_sec: i64, locale: TimeagoLocale, style: TimeagoStyle, out: []u8): string` | 通过 `formatTimeago` 编码或格式化输出，返回 `string`。 |
| `formatBetween` | `(target_sec: i64, now_sec: i64, locale: TimeagoLocale, style: TimeagoStyle, out: []u8): string` | 通过 `formatBetween` 编码或格式化输出，返回 `string`。 |
| `formatTimeagoEn` | `(delta_sec: i64, out: []u8): string` | 通过 `formatTimeagoEn` 编码或格式化输出，返回 `string`。 |
| `formatTimeagoZh` | `(delta_sec: i64, out: []u8): string` | 通过 `formatTimeagoZh` 编码或格式化输出，返回 `string`。 |
| `formatCompact` | `(delta_sec: i64, out: []u8): string` | 通过 `formatCompact` 编码或格式化输出，返回 `string`。 |

## 最小调用示例

```cp
import { appendStr } from "timeago.js";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = appendStr(out_buf[0..256], 0, "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
