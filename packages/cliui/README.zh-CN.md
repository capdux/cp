[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# cliui

Zero-allocation multi-column CLI layout engine with ANSI-aware wrapping, alignment, and padding in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:cliui` ([https://github.com/yargs/cliui](https://github.com/yargs/cliui))

| 元信息 | 值 |
| --- | --- |
| 包名 | `cliui` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add cliui
```

```cp
import {
    Align,
    Column,
    CliUi,
    alignToU8,
    cuWriteByte,
    cuWriteBytes,
    cuWriteSpaces,
    cuVisibleWidth,
} from "cliui";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `Align` | `Left, Right, Center` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Column` | `class` | 字段: `text: string, width: usize, alignment: u8, pad_left: usize, pad_right: usize, border: bool` · 方法: `plain, sized` |
| `CliUi` | `class` | 字段: `width: usize, len: usize, row_count: usize, in_span: bool, buf: [CU_BUF_CAP]u8` · 方法: `new, resetOutput, div, span, render` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `alignToU8` | `(a: Align): u8` | 调用 `alignToU8` 并返回 `u8`。 |
| `cuWriteByte` | `(out: []u8, pos: usize, b: u8): usize` | 调用 `cuWriteByte` 并返回 `usize`。 |
| `cuWriteBytes` | `(out: []u8, pos: usize, s: string): usize` | 调用 `cuWriteBytes` 并返回 `usize`。 |
| `cuWriteSpaces` | `(out: []u8, pos: usize, count: usize): usize` | 调用 `cuWriteSpaces` 并返回 `usize`。 |
| `cuVisibleWidth` | `(s: string): usize` | 调用 `cuVisibleWidth` 并返回 `usize`。 |
| `cuCountLines` | `(s: string): usize` | 调用 `cuCountLines` 并返回 `usize`。 |
| `cuGetLine` | `(s: string, target_line: usize): string` | 调用 `cuGetLine` 并返回 `string`。 |
| `cuResolveWidths` | `(total_width: usize, cols: &[]Column, n_cols: usize, widths: []usize)` | 就地执行 `cuResolveWidths` 操作。 |
| `formatTwoCol` | `(left: string, right: string, left_width: usize, total_width: usize, out: []u8): usize` | 通过 `formatTwoCol` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { formatTwoCol } from "cliui";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatTwoCol("example", "example", 0, 0, out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
