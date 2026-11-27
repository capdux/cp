[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# magic-string

Zero-allocation original-offset source string editor (npm magic-string parity) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`magic-string`](https://github.com/Rich-Harris/magic-string)（`MIT`）（包名 `magic-string`）。

| 元信息 | 值 |
| --- | --- |
| 包名 | `magic-string` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add magic-string
```

```cp
import {
    MstrChunk,
    MstrInsert,
    MagicString,
    mstrWriteStr,
    mstrWriteLeftInserts,
    mstrWriteRightInserts,
} from "magic-string";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `MstrChunk` | `class` | 字段: `start: usize, end: usize, content: string, edited: bool` |
| `MstrInsert` | `class` | 字段: `index: usize, content: string` |
| `MagicString` | `class` | 字段: `original: string, changed: bool, chunks: [64]MstrChunk, chunk_count: usize, global_intros: [16]string, global_intro_len: usize, +6 项` · 方法: `new, prepend, append, splitAt, overwrite, remove, +6 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `mstrWriteStr` | `(src: string, out: []u8, pos: usize): usize` | 调用 `mstrWriteStr` 并返回 `usize`。 |
| `mstrWriteLeftInserts` | `(inserts: [32]MstrInsert, count: usize, target_idx: usize, out: []u8, pos: usize): usize` | 调用 `mstrWriteLeftInserts` 并返回 `usize`。 |
| `mstrWriteRightInserts` | `(inserts: [32]MstrInsert, count: usize, target_idx: usize, out: []u8, pos: usize): usize` | 调用 `mstrWriteRightInserts` 并返回 `usize`。 |

## 最小调用示例

```cp
import { mstrWriteStr } from "magic-string";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = mstrWriteStr("example", out_buf[0..256], 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
