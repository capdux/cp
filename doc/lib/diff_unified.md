# `cap:diff_unified` — 零堆分配行级 Unified Diff 计算与格式化库

> 来源：吸收自 `/data/dev/bun/src/test_runner/diff/printDiff.cp` 与 `diff_format.cp`

`cap:diff_unified` 提供纯 CP、零外部依赖、零堆分配的行级最小编辑脚本（LCS 动态规划）计算、差异统计与标准 Unified Diff（`diff -u`）Hunk 合并格式化能力。

## 核心类型

### `DiffOpKind` & `DiffLine`

```cp
pub enum DiffOpKind: u8 {
    equal = 0,
    delete = 1,
    insert = 2,

    fn prefix(): u8 // ' ' | '-' | '+'
}

pub struct DiffLine {
    op: DiffOpKind = .equal,
    old_line: usize = 0, // 1-based（insert 时为 0）
    new_line: usize = 0, // 1-based（delete 时为 0）
    text: []const u8 = "",
}
```

### `DiffSummary`

```cp
pub struct DiffSummary {
    added: usize = 0,
    removed: usize = 0,
    unchanged: usize = 0,

    fn isIdentical(): bool
}
```

## 核心函数

### `diffLines`

```cp
pub fn diffLines(old_text: []const u8, new_text: []const u8, out_ops: []DiffLine): usize
```

- 基于栈上 65×65 LCS 动态规划表计算精确行级最小编辑脚本（单侧支持最多 64 行，零堆分配，自动兼容 `\n` 与 `\r\n`）。
- 写入 `out_ops` 并返回实际写入条目数。

### `summarize`

```cp
pub fn summarize(ops: []const DiffLine): DiffSummary
```

- 统计差异脚本中的 `added`、`removed` 与 `unchanged` 行数。

### `formatUnified`

```cp
pub fn formatUnified(
    old_name: []const u8,
    new_name: []const u8,
    ops: []const DiffLine,
    context_lines: usize,
    out: []u8,
): ([]const u8)?
```

- 输出标准 `--- <old_name>\n+++ <new_name>\n@@ -l1,c1 +l2,c2 @@\n` Hunk 格式，自动合并上下文重叠或相邻的 Hunk。
- 两段文本完全相同时返回空切片；若 `out` 缓冲区容量不足返回 `null`。

## 使用示例

```cp
import { DiffLine, diffLines, summarize, formatUnified } from "cap:diff_unified";

let ops: [128]DiffLine = [DiffLine{}; 128];
const n = diffLines("a\nb\nc\n", "a\nB\nc\n", ops[0..]);
const stats = summarize(ops[0..n]); // added=1, removed=1, unchanged=2

let buf: [512]u8 = [0; 512];
const unified = formatUnified("a.txt", "b.txt", ops[0..n], 1, buf[0..]).?;
```
