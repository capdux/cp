[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# braces

Zero-allocation Bash/Glob brace expansion and sequence range generator (braces + fill-range parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:braces` ([https://github.com/micromatch/braces](https://github.com/micromatch/braces))

| 元信息 | 值 |
| --- | --- |
| 包名 | `braces` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add braces
```

```cp
import {
    BraceResult,
    BrExpander,
    BrInt,
    BrRangeSpec,
    BrSpan,
    brInitExpander,
    brToResult,
    brPushItem,
} from "braces";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `BraceResult` | `class` | 字段: `buf: []u8, starts: [BR_MAX_ITEMS]usize, lens: [BR_MAX_ITEMS]usize, count: usize, truncated: bool` · 方法: `itemAt` |
| `BrExpander` | `class` | 字段: `out: []u8, pos: usize, starts: [BR_MAX_ITEMS]usize, lens: [BR_MAX_ITEMS]usize, count: usize, truncated: bool` |
| `BrInt` | `class` | 字段: `ok: bool, val: i64` |
| `BrRangeSpec` | `class` | 字段: `ok: bool, startStr: string, endStr: string, step: i32` |
| `BrSpan` | `class` | 字段: `found: bool, openIdx: usize, closeIdx: usize, isCommaSet: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `brInitExpander` | `(out: []u8): BrExpander` | 调用 `brInitExpander` 并返回 `BrExpander`。 |
| `brToResult` | `(e: &mut BrExpander): BraceResult` | 调用 `brToResult` 并返回 `BraceResult`。 |
| `brPushItem` | `(e: &mut BrExpander, s: string)` | 就地执行 `brPushItem` 操作。 |
| `brIsDigit` | `(c: u8): bool` | 调用 `brIsDigit` 并返回 `bool`。 |
| `brIsUpper` | `(c: u8): bool` | 调用 `brIsUpper` 并返回 `bool`。 |
| `brIsLower` | `(c: u8): bool` | 调用 `brIsLower` 并返回 `bool`。 |
| `brParseInt` | `(s: string): BrInt` | 调用 `brParseInt` 并返回 `BrInt`。 |
| `brFormatSimpleInt` | `(v: i64, buf: &mut [32]u8): string` | 调用 `brFormatSimpleInt` 并返回 `string`。 |
| `brParseRangeSpec` | `(body: string): BrRangeSpec` | 调用 `brParseRangeSpec` 并返回 `BrRangeSpec`。 |
| `brHasTopLevelComma` | `(body: string): bool` | 调用 `brHasTopLevelComma` 并返回 `bool`。 |
| `brFindExpandableBrace` | `(pat: string): BrSpan` | 调用 `brFindExpandableBrace` 并返回 `BrSpan`。 |
| `brBuildCombined` | `(prefix: string, mid: string, suffix: string, buf: &mut [BR_MAX_WORK_BUF]u8): usize` | 调用 `brBuildCombined` 并返回 `usize`。 |
| `brEmitToken` | `(e: &mut BrExpander, prefix: string, tok: string, suffix: string, depth: usize)` | 就地执行 `brEmitToken` 操作。 |
| `brExpandRangeWithContext` | `(e: &mut BrExpander, prefix: string, startStr: string, endStr: string, stepIn: i32, suffix: string, depth: usize)` | 就地执行 `brExpandRangeWithContext` 操作。 |
| `brExpandRec` | `(e: &mut BrExpander, pat: string, depth: usize)` | 就地执行 `brExpandRec` 操作。 |
| `expandBraces` | `(pattern: string, out: []u8): BraceResult` | 调用 `expandBraces` 并返回 `BraceResult`。 |
| `fillRange` | `(start: string, end: string, step: i32, out: []u8): BraceResult` | 调用 `fillRange` 并返回 `BraceResult`。 |

## 最小调用示例

```cp
import { brParseInt } from "braces";

pub fn main() {
    let result = brParseInt("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
