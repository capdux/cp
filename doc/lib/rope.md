# `cap:rope` (`cap.rope`) — Piece Table / Rope 大文本编辑器数据结构与行号映射标准库

## 1. 概述

`cap:rope`（或通过 `import cap from "cap"` 动态挂载为 `cap.rope`）提供面向大文本编辑器、IDE 缓冲区与流式文档处理的高性能 **Piece Table / Rope** 文本数据结构：

- **O(1) 尾部连续追加合并**：连续尾部/边界追加自动合并到同一个 `Add` 片段，大幅减少碎片数量。
- **全量编辑原语**：支持任意偏移 `insert`、`deleteRange`、`replaceRange`、`byteAt`、`slice`、`toString`。
- **快速行号 / 列号互转**：每个片段维护换行符计数，支持 `\n` 与 `\r\n` 换行符，`lineCount`、`getLine`、`offsetToLineCol`、`lineColToOffset` 精确映射。
- **Undo / Redo 编辑事务栈**：每次编辑记录轻量级逆操作快照，支持多步 `undo()` / `redo()`，新编辑自动清空 Redo 栈，零内存泄漏。

## 2. 核心类型

- `RopeError`：`OutOfBounds | OutOfMemory`
- `LineCol`：`{ line: usize, col: usize }`（0-indexed 行号与字节列号）
- `BufferKind`：`Original | Add`
- `Piece`：`{ kind: BufferKind, start: usize, len: usize, line_breaks: usize }`
- `RopeText`：核心 Piece Table 文本容器与 Undo/Redo 管理器

## 3. API 速查

| 方法 | 签名 | 说明 |
| :--- | :--- | :--- |
| `RopeText.initFromText` | `(alloc, initial_text: []const u8): !RopeText` | 从初始文本构造（内部拷贝 `initial_text`） |
| `deinit` | `(*_)` | 释放所有内部缓冲区、片段与 Undo/Redo 栈 |
| `len` | `(): usize` | 返回当前文本总字节数 |
| `pieceCount` | `(): usize` | 返回当前内部 Piece 片段数量 |
| `lineCount` | `(): usize` | 返回总行数（至少为 1） |
| `byteAt` | `(offset: usize): !u8` | 读取指定字节偏移的字节 |
| `slice` | `(alloc, start: usize, end: usize): ![]u8` | 提取 `[start, end)` 区间字节切片（调用方释放） |
| `toString` | `(alloc): ![]u8` | 导出完整文本（调用方释放） |
| `insert` | `(*_, offset: usize, text: []const u8): !void` | 在指定偏移插入文本 |
| `deleteRange` | `(*_, start: usize, del_len: usize): !void` | 删除 `[start, start + del_len)` |
| `replaceRange` | `(*_, start: usize, del_len: usize, text: []const u8): !void` | 原子替换区间文本（单步 Undo） |
| `offsetToLineCol` | `(offset: usize): !LineCol` | 字节偏移转 `(line, col)` |
| `lineColToOffset` | `(line: usize, col: usize): !usize` | `(line, col)` 转字节偏移 |
| `getLine` | `(alloc, line_idx: usize): ![]u8` | 提取指定行内容（自动剥离 `\r\n` 末尾 `\r`） |
| `undo` / `redo` | `(*_): !bool` | 执行撤销/重做，无历史时返回 `false` |
| `canUndo` / `canRedo` | `(): bool` | 查询是否可撤销/重做 |

## 4. 示例

```cp
import { RopeText } from "cap:rope";
const std = @import("std");

let rope = try RopeText.initFromText(std.heap.page_allocator, "hello\nworld");
defer rope.deinit();

try rope.insert(5, ", CP");
try rope.replaceRange(0, 5, "Hi");
const line0 = try rope.getLine(std.heap.page_allocator, 0);
defer std.heap.page_allocator.free(line0);
// line0 == "Hi, CP"
_ = try rope.undo();
```
