# CP 标准库：三方文本合并与冲突消解引擎 (`cap:diff3`)

## 一、 架构总览与设计准则

`cap:diff3` 是 CP 语言官方内置的纯 CP 三方文本合并（Three-Way Text Merge）与冲突消解标准库，**零外部依赖、零 `std.posix`、零 `@import("bun")`**。基于 Myers $O(ND)$ 行级最短编辑脚本与 64-bit FNV-1a 行哈希优化，将 `base -> ours` 与 `base -> theirs` 双向对齐，支持无冲突自动融合、双边相同修改（false conflict）自动消除、最小冲突窗口精确隔离，以及 `Standard` 与 `Diff3` 两种冲突标记输出格式。

既可通过 `import { ... } from "cap:diff3"` 导入，也支持 `import cap from "cap"; cap.diff3.*` 动态懒挂载调用。

### 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/diff3/diff3.cp` | `// Source: cp/lib/diff3/diff3.cp (Three-Way Text Merge & Conflict Resolution Engine)` | `Diff3Error`、`ConflictStyle`、`ConflictBlock`、`MergeHunk`、`MergeOptions`、`MergeResult`、`mergeLines`、`mergeTexts`、`hasConflicts` |
| `lib/diff3/index.cp` | `// Source: cp/lib/diff3/index.cp (Unified Exports for cap:diff3)` | `cap:diff3` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 类型与选项

- **`Diff3Error`**：`OutOfMemory`
- **`ConflictStyle`**：
  - `Standard` (`0`)：输出 `<<<<<<< <ours_label>` / `=======` / `>>>>>>> <theirs_label>`
  - `Diff3` (`1`)：输出 `<<<<<<< <ours_label>` / `||||||| <base_label>` / `=======` / `>>>>>>> <theirs_label>`
- **`ConflictBlock`**：`base: []const []const u8`、`ours: []const []const u8`、`theirs: []const []const u8`
- **`MergeHunk`**：带标签联合体（Tagged Union）：
  - `ok: []const []const u8`（干净合并的行切片）
  - `conflict: ConflictBlock`（未解决的冲突块）
- **`MergeOptions`**：
  - `ours_label: []const u8 = "ours"`
  - `base_label: []const u8 = "base"`
  - `theirs_label: []const u8 = "theirs"`
  - `style: ConflictStyle = ConflictStyle.Standard`
  - `resolve_identical_changes: bool = true`
- **`MergeResult`**：
  - `has_conflicts: bool`
  - `conflict_count: usize`
  - `hunks: []MergeHunk`
  - `merged_text: []u8`
  - `deinit(*_, allocator: std.mem.Allocator)`：释放 `hunks` 与 `merged_text` 占用的全部分配内存。

### 2. 核心函数

- **`mergeLines(allocator, base_lines, ours_lines, theirs_lines, opts): MergeResult ! Diff3Error`**
  对预切分的行数组执行三方合并，生成结构化 `hunks` 与以 `\n` 拼接的 `merged_text`。
- **`mergeTexts(allocator, base_text, ours_text, theirs_text, opts): MergeResult ! Diff3Error`**
  对原始文本执行三方合并，自动识别并保留 `\n` 与 `\r\n` 换行符及末尾换行语义。
- **`hasConflicts(base_text, ours_text, theirs_text): bool`**
  快速判定三方合并是否存在未解决冲突（内置临时 Arena，无外部分配器负担）。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { mergeTexts, ConflictStyle, MergeOptions } from "cap:diff3";
import cap from "cap";

pub fn demoDiff3(): ! {
    const alloc = std.heap.page_allocator;
    const base = "line 1\nline 2\nline 3\n";
    const ours = "line 1 (ours)\nline 2\nline 3\n";
    const theirs = "line 1\nline 2\nline 3 (theirs)\n";

    let res = mergeTexts(alloc, base, ours, theirs, MergeOptions{})?;
    defer res.deinit(alloc);

    // 通过 cap.diff3 动态挂载判定冲突
    const conflicted = cap.diff3.hasConflicts(base, "x\nline 2\nline 3\n", "y\nline 2\nline 3\n");
    * = conflicted;
}
```
