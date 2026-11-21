# `cap:line_offset_table` — 基于二分查找的源码行列偏移表

> 溯源：从 `/data/dev/bun/src/sourcemap/LineOffsetTable.cp` 吸收并规范化为纯 CP 标准库模块。

`cap:line_offset_table` 提供编译器、LSP 与 SourceMap 所需的 O(log N) 行列定位引擎：

- 单遍扫描构建每行起始偏移表，原生识别 **LF (`\n`)、CRLF (`\r\n`) 与 CR (`\r`)** 三种换行规范；
- 支持字节偏移 `offset -> (line, column)` 二分定位与 `(line, column) -> offset` 反向换算，以及剥离换行符的 `lineSlice(source, line)` 行内容提取；
- 实现 `~init()` 确定性 RAII 析构，离开作用域自动释放底层偏移数组，零内存泄漏。

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
| `findLine` | `fn findLine(offset: usize): usize` | O(log N) 二分查找字节偏移所在 0-based 行号 |
| `offsetToLineCol` | `fn offsetToLineCol(offset: usize): LineCol?` | 字节偏移 -> `(line, column)`（越界返回 `null`） |
| `lineColToOffset` | `fn lineColToOffset(line: usize, column: usize): usize?` | `(line, column)` -> 字节偏移（行/列越界返回 `null`） |
| `lineRange` | `fn lineRange(source: string, line: usize): LineRange?` | 获取指定行的内容范围（自动剥离尾部换行） |
| `lineSlice` | `fn lineSlice(source: string, line: usize): string?` | 提取指定行的纯内容切片（不含换行符） |

---

## 使用示例

```cp
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
