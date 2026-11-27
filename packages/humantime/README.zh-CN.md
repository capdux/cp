[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# humantime

Zero-heap human-readable duration parser/formatter and RFC 3339 timestamp codec (clean-room CP implementation of crates.io humantime)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:humantime` ([https://github.com/tailhook/humantime](https://github.com/tailhook/humantime))

| 元信息 | 值 |
| --- | --- |
| 包名 | `humantime` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add humantime
```

```cp
import {
    DurationParseResult,
    htIsSpace,
    htIsDigit,
    htIsAlpha,
    htEq,
    htUnitNanos,
    parseDuration,
    htPushStr,
} from "humantime";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `DurationParseResult` | `class` | 字段: `ok: bool, nanos: u64` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `htIsSpace` | `(c: u8): bool` | 调用 `htIsSpace` 并返回 `bool`。 |
| `htIsDigit` | `(c: u8): bool` | 调用 `htIsDigit` 并返回 `bool`。 |
| `htIsAlpha` | `(c: u8): bool` | 调用 `htIsAlpha` 并返回 `bool`。 |
| `htEq` | `(s: string, start: usize, end: usize, lit: string): bool` | 调用 `htEq` 并返回 `bool`。 |
| `htUnitNanos` | `(s: string, start: usize, end: usize): u64` | 调用 `htUnitNanos` 并返回 `u64`。 |
| `parseDuration` | `(input: string): DurationParseResult` | 通过 `parseDuration` 解析输入数据，返回 `DurationParseResult`。 |
| `htPushStr` | `(out: []u8, pos: usize, s: string): usize` | 调用 `htPushStr` 并返回 `usize`。 |
| `htPushPart` | `(out: []u8, pos: usize, val: u64, suffix: string): usize` | 调用 `htPushPart` 并返回 `usize`。 |
| `formatDuration` | `(nanos: u64, out: []u8): usize` | 通过 `formatDuration` 编码或格式化输出，返回 `usize`。 |
| `htPush2Digits` | `(out: []u8, pos: usize, v: u64): usize` | 调用 `htPush2Digits` 并返回 `usize`。 |
| `htPush4Digits` | `(out: []u8, pos: usize, v: u64): usize` | 调用 `htPush4Digits` 并返回 `usize`。 |
| `formatRfc3339Seconds` | `(unix_secs: u64, out: []u8): usize` | 通过 `formatRfc3339Seconds` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { parseDuration } from "humantime";

pub fn main() {
    let result = parseDuration("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
