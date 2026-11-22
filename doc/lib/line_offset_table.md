# `cap:line_offset_table` — 基于二分查找的源码行列偏移表

> 参考来源：/data/dev/bun/src/sourcemap/LineOffsetTable.cp | 吸收日期：2026-11-22

## 概述

`cap:line_offset_table` 吸收自 Bun SourceMap 与诊断引擎核心组件（`src/sourcemap/LineOffsetTable.cp`），提供编译器、LSP 与 SourceMap 所需的 $O(\log N)$ 行列定位能力。

## 上游算法原理与行为契约

- **单遍扫描构建与三换行符兼容**：`LineOffsetTable.build` 单遍扫描源码字节流，记录每一行的起始字节偏移，原生识别 **LF (`\n`)、CRLF (`\r\n`) 与旧式 CR (`\r`)** 三种换行规范，且不会将 `\r\n` 误计为两行。
- **$O(\log N)$ 二分查找（吸收自 Bun `LineOffsetTable.findLine`）**：所有行起始偏移严格单调递增，查询任意字节偏移所在行号（`findLine` / `offsetToLineCol`）均通过二分查找完成，即使面对数万行单文件也能在纳秒级定位。
- **0-based 内部存储 + 1-based 展示换算**：遵循 LSP 与 SourceMap 规范，内部坐标统一采用 0-based（`LineCol.line`、`LineCol.column`），同时提供 `line1()` / `column1()` 便捷方法用于编译器错误诊断展示。
- **确定性 RAII 生命周期**：实现 `~init()` 析构函数，离开作用域自动释放底层偏移数组，零内存泄漏。

---

## 快速导入

```cp
import {
    LineOffsetTable,
    LineCol,
    LineRange,
    Error,
} from "cap:line_offset_table";
```

---

## API 参考

### 1. `LineCol` & `LineRange`

| 类型 | 字段 / 方法 | 说明 |
| :--- | :--- | :--- |
| `LineCol` | `line: usize`, `column: usize`, `line1()`, `column1()`, `eql(other)` | 0-based 行列坐标及 1-based 展示换算 |
| `LineRange` | `start: usize`, `end: usize`, `end_with_newline: usize` | 单行起止字节偏移（含/不含换行符） |

### 2. `LineOffsetTable`（含 `~init()` RAII 析构）

| 方法 | 签名 | 说明 |
| :--- | :--- | :--- |
| `build` | `static fn build(allocator, source: string): LineOffsetTable!` | 单遍扫描源码构建行偏移表 |
| `lineCount` / `sourceLen` | `fn lineCount(): usize` / `fn sourceLen(): usize` | 总行数与源码字节长度 |
| `lineStart` | `fn lineStart(line: usize): usize?` | 第 `line` 行起始字节偏移（越界返回 `null`） |
| `findLine` | `fn findLine(offset: usize): usize` | $O(\log N)$ 二分查找字节偏移所在 0-based 行号 |
| `offsetToLineCol` | `fn offsetToLineCol(offset: usize): LineCol?` | 字节偏移 -> `(line, column)`（越界返回 `null`） |
| `lineColToOffset` | `fn lineColToOffset(line: usize, column: usize): usize?` | `(line, column)` -> 字节偏移（行/列越界返回 `null`） |
| `lineRange` | `fn lineRange(source: string, line: usize): LineRange?` | 获取指定行的内容范围（自动剥离尾部换行） |
| `lineSlice` | `fn lineSlice(source: string, line: usize): string?` | 提取指定行的纯内容切片（不含换行符） |

---

## 边界注意事项

1. **空源码处理**：空字符串 `""` 至少包含第 `0` 行（起始偏移 `0`），`offsetToLineCol(0)` 返回 `{ line: 0, column: 0 }`。
2. **字节列偏移语义**：`LineCol.column` 表示自当前行首起的 UTF-8 字节偏移；若需 UTF-16 代码单元列号（如 LSP 协议），调用方可在 `lineSlice` 上进一步换算。
3. **越界安全**：`offset > sourceLen()` 或 `line >= lineCount()` 时查询接口均安全返回 `null`，不触发越界 panic。

## 使用示例

```cp
import std from "std";
import { LineOffsetTable } from "cap:line_offset_table";

fn locateError(alloc: std.mem.Allocator, src: string, byte_off: usize)! {
    const table = LineOffsetTable.build(alloc, src)?;
    if (lc := table.offsetToLineCol(byte_off)) {
        const line_num = lc.line1();
        const col_num = lc.column1();
        * = line_num;
        * = col_num;
    }
    if (text := table.lineSlice(src, 0)) {
        * = text;
    }
} // 离开作用域自动触发 table.~init() 释放内存
```
