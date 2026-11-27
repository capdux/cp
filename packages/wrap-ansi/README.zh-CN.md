[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# wrap-ansi

Zero-allocation ANSI-preserving word/hard wrapper and visible-column slicer with CJK double-width awareness (wrap-ansi / slice-ansi parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:wrap-ansi` ([https://github.com/chalk/wrap-ansi](https://github.com/chalk/wrap-ansi))

| 元信息 | 值 |
| --- | --- |
| 包名 | `wrap-ansi` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add wrap-ansi
```

```cp
import {
    Utf8Step,
    AnsiStyleState,
    decodeUtf8At,
    codepointWidth,
    scanAnsiCsi,
    isResetSgr,
    startsWithZeroReset,
    pushByte,
} from "wrap-ansi";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Utf8Step` | `class` | 字段: `cp: u32, width: usize` |
| `AnsiStyleState` | `class` | 字段: `seqs: [MAX_ACTIVE_SGR]string, count: usize` · 方法: `empty, hasActive, applySeq, emitActive` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `decodeUtf8At` | `(s: string, i: usize): Utf8Step` | 通过 `decodeUtf8At` 解析输入数据，返回 `Utf8Step`。 |
| `codepointWidth` | `(cp: u32): usize` | 调用 `codepointWidth` 并返回 `usize`。 |
| `scanAnsiCsi` | `(s: string, i: usize): usize` | 调用 `scanAnsiCsi` 并返回 `usize`。 |
| `isResetSgr` | `(seq: string): bool` | 执行 `isResetSgr` 条件判定，返回 `bool`。 |
| `startsWithZeroReset` | `(seq: string): bool` | 调用 `startsWithZeroReset` 并返回 `bool`。 |
| `pushByte` | `(out: []u8, pos: usize, b: u8): usize` | 写入或更新状态数据。 |
| `pushStr` | `(out: []u8, pos: usize, s: string): usize` | 写入或更新状态数据。 |
| `visibleWidth` | `(s: string): usize` | 调用 `visibleWidth` 并返回 `usize`。 |
| `emitLineBreak` | `(out: []u8, pos: usize, style: AnsiStyleState): usize` | 调用 `emitLineBreak` 并返回 `usize`。 |
| `wrapAnsi` | `(text: string, max_cols: usize, hard_wrap: bool, out: []u8): string` | 调用 `wrapAnsi` 并返回 `string`。 |
| `sliceAnsi` | `(text: string, start_col: usize, end_col: usize, out: []u8): string` | 调用 `sliceAnsi` 并返回 `string`。 |

## 最小调用示例

```cp
import { decodeUtf8At } from "wrap-ansi";

pub fn main() {
    let result = decodeUtf8At("example", 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
