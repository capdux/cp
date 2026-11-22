# CP 标准库：Myers 文本差分、Unified Patch 与彩色测试 Diff 渲染子系统 (`cap:patch`)

> 参考来源：`/data/dev/bun/src/patch/patch.cp`, `/data/dev/bun/src/test_runner/diff_format.zig` | 吸收日期：2026-11-22

## 一、 架构总览与设计哲学

`cap:patch` 是 CP 语言官方内置的**行级 Myers $O(ND)$ 文本差分引擎、完整多文件/多 Hunk Unified Diff 解析器、支持精确/偏移/上下文模糊容错（Fuzz/Offset）的补丁应用引擎、标准 Unified Diff 生成器与 `bun test` 风格彩色差异渲染器（`DiffFormatter`）**标准库。该模块吸收自 Bun（`/data/dev/bun/src/patch/patch.cp` 与 `/data/dev/bun/src/test_runner/diff_format.zig`），为包管理器 `cap patch`、代码热修复与 `cap test` 断言比对提供工业级差分、打补丁与诊断渲染能力。

```text
┌────────────────────────────────────────────────────────────────────────────────────────┐
│                              cap:patch 差分、补丁与测试渲染架构                          │
├─────────────────────────────────┬─────────────────────────────┬────────────────────────┤
│  Myers O(ND) 差分 (myers.cp)    │  Unified Diff (diff.cp)     │ 测试 Diff 渲染 (diffFormat.cp) │
│  • LineDiffer.diffLines         │  • UnifiedDiff.parse        │  • DiffFormatOptions   │
│  • LineDiffer.diffLinesAlloc    │  • PatchFileEntry           │  • DiffFormatter       │
│  • LineDiffer.generateUnifiedDiff│ • UnifiedDiff.apply        │    (绿色 + Expected /  │
│  • LineDiffer.editDistance      │  • applyWithOptions         │     红色 - Received /  │
│                                 │    (±offset + fuzz 容错)    │     行号/裁剪/行内高亮) │
└─────────────────────────────────┴─────────────────────────────┴────────────────────────┘
```

---

## 二、 核心能力与算法原理

### 1. 完整 Unified Diff 解析 (`UnifiedDiff.parse` -> `PatchDocument`)
对照 Bun `PatchLinesParser`，支持：
- **多文件格式兼容**：同时支持 Git 扩展格式（`diff --git a/... b/...`）与传统多文件 Unified Diff（连续 `--- a/...` / `+++ b/...` 切换文件）。
- **完整文件元数据头**：解析 `--- a/...`、`+++ b/...`（自动剥离可选时间戳与前缀，识别 `/dev/null`）、`new file mode`、`deleted file mode`、`rename from` / `rename to`、`old mode` / `new mode` 以及 `index hash1..hash2`。
- **多 Hunk 与 `\ No newline at end of file`**：完整解析多个 `@@ -old_start,old_count +new_start,new_count @@ section` 块，精确追踪旧文件与新文件末尾是否缺少换行符，并执行行计数完整性校验（`PatchHunk.verifyIntegrity()`）。

### 2. Hunk 应用引擎 (`UnifiedDiff.apply` / `applyWithOptions` / `applyFilePatch`)
遵循 GNU `patch` 的三阶容错应用管线，并返回结构化诊断 `PatchApplyResult`：
1. **精确行号匹配 (`offset = 0, fuzz = 0`)**：根据 Hunk 头行号及前置 Hunk 累积行偏移定位目标位置，逐行比对全部上下文行（` `）与删除行（`-`）。
2. **上下文偏移搜索 (`±max_offset`, 默认 64 行)**：以 `±1, ±2, ...` 向外螺旋搜索精确匹配位置，记录 `offset_hunks` 与 `applied_offset`。
3. **上下文模糊容错 (`fuzz = 1 .. max_fuzz`, 默认 2 行)**：允许逐级放宽首尾各 `fuzz` 行上下文完成安全替换。

### 3. Myers $O(ND)$ 差分与标准 Unified Diff 生成 (`LineDiffer` & `UnifiedDiff.diffLines`)
- `LineDiffer.diffLines(old_text, new_text, out_ops)`：先以 $O(N)$ 剪枝公共前后缀，再在中间差异窗口执行零堆分配 Myers $O(ND)$ 最短编辑路径回溯。
- `UnifiedDiff.diffLines(allocator, old, new, old_path, new_path, context_lines)`：生成标准 Unified Diff 文本。

