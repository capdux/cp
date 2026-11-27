[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# jest-diff

Zero-heap pure CP Jest-style unified diff engine with ANSI coloring (clean-room MIT implementation of npm jest-diff)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:jest-diff` ([https://github.com/jestjs/jest](https://github.com/jestjs/jest))

| 元信息 | 值 |
| --- | --- |
| 包名 | `jest-diff` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add jest-diff
```

```cp
import {
    JdfLine,
    DiffOptions,
    DiffSummary,
    jdfSplitLines,
    jdfWriteBytes,
    jdfWriteStr,
    jdfWriteByte,
    jdfWriteUsize,
} from "jest-diff";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `JdfLine` | `class` | 字段: `start: usize, end: usize` |
| `DiffOptions` | `class` | 字段: `a_annotation: string, b_annotation: string, context_lines: usize, expand: bool, include_change_counts: bool, use_color: bool` · 方法: `defaults, colored` |
| `DiffSummary` | `class` | 字段: `insertions: usize, deletions: usize, equal: usize, has_diff: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `jdfSplitLines` | `(text: string, out_lines: []JdfLine): usize` | 调用 `jdfSplitLines` 并返回 `usize`。 |
| `jdfWriteBytes` | `(out: []u8, pos: usize, src: string, start: usize, end: usize): usize` | 调用 `jdfWriteBytes` 并返回 `usize`。 |
| `jdfWriteStr` | `(out: []u8, pos: usize, s: string): usize` | 调用 `jdfWriteStr` 并返回 `usize`。 |
| `jdfWriteByte` | `(out: []u8, pos: usize, b: u8): usize` | 调用 `jdfWriteByte` 并返回 `usize`。 |
| `jdfWriteUsize` | `(out: []u8, pos: usize, val: usize): usize` | 调用 `jdfWriteUsize` 并返回 `usize`。 |
| `jdfEmitStyledLine` | `(fmt: PicoFormatter, color_kind: u8, line_text: string, out: []u8, pos: usize): usize` | 调用 `jdfEmitStyledLine` 并返回 `usize`。 |
| `diffStats` | `(expected: string, received: string): DiffSummary` | 调用 `diffStats` 并返回 `DiffSummary`。 |
| `diffLinesUnified` | `(expected: string, received: string, opts: DiffOptions, out: []u8): usize` | 调用 `diffLinesUnified` 并返回 `usize`。 |

## 最小调用示例

```cp
import { jdfSplitLines } from "jest-diff";

pub fn main() {
    let result = jdfSplitLines("example", /* out_lines: []JdfLine */);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
