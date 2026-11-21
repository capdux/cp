# `cap:git_diff_stat` — Git Unified Diff 零分配摘要与统计提取库

> 来源：吸收自 `/data/dev/bun/src/cli.zig`（Git Unified Diff Numstat & File Summary Parser）

`cap:git_diff_stat` 提供纯 CP、零外部依赖、零堆分配的 `diff --git` 补丁文件块迭代、新增/删除行与 Hunk 统计、文件变更类型识别（修改/新增/删除/重命名/二进制）及整份补丁汇总能力。

## 核心类型与函数

### `FileChangeKind` & `FileDiffStat`

```cp
pub enum FileChangeKind: u8 {
    modified = 0,
    added = 1,
    deleted = 2,
    renamed = 3,
    binary = 4,

    fn asStr(): string
}

pub struct FileDiffStat {
    old_path: []const u8 = "",
    new_path: []const u8 = "",
    kind: FileChangeKind = .modified,
    additions: usize = 0,
    deletions: usize = 0,
    hunks: usize = 0,
    is_binary: bool = false,
}
```

### `GitDiffIter`

```cp
pub struct GitDiffIter {
    static fn init(diff_text: []const u8): GitDiffIter
    fn next(*_): FileDiffStat?
}
```

- 按 `diff --git a/... b/...` 切分文件块（自动跳过前置 commit message 文本）。
- 识别 `new file mode`、`deleted file mode`、`rename from`/`rename to`、`Binary files ... differ`、`GIT binary patch` 与 `@@ ... @@` Hunk 头。
- 严格在 Hunk 内统计 `+`/`-` 行，自动排除 `+++`/`---` 文件头与 `\ No newline at end of file`。

### `DiffSummary` & `summarizeGitDiff`

```cp
pub struct DiffSummary {
    files_changed: usize = 0,
    insertions: usize = 0,
    deletions: usize = 0,
    binary_files: usize = 0,
}

pub fn summarizeGitDiff(diff_text: []const u8): DiffSummary
```

## 使用示例

```cp
import { GitDiffIter, summarizeGitDiff } from "cap:git_diff_stat";

const patch = "diff --git a/a.cp b/a.cp\n--- a/a.cp\n+++ b/a.cp\n@@ -1,2 +1,2 @@\n-old\n+new\n";
const sum = summarizeGitDiff(patch);
// sum.files_changed == 1, sum.insertions == 1, sum.deletions == 1
```
