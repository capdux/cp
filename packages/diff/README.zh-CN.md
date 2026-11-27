[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# `diff`

面向 CP 语言的零分配 Myers `O(ND)` 行级与字符级 Diff 引擎——完整对标 **npm `diff`**（`jsdiff`）与 **`fast-diff`**（并融合 `similar` 相似度统计），内置 FNV-1a 64-bit 行哈希预过滤、标准 GNU Unified Diff 格式化与无损补丁校验应用。

## 源包溯源与零成本迁移

| 项目 | 说明 |
| :--- | :--- |
| **源包平台与对标** | `npm:diff` ([kpdecker/jsdiff](https://github.com/kpdecker/jsdiff), v7 对齐) & `npm:fast-diff` ([jhchen/fast-diff](https://github.com/jhchen/fast-diff), v1 对齐) |
| **CP 包名与版本** | `diff@1.0.0` (`MIT`) |
| **外部依赖** | 无（纯 CP，零外部依赖） |

保持原包名导入，零改名成本直接迁移：

```ts
// TypeScript (npm diff)
import { diffLines, createTwoFilesPatch } from "diff";
const changes = diffLines(oldText, newText);
const patch = createTwoFilesPatch("a.txt", "b.txt", oldText, newText);
```

```cp
// CP — 保持原包名，零堆分配 Myers O(ND)
import { diffLines, formatUnifiedDiff } from "diff";

const res = diffLines(old_text, new_text) catch return;
let patch_buf: [1024]u8 = undefined;
const patch = formatUnifiedDiff("a.txt", "b.txt", old_text, new_text, 3, patch_buf[0..]) catch return;
```

## 架构特性与零堆设计

- **FNV-1a 64-bit 行哈希预过滤**：切分阶段为每行计算 64 位 FNV-1a 哈希，比对时先 `O(1)` 哈希与长度快拒，命中后再做精确字节比对，彻底免疫哈希碰撞。
- **前后缀裁剪 + 紧凑三角形栈 Trace**：相同文件 0 次 Myers 迭代即刻返回；核心 Trace 使用紧凑三角形 `u16` 栈缓冲（`undefined` 延迟初始化、零栈清零开销，较朴素二维矩阵节省 8× 内存）。
- **无损换行保留与补丁校验**：完整保留 `\n` 与 `\r\n`，保证 `applyLineDiff(old, diffLines(old, new), out) == new`；应用补丁时逐行校验源内容，不匹配立即返回 `DiffError.PatchFailed`。

## 实战示例

### 1. 行级 Myers Diff、相似度统计与无损重构

```cp
import { diffLines, applyLineDiff, DiffOp } from "diff";

fn demoLineDiff() {
    const old_doc = "alpha\nbeta\ngamma\ndelta\n";
    const new_doc = "alpha\nBETA\ngamma\nepsilon\ndelta\n";

    const res = diffLines(old_doc, new_doc) catch return;
    if (!res.isIdentical()) {
        const stats = res.stats();
        const ratio = stats.similarity(); // 2 * 3 / (4 + 5) = 0.6667
        _ = ratio;
    }

    let out: [512]u8 = undefined;
    const rebuilt = applyLineDiff(old_doc, res, out[0..]) catch return;
    // rebuilt == new_doc
    _ = rebuilt;
}
```

### 2. 生成标准 GNU Unified Diff（`formatUnifiedDiff`）

```cp
import { formatUnifiedDiff } from "diff";

fn demoUnifiedDiff() {
    const old_src = "line1\nline2\nline3\nline4\nline5\n";
    const new_src = "line1\nline2\nLINE_3_MOD\nline4\nline5\n";

    let patch_buf: [1024]u8 = undefined;
    const patch = formatUnifiedDiff(
        "a/config.cp",
        "b/config.cp",
        old_src,
        new_src,
        1,
        patch_buf[0..],
    ) catch return;
    // --- a/config.cp
    // +++ b/config.cp
    // @@ -2,3 +2,3 @@
    //  line2
    // -line3
    // +LINE_3_MOD
    //  line4
    _ = patch;
}
```

### 3. 字符级 Diff（`diffChars`）与边界错误处理（`DiffError`）

```cp
import { diffChars, applyLineDiff, DiffError } from "diff";

fn demoCharsAndErrors() {
    // 字节/字符级精确 Myers Diff（单侧最多 MAX_LINES = 256 字节）
    const res = diffChars("kitten", "sitting") catch |err| {
        // 单侧超过 256 字节时返回 DiffError.TooManyLines
        _ = err;
        return;
    };
    const stats = res.stats(); // equal=4, deleted=2, inserted=3
    _ = stats;

    // 源文本不匹配或输出缓冲不足时返回 DiffError.PatchFailed / BufferTooSmall
    let small: [4]u8 = undefined;
    _ = applyLineDiff("wrong-source", res, small[0..]) catch |err| {
        _ = err;
        return;
    };
}
```

## 核心 API 速览

- **常量**：`MAX_LINES = 256`、`MAX_EDITS = 512`
- **错误集**：`DiffError`（`TooManyLines`、`BufferTooSmall`、`PatchFailed`、`InvalidPatch`）
- **核心类型**：`DiffOp`（`Equal`、`Delete`、`Insert`）、`DiffEdit`、`DiffStats`（`similarity()`）、`LineDiffResult`（`stats()`、`isIdentical()`、`slice()`、`get(idx)`）
- **核心函数**：`diffLines`、`diffChars`、`formatUnifiedDiff`、`applyLineDiff`、`fnv1a64`、`splitLines`

完整类型签名请查阅 [`src/index.d.cp`](src/index.d.cp)。
