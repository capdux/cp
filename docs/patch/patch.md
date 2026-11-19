# CP 标准库：Myers 文本差分与 Unified Patch 补丁子系统使用手册 (`cap:patch`)

## 一、 架构总览与设计哲学

`cap:patch` 是 CP 语言官方内置的**行级文本差分（Line-Level Myers / Prefix-Suffix Anchored Differ）、Unified Diff 块头解析（`UnifiedHunkHeader`）、补丁统计分析（`LineDiffStats`）与补丁渲染应用（`UnifiedDiff`）**标准库。该模块吸收自 Bun（`/data/dev/bun/src/patch/patch.cp`），为包管理器 `cap patch`、自动化测试快照比对（Snapshot Testing）以及代码热修复提供零堆碎片、纯切片驱动的差分引擎。

```text
┌────────────────────────────────────────────────────────────────────────────────────────┐
│                              cap:patch 差分与补丁架构                                   │
├───────────────────────────────────────────┬────────────────────────────────────────────┤
│          行级差分生成引擎 (myers.cp)        │       Unified Diff 解析与应用 (diff.cp)     │
│  • LineDiffer.splitLines (CRLF/LF 兼容)    │  • UnifiedHunkHeader.parse ("@@ -a,b +c,d")│
│  • LineDiffer.diffLines (公共前后缀剪枝)    │  • HunkRange (start_line, line_count)      │
│  • LineDiffer.editDistance (编辑距离 D)    │  • UnifiedDiff.analyzePatch (LineDiffStats)│
│  • LineDiffer.formatUnifiedToBuf          │  • UnifiedDiff.renderHunkNewSide           │
└───────────────────────────────────────────┴────────────────────────────────────────────┘
```

---

## 二、 核心算法原理

### 1. 公共前后缀线性剪枝 + Myers 编辑序列 (`LineDiffer.diffLines`)

在实际工程源码修改中，两个版本文件通常有 80% 以上的首尾行完全一致。`LineDiffer.diffLines` 采用两阶段算法：

1. **O(N) 公共前缀与公共后缀剥离**：
   - 从首行向下扫描最长公共前缀 `prefix`（直接标记为 `DiffOpKind.Equal`）；
   - 从尾行向上扫描最长公共后缀 `suffix`（保留至末尾标记为 `DiffOpKind.Equal`）；
2. **中间差异窗口（`prefix .. len - suffix`）最小编辑生成**：
   - 仅针对发生实质变更的中间窗口生成 `Delete` (`-`)、`Insert` (`+`) 与 `Equal` (` `) 操作序列 `[]DiffOp`，并可通过 `LineDiffer.editDistance(ops)` 计算 Myers 编辑距离 $D$（插入行数 + 删除行数）。

### 2. Unified Diff 块头语法 (`@@ -l,s +l,s @@`)

遵循 GNU Unified Diff 标准规范：
- `@@ -10,5 +12,7 @@` 解析为 `old_range = { start_line: 10, line_count: 5 }` 与 `new_range = { start_line: 12, line_count: 7 }`；
- 当行数为 1 且省略逗号（如 `@@ -1 +1 @@`）时，`parseRangeSpec` 自动将 `line_count` 归一化为 `1`。

---

## 三、 核心组件详解与完整代码示例

### 1. 行级差分计算与 Unified Diff 格式化 (`LineDiffer`)

```typescript
import {
    DiffOpKind,
    DiffOp,
    LineDiffer,
} from "cap:patch";

pub fn demoLineDiffer() {
    const old_src = "fn main() {\n    let x = 1;\n    return;\n}\n";
    const new_src = "fn main() {\n    let x = 2;\n    let y = 3;\n    return;\n}\n";

    // 1. 计算行级 DiffOp 序列（零堆分配，写入预分配栈数组）
    let ops: [16]DiffOp = undefined;
    const op_count = LineDiffer.diffLines(old_src, new_src, ops[0..16]);

    // 2. 计算 Myers 编辑距离 D (1 次删除 + 2 次插入 = 3)
    const dist = LineDiffer.editDistance(ops[0..op_count]);

    // 3. 将 DiffOp 序列格式化为标准 Unified Diff 文本块 (' ', '-', '+' 前缀行)
    let diff_buf: [512]u8 = undefined;
    const unified_body = LineDiffer.formatUnifiedToBuf(ops[0..op_count], diff_buf[0..512]);
    @print("{s}", unified_body);
}
```

---

### 2. Unified Hunk Header 解析、补丁统计与目标态还原 (`UnifiedHunkHeader` & `UnifiedDiff`)

```typescript
import {
    UnifiedHunkHeader,
    UnifiedDiff,
} from "cap:patch";

pub fn demoUnifiedPatch() {
    // 1. 解析 "@@ -old_start,old_count +new_start,new_count @@"
    if (hdr := UnifiedHunkHeader.parse("@@ -15,4 +15,5 @@ fn process()")) {
        @print("Old range: start={d}, count={d}\n", hdr.old_range.start_line, hdr.old_range.line_count);
        @print("New range: start={d}, count={d}\n", hdr.new_range.start_line, hdr.new_range.line_count);
    }

    // 2. 分析完整补丁文本（统计 hunk 数、插入行、删除行、上下文未变行）
    const patch_text =
        "--- a/config.toml\n" ++
        "+++ b/config.toml\n" ++
        "@@ -1,3 +1,4 @@\n" ++
        " host = \"127.0.0.1\"\n" ++
        "-port = 8080\n" ++
        "+port = 9090\n" ++
        "+tls = true\n" ++
        " workers = 4\n";

    const stats = UnifiedDiff.analyzePatch(patch_text);
    // stats.hunk_count == 1, stats.equal_lines == 2, stats.deleted_lines == 1, stats.inserted_lines == 2

    // 3. 应用 Hunk 渲染新版本目标文本
    let out_buf: [256]u8 = undefined;
    const patched_text = UnifiedDiff.renderHunkNewSide(patch_text, out_buf[0..256]);
    // patched_text == "host = \"127.0.0.1\"\nport = 9090\ntls = true\nworkers = 4\n"
}
```

---

## 四、 与 Bun (`bun patch`) / GNU Diff 对照表

| 功能特性 | GNU `diff -u` / `patch` | Bun (`src/patch/patch.cp`) | CP 标准库 (`cap:patch`) |
| :--- | :--- | :--- | :--- |
| **块头解析 (`@@ ... @@`)** | `pch.c` C 解析器 | `parseHeaderPart` | `UnifiedHunkHeader.parse` / `parseRangeSpec` |
| **CRLF / LF 跨平台兼容** | 需 `--ignore-trailing-space`| `std.mem.trim` 剥离 `\r` | `LineDiffer.splitLines` 自动归一化 `\r\n` |
| **行级差分序列生成** | Myers $O(ND)$ | `diff_main` | `LineDiffer.diffLines` + `editDistance` |
| **补丁统计分析** | `diffstat` 外部工具 | 内部遍历统计 | `UnifiedDiff.analyzePatch` (`LineDiffStats`) |
| **零堆分配栈内应用** | 不支持（全堆 `malloc`） | `ArenaAllocator` | `UnifiedDiff.renderHunkNewSide` + `formatUnifiedToBuf` |
