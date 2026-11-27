[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# slice-ansi

Zero-allocation ANSI-preserving terminal column slicer and truncator with CJK double-width support in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:slice-ansi` ([https://github.com/chalk/slice-ansi](https://github.com/chalk/slice-ansi))

| 元信息 | 值 |
| --- | --- |
| 包名 | `slice-ansi` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add slice-ansi
```

```cp
import {
    SaUtf8Char,
    SaStyleState,
    saDecodeUtf8,
    saWriteByte,
    saWriteBytes,
    saWriteU16,
    saWriteSgrParam,
    saEmitActiveStyle,
} from "slice-ansi";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `SaUtf8Char` | `class` | 字段: `cp: u32, byte_len: usize` |
| `SaStyleState` | `class` | 字段: `bold: bool, dim: bool, italic: bool, underline: bool, blink: bool, inverse: bool, +12 项` · 方法: `empty, reset, hasAttr, hasAny, applySgr` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `saDecodeUtf8` | `(s: string, pos: usize): SaUtf8Char` | 调用 `saDecodeUtf8` 并返回 `SaUtf8Char`。 |
| `saWriteByte` | `(out: []u8, pos: usize, b: u8): usize` | 调用 `saWriteByte` 并返回 `usize`。 |
| `saWriteBytes` | `(out: []u8, pos: usize, src: string): usize` | 调用 `saWriteBytes` 并返回 `usize`。 |
| `saWriteU16` | `(out: []u8, pos: usize, val: u16): usize` | 调用 `saWriteU16` 并返回 `usize`。 |
| `saWriteSgrParam` | `(out: []u8, pos: usize, first: &mut bool, code: u16): usize` | 调用 `saWriteSgrParam` 并返回 `usize`。 |
| `saEmitActiveStyle` | `(out: []u8, pos: usize, st: SaStyleState): usize` | 调用 `saEmitActiveStyle` 并返回 `usize`。 |
| `saEmitCloseStyle` | `(out: []u8, pos: usize, st: SaStyleState): usize` | 调用 `saEmitCloseStyle` 并返回 `usize`。 |
| `ansiVisibleWidth` | `(input: string): usize` | 调用 `ansiVisibleWidth` 并返回 `usize`。 |
| `sliceAnsi` | `(input: string, begin: usize, end: usize, out: []u8): usize` | 调用 `sliceAnsi` 并返回 `usize`。 |
| `sliceAnsiFrom` | `(input: string, begin: usize, out: []u8): usize` | 调用 `sliceAnsiFrom` 并返回 `usize`。 |
| `truncateAnsi` | `(input: string, max_cols: usize, ellipsis: string, out: []u8): usize` | 调用 `truncateAnsi` 并返回 `usize`。 |

## 最小调用示例

```cp
import { saDecodeUtf8 } from "slice-ansi";

pub fn main() {
    let result = saDecodeUtf8("example", 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
