[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# pretty-format

Zero-heap pure CP structured value pretty-printer with indentation, compact mode, and max-depth protection (clean-room MIT implementation of npm pretty-format)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:pretty-format` ([https://github.com/jestjs/jest](https://github.com/jestjs/jest))

| 元信息 | 值 |
| --- | --- |
| 包名 | `pretty-format` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add pretty-format
```

```cp
import {
    PrettyOptions,
    PrettyWriter,
    pfmtWriteByte,
    pfmtWriteSpaces,
    pfmtWriteF64,
    pfmtWriteStringLiteral,
    formatStringLiteral,
    formatPrimitiveInt,
} from "pretty-format";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `PrettyOptions` | `class` | 字段: `indent: usize, max_depth: usize, min: bool, escape_string: bool, print_basic_prototype: bool` · 方法: `defaults, compact` |
| `PrettyWriter` | `class` | 字段: `opts: PrettyOptions, buf: [2048]u8, len: usize, depth: usize, item_counts: [32]usize, truncated_depth: usize` · 方法: `new, defaultWriter, clear, isTruncated, writeNull, writeUndefined, +12 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `pfmtWriteByte` | `(out: []u8, pos: usize, b: u8): usize` | 调用 `pfmtWriteByte` 并返回 `usize`。 |
| `pfmtWriteSpaces` | `(out: []u8, pos: usize, count: usize): usize` | 调用 `pfmtWriteSpaces` 并返回 `usize`。 |
| `pfmtWriteF64` | `(out: []u8, pos: usize, val: f64): usize` | 调用 `pfmtWriteF64` 并返回 `usize`。 |
| `pfmtWriteStringLiteral` | `(out: []u8, pos: usize, s: string, escape_string: bool): usize` | 调用 `pfmtWriteStringLiteral` 并返回 `usize`。 |
| `formatStringLiteral` | `(s: string, escape: bool, out: []u8): usize` | 通过 `formatStringLiteral` 编码或格式化输出，返回 `usize`。 |
| `formatPrimitiveInt` | `(v: i64, out: []u8): usize` | 通过 `formatPrimitiveInt` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { formatStringLiteral } from "pretty-format";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatStringLiteral("example", false, out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
