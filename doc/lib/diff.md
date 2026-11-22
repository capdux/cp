# CP 标准库：Myers Diff、Unified Patch 与三方合并手册 (`cap:diff`)

> 参考来源：`/data/dev/bun/src/patch/patch.cp`, `/data/dev/bun/src/test_runner/diff_format.zig` | 吸收日期：2026-11-22

## 一、 架构总览与设计准则

`cap:diff` 是 CP 语言官方内置的纯 CP 文本差异分析与代码合并标准库，**零外部依赖、零 `std.posix`、零 `@import("bun")`**，专为 CP 代码工具链与 AI Coding Agent 设计。既可通过 `import { ... } from "cap:diff"` 导入，也支持 `import cap from "cap"; cap.diff.*` 动态懒挂载调用。

```text
┌─────────────────────────────────────────────────────────────────────────┐
│                    CP 用户层 Diff & Merge 引擎 (cap:diff)                │
├──────────────────────────────────┬──────────────────────────────────────┤
│ Myers O(ND) 引擎 (myers.cp)       │ 补丁与三方合并 (patch.cp)             │
│ DiffEngine.diffLines (行级 SES)   │ UnifiedDiff.createPatch (多 Hunk)    │
│ DiffEngine.diffWords (词级精细)   │ UnifiedDiff.applyPatch (严格上下文)   │
│ DiffEngine.computeStats (统计)    │ UnifiedDiff.merge3 (冲突标记)         │
└──────────────────────────────────┴──────────────────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/diff/myers.cp` | `// Source: cp/lib/diff/myers.cp (Myers O(ND) Line & Word Diff Engine)` | `DiffError`、`DiffOpKind`、`DiffEdit`、`DiffStats`、`DiffEngine`、`splitLinesAlloc`、`tokenizeWordsAlloc` |
| `lib/diff/patch.cp` | `// Source: cp/lib/diff/patch.cp (Unified Diff Generator, Strict Patch Applier & 3-Way Merge Engine)` | `MergeResult`、`UnifiedDiff`（`createPatch` / `applyPatch` / `merge3`） |
| `lib/diff/index.cp` | `// Source: cp/lib/diff/index.cp (Unified Exports for cap:diff)` | `cap:diff` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误与基础类型（`myers.cp`）

- **`DiffError`**：`InvalidPatch`、`HunkMismatch`、`ContextConflict`、`OutOfMemory`。
- **`DiffOpKind`**：`Equal` (`0`)、`Insert` (`1`)、`Delete` (`2`)。
- **`DiffEdit`**：`kind: DiffOpKind`、`text: string`、`old_line: usize`（1-based，仅 `Equal`/`Delete` 有效）、`new_line: usize`（1-based，仅 `Equal`/`Insert` 有效）。
- **`DiffStats`**：`equal_count: usize`、`insert_count: usize`、`delete_count: usize`。

### 2. `DiffEngine`（`myers.cp`）

- **`DiffEngine.diffLines(alloc, old_text, new_text): ([]DiffEdit) ! DiffError`**：基于前缀/后缀修剪 + 64-bit FNV-1a 行哈希优化 + 经典 Myers $O(ND)$ 最短编辑脚本算法，精确处理空文本、单行、尾随换行差异及大规模相等块。
- **`DiffEngine.diffWords(alloc, old_text, new_text): ([]DiffEdit) ! DiffError`**：按空白、标识符与标点边界无损切分 token，计算词级精细编辑序列。
- **`DiffEngine.computeStats(edits): DiffStats`**：统计 `Equal`、`Insert`、`Delete` 数量。

### 3. `UnifiedDiff` 与 `MergeResult`（`patch.cp`）

- **`MergeResult`**：`clean: bool`、`conflict_count: usize`、`merged_text: string`。
- **`UnifiedDiff.createPatch(alloc, old_name, new_name, old_text, new_text, context_lines): string ! DiffError`**：生成标准 Unified Diff 文本（`--- a/...`、`+++ b/...`、`@@ -old_start,old_len +new_start,new_len @@`），超距不变块自动拆分为多个 Hunk，支持 `\ No newline at end of file`。
- **`UnifiedDiff.applyPatch(alloc, original_text, patch_text): string ! DiffError`**：严格解析 Hunk 头与行计数（畸形补丁返回 `DiffError.InvalidPatch`），并逐行校验上下文与删除行内容（不匹配返回 `DiffError.HunkMismatch`）。
- **`UnifiedDiff.merge3(alloc, base_text, ours_text, theirs_text): MergeResult ! DiffError`**：基于 `base_text` 执行三方合并；非重叠或双边相同修改自动合并（`clean = true`），重叠分歧修改输出标准 `<<<<<<< ours` / `=======` / `>>>>>>> theirs` 冲突标记（`clean = false`）。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { DiffEngine, UnifiedDiff } from "cap:diff";
import cap from "cap";

pub fn demoDiff(): ! {
    const alloc = std.heap.page_allocator;

    const old_text = "let a = 1;\nlet b = 2;\n";
    const new_text = "let a = 1;\nlet b = 42;\n";

    // 1. 生成并应用 Unified Diff
    const patch = UnifiedDiff.createPatch(alloc, "main.cp", "main.cp", old_text, new_text, 1)?;
    defer alloc.free(patch);
    const restored = UnifiedDiff.applyPatch(alloc, old_text, patch)?;
    defer alloc.free(restored);

    // 2. 通过 cap.diff 动态挂载执行三方合并
    const m = cap.diff.UnifiedDiff.merge3(alloc, old_text, new_text, "let a = 10;\nlet b = 2;\n")?;
    defer alloc.free(m.merged_text);
    * = restored;
}
```


---

## 扩展行级 Myers 与轻量 Unified Diff（已收敛至 `cap:diff`）

- **`lib/diff/linediff.cp`**：`MyersOpKind`, `DiffOp`, `DiffResult`, `diffLines`, `reconstructNew`, `formatUnifiedDiff`
- **`lib/diff/unified.cp`**：`UnifiedOpKind`, `DiffLine`, `DiffSummary`, `unifiedDiffLines`, `summarize`, `formatUnified`

---

## 上游行为契约与合并边界注意事项

1. **严格上下文校验 vs 模糊补丁定位**：
   - `cap:diff` 的 `UnifiedDiff.applyPatch` 采用**严格上下文精确校验**（任意上下文行不匹配即报错拒绝，适合自动化代码生成与 AI Coding Agent 安全编辑）；若需要面向旧版源码的 `±offset` / `fuzz` 容错打补丁，请使用 `cap:patch` 的 `applyWithOptions`。
2. **三方合并冲突检测（`UnifiedDiff.merge3`）**：
   - 以共同祖先 `base` 为基准分别比对 `ours` 与 `theirs`：互不重叠的修改自动无损合并；同一行区间被双方不同修改时输出标准 `<<<<<<<` / `=======` / `>>>>>>>` 冲突标记并置位 `MergeResult.has_conflicts`。
3. **行级 vs 词级差分选型**：
   - 代码补丁与版本控制使用 `DiffEngine.diffLines`；长段落文本或断言行内细粒度高亮使用 `DiffEngine.diffWords` + `tokenizeWordsAlloc`。
