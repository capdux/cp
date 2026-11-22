# `cap:myers_diff` — Myers $O(ND)$ 最短编辑脚本 Diff 引擎

> 参考来源：`/data/dev/bun/src/runtime/node/assert/myers_diff.cp` | 吸收日期：2026-11-22

提供零外部依赖的纯 CP Myers 最短编辑脚本（SES）差异计算引擎，支持行级 Diff（含尾随逗号容错模式）、字节级 Diff、按行切分与统一格式渲染，并内建 `~init()` 确定性 RAII 释放。

## 核心导出 API

| 符号 | 说明 |
| :--- | :--- |
| `DiffError` | 错误集：`InputsTooLarge`、`DiffTooLarge`、`OutOfMemory` |
| `DiffKind` | 差异种类枚举：`equal`、`insert`、`delete`（方法 `prefixByte(): u8`） |
| `LineDiff` / `ByteDiff` | 单条行/字节差异项 `{ kind: DiffKind, value: string / u8 }` |
| `DiffOptions` | 配置 `{ check_comma_disparity: bool = false }` |
| `LineDiffResult` / `ByteDiffResult` | RAII 结果容器（含 `~init()` 自动释放），提供 `len()`、`countKind(k)`、`editDistance()`、`isIdentical()` |
| `areLinesEqual(a, b, check_comma)` | 行相等判定（支持尾随逗号容错） |
| `splitLines(alloc, text)` | 按 `\n` 切分为只读切片数组 |
| `diffLines(alloc, actual, expected, opts)` | 计算两段行切片的 Myers 最短编辑脚本（按正序返回） |
| `diffBytes(alloc, actual, expected)` | 计算两段字节序列的 Myers 最短编辑脚本 |
| `formatLineDiff(alloc, diffs)` | 格式化输出带 `+` / `-` / ` ` 前缀的差异文本 |

## 示例

```cp
import { diffLines, DiffOptions, formatLineDiff } from "cap:myers_diff";

const a: [2]string = ["alpha", "beta"];
const b: [2]string = ["alpha", "gamma"];
let res = diffLines(std.heap.page_allocator, a[0..], b[0..], DiffOptions{})?;
const rendered = formatLineDiff(std.heap.page_allocator, res.items)?;
defer std.heap.page_allocator.free(rendered);
// res 离开作用域自动触发 ~init() 释放
```

## 上游行为契约与防资源耗尽边界

1. **确定性规模上限防护（`InputsTooLarge` / `DiffTooLarge`）**：
   - Myers 算法最坏时间/空间复杂度为 $O(ND)$（$D$ 为编辑距离）；当输入行数/字节数超限或编辑距离过大时，引擎主动返回 `DiffError.InputsTooLarge` 或 `DiffError.DiffTooLarge`，防止极端无公共子序列的超大文本触发内存耗尽。
2. **尾随逗号容错（`DiffOptions.check_comma_disparity`）**：
   - 继承自 Bun `node:assert` 断言 diff 行为：对比 JSON / JS 对象快照时，启用 `check_comma_disparity = true` 可避免因最后一行多/少一个尾随逗号产生干扰性整行删除与新增。
3. **RAII 确定性释放**：
   - `LineDiffResult` 与 `ByteDiffResult` 均内置 `~init()` 析构函数，离开作用域自动释放差异项数组；`splitLines` 返回的切片零拷贝借用原 `text`，仅需释放外层切片表。
