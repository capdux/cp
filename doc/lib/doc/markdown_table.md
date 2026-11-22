# `cap:markdown_table` — GFM 管道表格解析与单元格转义库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:doc`，导入请使用 `import { ... } from "cap:doc"`。

> 参考来源：/data/dev/bun/src/md/parser.cp, /data/dev/bun/docs/runtime/markdown.mdx | 吸收日期：2026-11-22

## 概述

`cap:markdown_table` 吸收自 Bun 原生 Markdown 解析器（`src/md/parser.cp` 与 `docs/runtime/markdown.mdx`），提供纯 CP、零外部依赖、零堆分配的 GitHub Flavored Markdown（GFM §4.10）管道表格行单元格迭代、对齐分隔行解析与 `\|` 单元格转义/反转义工具。

## 上游 GFM 规范与边界契约

- **转义管道符保护（`\|`）**：遵循 GFM §4.10 规范，单元格内以反斜杠转义的 `\|` 属于单元格内容而非列分隔符；`TableRowIter` 在切分列时自动跳过转义管道符，并支持可选的首尾外边框管道符（`| a | b |` 与 `a | b` 均兼容），自动裁剪每个单元格首尾空白。
- **对齐分隔行语法（`ColAlign`）**：
  - `---` → `ColAlign.none`（默认对齐）；
  - `:---` → `ColAlign.left`（左对齐）；
  - `:---:` → `ColAlign.center`（居中对齐）；
  - `---:` → `ColAlign.right`（右对齐）；
  - 每个分隔单元格至少需包含 1 个连字符 `-`，含非法字符时返回 `null`。
- **列数上限防护**：`parseDelimiterRow(raw_line, out_aligns)` 写入调用方栈数组 `out_aligns`，若列数超出 `out_aligns.len` 或存在非法分隔单元格，安全返回 `null`。

## 导出 API

| 符号 | 说明 |
| :--- | :--- |
| `ColAlign` | 枚举：`none` / `left` / `center` / `right` |
| `parseDelimiterCell(raw_cell): ColAlign?` | 解析单个分隔单元格对齐方式 |
| `TableRowIter` | `init(raw_line)`、`next(*_): ([]const u8)?` — 零拷贝逐列迭代单元格原始切片 |
| `parseDelimiterRow(raw_line, out_aligns: []ColAlign): usize?` | 解析整行对齐分隔行并返回列数 |
| `unescapeTableCell(raw_cell, out): ([]const u8)?` | 将 `\|` 还原为 `|` 写入 `out` |
| `escapeTableCell(text, out): ([]const u8)?` | 将单元格文本中的 `|` 转义为 `\|` 写入 `out` |

## 使用示例

```cp
import { ColAlign, TableRowIter, parseDelimiterRow, unescapeTableCell } from "cap:doc";

fn demo() {
    let aligns: [8]ColAlign = [ColAlign.none; 8];
    const ncols = parseDelimiterRow("| :--- | :---: | ---: |", aligns[0..]).?; // 3
    let it = TableRowIter.init("| key | a \\| b | 42 |");
    while (cell := it.next()) {
        let buf: [64]u8 = undefined;
        * = unescapeTableCell(cell, buf[0..]);
    }
    * = ncols;
}
```