### 4. `bun test` 风格彩色测试差异渲染器 (`DiffFormatter` & `DiffFormatOptions`)
吸收自 Bun `src/test_runner/diff_format.zig`：
- **绿色期望与红色实际**：在顶部生成 `+ Expected`（绿色 `\x1b[32m`）与 `- Received`（红色 `\x1b[31m`）图例，并将实际/期望差异逐行配色展示；
- **行号标尺**：`show_line_numbers = true` 时自动按最大行号对齐左侧 `N | ` 标尺；
- **上下文行裁剪**：超出 `context_lines` 窗口的连续未变更行折叠为 `... N unchanged lines ...`；
- **单行内字符级公共前后缀差异高亮**：针对单行对比或成对修改行，计算公共前缀与公共后缀，在彩色模式下使用反显（`\x1b[7m...\x1b[27m`）、在纯文本模式下使用 `«...»` 精确标出变更子串。

---

## 三、 完整代码示例

### 1. 生成标准 Unified Diff 并无损应用 (`UnifiedDiff.diffLines` + `UnifiedDiff.apply`)

```typescript
const std = @import("std");
import { UnifiedDiff } from "cap:patch";

fn demoGenerateAndApply(): ! {
    const allocator = std.heap.page_allocator;
    const old_src = "fn init() {}\nconst v = 1;\nconst keep = true;\nconst end = 0";
    const new_src = "fn init() {}\nconst v = 2;\nconst keep = true;\nconst end = 1;\n";

    const patch_txt = try UnifiedDiff.diffLines(allocator, old_src, new_src, "mod.cp", "mod.cp", 1);
    defer allocator.free(patch_txt);

    const res = try UnifiedDiff.apply(allocator, old_src, patch_txt);
    defer if (res.ok) allocator.free(res.text);
}
```

### 2. `bun test` 风格彩色 Diff 渲染 (`DiffFormatter` + `DiffFormatOptions`)

```typescript
const std = @import("std");
import { DiffFormatter, DiffFormatOptions } from "cap:patch";

fn demoColoredTestDiff(): ! {
    const allocator = std.heap.page_allocator;
    const expected = "line 1\nline 2\nconst port = 8080;\nline 4\nline 5";
    const received = "line 1\nline 2\nconst port = 9090;\nline 4\nline 5";

    const rendered = try DiffFormatter.format(
        allocator,
        expected,
        received,
        DiffFormatOptions{
            use_color: true,
            context_lines: 1,
            show_line_numbers: true,
            expected_label: "Expected",
            received_label: "Received",
        },
    );
    defer allocator.free(rendered);
}
```

---

## 四、 与 Bun (`bun patch` / `bun test`) / GNU Diff 对照表

| 功能特性 | GNU `diff -u` / `patch` | Bun 上游源码 | CP 标准库 (`cap:patch`) |
| :--- | :--- | :--- | :--- |
| **多文件/元信息头解析** | `pch.c` | `src/patch/patch.cp` (`PatchLinesParser`) | `UnifiedDiff.parse` (`PatchDocument`) |
| **块头解析 (`@@ ... @@`)** | 完整支持 | `parseHunkHeaderLine` | `UnifiedHunkHeader.parse`（含 section） |
| **No newline at EOF** | 支持 | `no_newline_at_end_of_file` | 完整解析、生成与还原 |
| **Hunk 应用（偏移与 Fuzz）** | `--fuzz` / 行偏移搜索 | `applyPatch` | `UnifiedDiff.applyFilePatch`（精确+±offset+fuzz+错误定位） |
| **行级 Myers $O(ND)$ 差分** | Myers $O(ND)$ | `diff` | `LineDiffer.diffLines` / `diffLinesAlloc` + `UnifiedDiff.diffLines` |
| **测试断言彩色对比渲染** | — | `src/test_runner/diff_format.zig` | `DiffFormatter`, `DiffFormatOptions`（行号、裁剪、行内前后缀高亮） |

---

## 五、 上游 Bun `patch` 行为契约与边界注意事项

1. **Hunk 完整性校验与偏移/模糊容错（`applyWithOptions`）**：
   - 源自 Bun `src/patch/patch.cp`：每个 Hunk 应用前可调用 `PatchHunk.verifyIntegrity()` 校验 `old_count`/`new_count` 与实际上下文/增删行数一致；默认精确行号失败时，`applyWithOptions` 按偏移窗口（`±offset`）与上下文模糊度（`fuzz`）尝试安全重定位，防止源码轻微变动导致补丁整体失效。
2. **文件末尾换行符（`\ No newline at end of file`）精确还原**：
   - 解析器与生成器均严格区分“末尾含 `\n`”与“无末尾换行”两种状态，确保 `diff -> apply` 往返后的字节哈希与原目标文件 100% 比特级一致。
3. **测试差异输出可读性（`DiffFormatter`）**：
   - 继承自 Bun `test_runner/diff_format.zig`：大文本比对自动裁剪未变更中间行并高亮单行内的差异前后缀，非 TTY 环境下可关闭 ANSI 颜色标志输出纯文本诊断。
