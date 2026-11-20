# CP 标准库：Myers 文本差分与 Unified Patch 补丁子系统使用手册 (`cap:patch`)

## 一、 架构总览与设计哲学

`cap:patch` 是 CP 语言官方内置的**行级 Myers $O(ND)$ 文本差分引擎、完整多文件/多 Hunk Unified Diff 解析器、支持精确/偏移/上下文模糊容错（Fuzz/Offset）的补丁应用引擎与标准 Unified Diff 生成器**标准库。该模块吸收自 Bun（`/data/dev/bun/src/patch/patch.cp`），为包管理器 `cap patch`、代码热修复与快照比对提供工业级差分与打补丁能力。

```text
┌────────────────────────────────────────────────────────────────────────────────────────┐
│                              cap:patch 差分与补丁架构                                   │
├───────────────────────────────────────────┬────────────────────────────────────────────┤
│       Myers O(ND) 差分与生成器 (myers.cp)  │     Unified Diff 解析与应用 (diff.cp)      │
│  • LineDiffer.diffLines (栈零堆 Myers)     │  • UnifiedDiff.parse (多文件/多Hunk/元数据)│
│  • LineDiffer.diffLinesAlloc (任意长度文件)│  • PatchFileEntry (修改/新增/删除/重命名/权限)│
│  • LineDiffer.generateUnifiedDiff         │  • UnifiedDiff.apply / applyWithOptions     │
│  • LineDiffer.formatUnifiedToBuf          │    (精确匹配 + ±offset 搜索 + fuzz 上下文容错)│
│  • LineDiffer.editDistance                │  • UnifiedDiff.diffLines / analyzePatch     │
└───────────────────────────────────────────┴────────────────────────────────────────────┘
```

---

## 二、 核心能力与算法原理

### 1. 完整 Unified Diff 解析 (`UnifiedDiff.parse` -> `PatchDocument`)
对照 Bun `PatchLinesParser`，支持：
- **多文件格式兼容**：同时支持 Git 扩展格式（`diff --git a/... b/...`）与传统多文件 Unified Diff（连续 `--- a/...` / `+++ b/...` 切换文件）。
- **完整文件元数据头**：解析 `--- a/...`、`+++ b/...`（自动剥离可选时间戳与前缀，识别 `/dev/null`）、`new file mode`（新增 `Create`）、`deleted file mode`（删除 `Delete`）、`rename from` / `rename to`（重命名 `Rename`）、`old mode` / `new mode`（权限变更 `ModeChange`）以及 `index hash1..hash2`。
- **多 Hunk 与 `\ No newline at end of file`**：完整解析多个 `@@ -old_start,old_count +new_start,new_count @@ section` 块，精确追踪旧文件与新文件末尾是否缺少换行符（`old_no_newline` / `new_no_newline`），并执行行计数完整性校验（`PatchHunk.verifyIntegrity()`）。

### 2. Hunk 应用引擎 (`UnifiedDiff.apply` / `applyWithOptions` / `applyFilePatch`)
遵循 GNU `patch` 的三阶容错应用管线，并返回结构化诊断 `PatchApplyResult`：
1. **精确行号匹配 (`offset = 0, fuzz = 0`)**：根据 Hunk 头行号及前置 Hunk 累积行偏移定位目标位置，逐行比对全部上下文行（` `）与删除行（`-`）。
2. **上下文偏移搜索 (`±max_offset`, 默认 64 行)**：若原文件在 Hunk 之前增删了无关代码导致行号漂移，引擎以 `±1, ±2, ...` 向外螺旋搜索精确匹配位置，记录 `offset_hunks` 与 `applied_offset`。
3. **上下文模糊容错 (`fuzz = 1 .. max_fuzz`, 默认 2 行)**：若目标位置前导或尾随外层上下文行发生变更，允许逐级放宽首尾各 `fuzz` 行上下文（绝不放宽删除行 `-`），并保留原文件外层行完成安全替换。
4. **精准错误定位**：若任一 Hunk 应用失败，返回 `ok = false`、`error_kind = .HunkMismatch`、`failed_hunk_index`（失败的 Hunk 下标）与 `expected_line`（预期起始行号）。

