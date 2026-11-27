[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `@cap/diff` — 零分配 Myers O(ND) 文本与字符 Diff 引擎

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:diff` ([https://github.com/kpdecker/jsdiff](https://github.com/kpdecker/jsdiff)) / `npm:fast-diff` ([https://github.com/jhchen/fast-diff](https://github.com/jhchen/fast-diff))

> 独立 Clean-Room 纯 CP 实现（严格对标 npm `diff` [周下载 120M+] 与 Rust `similar` crate）| 许可协议：MIT

## 架构概述

`@cap/diff` 是面向代码审查、版本控制、CLI 工具与 AI Agent 增量补丁场景设计的纯 CP、零外部依赖、零堆分配差分引擎：

- **FNV-1a 64-bit 行哈希预过滤**：行切分阶段为每一行预计算 64-bit FNV-1a 哈希，进入比对时先执行 `O(1)` 哈希值与长度双重快拒，仅在哈希与长度均命中时执行字节级 `std.mem.eql` 最终确认，大幅加速长文本行比对并彻底免疫哈希碰撞。
- **前后缀裁剪 + 标准 Myers O(ND) 最短编辑脚本（SES）**：
  - 先以 `O(min(N, M))` 剥离公共前缀与公共后缀（完全相同文本 0 次 Myers 迭代即刻返回）；
  - 核心中间窗口采用标准 Eugene W. Myers `O(ND)` 算法，使用**紧凑三角形栈 Trace 缓冲**（`u16` 坐标，`513 * 514 / 2` 槽位，约 257 KiB 栈空间，相比朴素二维数组压缩 8x 内存），全部以 `undefined` 延迟初始化，零清零开销、零堆分配（支持单侧最多 256 行 / 256 字节）。
- **无损行切分与补丁校验**：行切分完整保留行尾 `\n` 与 `\r\n`，保证任意输入（含空文本、缺失末尾换行符、混合换行符）均满足 `applyLineDiff(old, diffLines(old, new), out) == new`；应用补丁时逐行校验 `Equal`/`Delete` 与源文本的一致性，不匹配立即返回 `DiffError.PatchFailed`。
- **标准 GNU Unified Diff 格式化**：输出标准 `--- old_name`、`+++ new_name`、`@@ -a,b +c,d @@` 头部，自动按 `context_lines` 展开上下文并合并重叠/相邻 Hunk，对无末尾换行符的行自动追加 `\ No newline at end of file`。

## 版权与许可

Copyright (c) 2026 CP-Lang Contributors. Released under the **MIT License**.

## 导出 API

| 符号 / 签名 | 说明 |
| :--- | :--- |
| `MAX_LINES` (`256`) / `MAX_EDITS` (`512`) | 单侧最大行数（或字符数）与最大编辑脚本条目上限 |
| `DiffError` | 错误集：`TooManyLines`、`BufferTooSmall`、`PatchFailed`、`InvalidPatch` |
| `DiffOp` | 编辑操作枚举：`Equal = 0`、`Delete = 1`、`Insert = 2` |
| `DiffEdit` | 单条编辑记录：`op`、`old_index`、`new_index`、`text` |
| `DiffStats` | 统计信息：`equal`、`inserted`、`deleted` 及 `similarity(): f64`（`[0.0, 1.0]`，对标 `similar::TextDiff::ratio()`） |
| `LineDiffResult` | 栈分配 Diff 结果：`edits`、`len`、`stats()`、`isIdentical()`、`slice()`、`get(idx)` |
| `diffLines(old_text, new_text)` | 行级 Myers O(ND) 最短编辑脚本（FNV-1a 哈希预过滤），返回 `LineDiffResult ! DiffError` |
| `diffChars(old_text, new_text)` | 字节/字符级精确 Myers O(ND) Diff（支持最长 256 字节短串），返回 `LineDiffResult ! DiffError` |
| `formatUnifiedDiff(old_name, new_name, old_text, new_text, context_lines, out)` | 生成标准 GNU Unified Diff（含多 Hunk 自动合并），写入调用方栈缓冲 `out` |
| `applyLineDiff(old_text, diff_res, out)` | 校验并应用行级 Diff 无损重构目标文本，写入调用方栈缓冲 `out` |

## 使用示例

### 示例 1：行级 Myers Diff、统计相似度与无损重构

```cp
const std = @import("std");
import { diffLines, applyLineDiff, DiffOp } from "@cap/diff";

fn demoLineDiffAndApply() {
    const old_doc = "alpha\nbeta\ngamma\ndelta\n";
    const new_doc = "alpha\nBETA\ngamma\nepsilon\ndelta\n";

    const res = diffLines(old_doc, new_doc) catch return;
    const stats = res.stats();
    const ratio = stats.similarity(); // 2 * 3 / (4 + 5) = 0.6667

    let out: [512]u8 = undefined;
    const rebuilt = applyLineDiff(old_doc, res, out[0..]) catch return;
    // std.mem.eql(u8, rebuilt, new_doc) == true
    * = ratio;
    * = rebuilt;
}
```

### 示例 2：生成标准 GNU Unified Diff（带上下文合并 Hunk）

```cp
import { formatUnifiedDiff } from "@cap/diff";

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
    // 输出:
    // --- a/config.cp
    // +++ b/config.cp
    // @@ -2,3 +2,3 @@
    //  line2
    // -line3
    // +LINE_3_MOD
    //  line4
    * = patch;
}
```

### 示例 3：字符级精确 Myers Diff

```cp
import { diffChars, DiffOp } from "@cap/diff";

fn demoCharDiff() {
    const res = diffChars("kitten", "sitting") catch return;
    const stats = res.stats();
    // equal = 4 ('i','t','t','n'), deleted = 2 ('k','e'), inserted = 3 ('s','i','g')
    * = stats;
}
```
