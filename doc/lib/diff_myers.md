# `cap:diff_myers` 标准库参考

## 概述

`cap:diff_myers`（`lib/diff_myers/`）是纯 CP 实现的工业级 Myers 最短编辑脚本（Shortest Edit Script, SES）行级 Diff 引擎，提供精确的 `Equal`/`Insert`/`Delete` 操作序列生成、新文本重构还原以及标准 Unified Diff 格式化输出，支持 `import { ... } from "cap:diff_myers"` 与 `cap.diff_myers` 动态挂载。

## 核心类型

- `DiffOpKind`: 枚举类型，包含 `Equal`、`Insert`、`Delete`。
- `DiffOp`: 单行操作结构体：
  - `kind: DiffOpKind`
  - `line: []const u8`
  - `old_line_no: usize?`（`Insert` 时为 `null`）
  - `new_line_no: usize?`（`Delete` 时为 `null`）
- `DiffResult`: Diff 结果结构体：
  - `ops: []DiffOp`
  - `added: usize`
  - `deleted: usize`
  - `deinit(self: *DiffResult, alloc: std.mem.Allocator)`: 释放 `ops` 切片。

## API 速查

- `diffLines(alloc: std.mem.Allocator, old_text: []const u8, new_text: []const u8) !DiffResult`
  计算 `old_text` 到 `new_text` 的 Myers 最短编辑脚本。
- `reconstructNew(alloc: std.mem.Allocator, ops: []const DiffOp) ![]u8`
  根据 `DiffOp` 序列 100% 还原新文本（保留非 `Delete` 行并以 `\n` 拼接）。
- `formatUnifiedDiff(alloc: std.mem.Allocator, old_name: []const u8, new_name: []const u8, old_text: []const u8, new_text: []const u8, context_lines: usize) ![]u8`
  生成标准 `---` / `+++` / `@@ -a,b +c,d @@` 格式的 Unified Diff 文本；无差异时返回空切片。

## 示例

```cp
import { diffLines, reconstructNew, formatUnifiedDiff } from "cap:diff_myers";

let res = try diffLines(alloc, "a\nb\nc", "a\nx\nc");
defer res.deinit(alloc);

const restored = try reconstructNew(alloc, res.ops);
defer alloc.free(restored);

const patch = try formatUnifiedDiff(alloc, "a.txt", "b.txt", "a\nb\nc", "a\nx\nc", 1);
defer alloc.free(patch);
```