### 3. Myers $O(ND)$ 差分与标准 Unified Diff 生成 (`LineDiffer` & `UnifiedDiff.diffLines`)
- `LineDiffer.diffLines(old_text, new_text, out_ops)`：先以 $O(N)$ 剪枝公共前后缀，再在中间差异窗口执行零堆分配 Myers $O(ND)$ 最短编辑路径回溯（支持多处分离修改识别为多个局部变更而非整段替换）。
- `UnifiedDiff.diffLines(allocator, old, new, old_path, new_path, context_lines)`（等价 `LineDiffer.generateUnifiedDiff`）：基于无界 `diffLinesAlloc` 自动合并邻近 Hunk 上下文窗口，生成包含 `--- a/...`、`+++ b/...`、`@@ -l,s +l,s @@` 与 `\ No newline at end of file` 的标准 Unified Diff 文本。

---

## 三、 完整代码示例

### 1. 生成标准 Unified Diff 并无损应用 (`UnifiedDiff.diffLines` + `UnifiedDiff.apply`)

```typescript
const std = @import("std");
import { UnifiedDiff, PatchApplyOptions, PatchErrorKind } from "cap:patch";

fn demoGenerateAndApply(): ! {
    const allocator = std.heap.page_allocator;
    const old_src = "fn init() {}\nconst v = 1;\nconst keep = true;\nconst end = 0";
    const new_src = "fn init() {}\nconst v = 2;\nconst keep = true;\nconst end = 1;\n";

    // 1. 生成标准多 Hunk Unified Diff（保留 1 行上下文及 No newline 标记）
    const patch_txt = try UnifiedDiff.diffLines(allocator, old_src, new_src, "mod.cp", "mod.cp", 1);
    defer allocator.free(patch_txt);

    // 2. 将补丁应用到原始文本（支持精确 + 偏移 + Fuzz 容错）
    const res = try UnifiedDiff.apply(allocator, old_src, patch_txt);
    defer if (res.ok) allocator.free(res.text);
    // res.ok == true && res.text == new_src
}
```

### 2. 多文件 Patch 解析、偏移/Fuzz 应用与错误定位

```typescript
const std = @import("std");
import { UnifiedDiff, PatchApplyOptions, PatchFileKind } from "cap:patch";

fn demoMultiFileAndFuzzyApply(patch_text: []const u8): ! {
    const allocator = std.heap.page_allocator;

    // 1. 解析多文件 Unified Diff（Modify / Create / Delete / Rename）
    let doc = try UnifiedDiff.parse(allocator, patch_text);
    defer doc.deinit(allocator);

    // 2. 带自定义偏移与上下文 Fuzz 容错应用补丁
    const target_src = "// extra banner\nctx1\nold_val\nctx2\n";
    const res = try UnifiedDiff.applyWithOptions(
        allocator,
        target_src,
        patch_text,
        PatchApplyOptions{ max_offset: 32, max_fuzz: 2 },
    );
    defer if (res.ok) allocator.free(res.text);

    if (!res.ok) {
        // 可精准读取 res.error_kind, res.failed_hunk_index, res.expected_line
    }
}
```

---

## 四、 与 Bun (`bun patch`) / GNU Diff 对照表

| 功能特性 | GNU `diff -u` / `patch` | Bun (`src/patch/patch.cp`) | CP 标准库 (`cap:patch`) |
| :--- | :--- | :--- | :--- |
| **多文件/元信息头解析** | `pch.c` | `PatchLinesParser` + `PatchFile` | `UnifiedDiff.parse` (`PatchDocument`) |
| **块头解析 (`@@ ... @@`)** | 完整支持 | `parseHunkHeaderLine` | `UnifiedHunkHeader.parse`（含 section） |
| **No newline at EOF** | 支持 | `no_newline_at_end_of_file` | 完整解析、生成与还原 |
| **Hunk 应用（偏移与 Fuzz）** | `--fuzz` / 行偏移搜索 | `applyPatch` | `UnifiedDiff.applyFilePatch`（精确+±offset+fuzz+错误定位） |
| **行级 Myers $O(ND)$ 差分** | Myers $O(ND)$ | `diff` | `LineDiffer.diffLines` / `diffLinesAlloc` + `UnifiedDiff.diffLines` |
