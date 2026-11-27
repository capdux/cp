[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# ndjson

Zero-heap NDJSON / JSON Lines streaming incremental line splitter, scanner, and writer with max-line overflow protection.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:ndjson` ([https://github.com/ndjson/ndjson.js](https://github.com/ndjson/ndjson.js))

| 元信息 | 值 |
| --- | --- |
| 包名 | `ndjson` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add ndjson
```

```cp
import {
    NdjsonLine,
    NdjsonSplitter,
    isBlankSlice,
    nextLine,
    countLines,
    writeLine,
} from "ndjson";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `NdjsonLine` | `class` | 字段: `start: usize, len: usize, found: bool` |
| `NdjsonSplitter` | `class` | 字段: `buf: [1024]u8, pos: usize, ready_len: usize, overflowed: bool` · 方法: `init, reset, pushByte, currentLine, flush` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `isBlankSlice` | `(s: &[]u8): bool` | 执行 `isBlankSlice` 条件判定，返回 `bool`。 |
| `nextLine` | `(src: &[]u8, start_pos: usize, out_next_pos: &mut usize): NdjsonLine` | 调用 `nextLine` 并返回 `NdjsonLine`。 |
| `countLines` | `(src: &[]u8): usize` | 调用 `countLines` 并返回 `usize`。 |
| `writeLine` | `(json_record: &[]u8, dst: []u8): i64` | 调用 `writeLine` 并返回 `i64`。 |

## 最小调用示例

```cp
import { isBlankSlice } from "ndjson";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = isBlankSlice(out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
