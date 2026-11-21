# `cap:cif` — 零分配 IUCr CIF 1.1 / mmCIF 晶体学与生物大分子结构解析库

> **模块路径**：`lib/cif/`（入口 `lib/cif/index.cp`）  
> **特性**：纯 CP 实现、零堆分配（Zero-Alloc）、零 C 依赖，完整遵循 IUCr CIF 1.1 / mmCIF (PDBx) 规范：支持大小写不敏感 `data_`/`save_`/`loop_`、跳过 `#` 注释、仅在后跟空白或 EOF 时闭合的单双引号字符串、行首 `;...;` 多行文本字段、`"."` 缺失 / `"?"` 未知哨兵值区分，以及 `loop_` 表格列查找与行分组零分配迭代。

---

## 一、导入方式

```cp
import {
    CifTokenKind,
    CifToken,
    CifEntry,
    CifStats,
    isCif,
    CifLexer,
    CifPairIter,
    findCifTag,
    CifRow,
    CifRowIter,
    CifLoop,
    CifLoopIter,
    summarizeCif,
} from "cap:cif";
```

---

## 二、核心数据结构与 API

| 类型 / 函数 | 字段 / 方法 | 说明 |
| :--- | :--- | :--- |
| `CifTokenKind: u8` | `data_block`, `save_frame`, `loop_kw`, `tag`, `value` | CIF 词法单元分类 |
| `CifToken` | `kind`, `text`, `is_quoted`; `isMissing()`, `isUnknown()`, `blockName()` | 零拷贝词法 Token，区分未加引号 `.`/`?` 与字面量引号值 |
| `isCif(data)` | `(data: []const u8): bool` | 跳过空白与 `#` 注释后校验 `data_` 开头（大小写不敏感） |
| `CifLexer` | `CifLexer.init(data)` / `next(*_): CifToken?` / `hasError()` | 零分配流式 CIF 词法器 |
| `CifEntry` / `CifPairIter` | `tag`, `value`, `is_missing`, `is_unknown` | 迭代所有顶层 `_tag value` 对（自动跳过 `loop_` 表格区） |
| `findCifTag(data, tag_name)` | `([]const u8)?` | 大小写不敏感查找顶层单值标签（如 `_cell_length_a`、`_entry.id`） |
| `CifLoop` / `CifLoopIter` | `colCount(): u32`, `colIndex(tag_name): u32?`, `rows(): CifRowIter` | 遍历每个 `loop_` 表格并按列头查找索引 |
| `CifRow` / `CifRowIter` | `colCount(): u32`, `get(col_idx): ([]const u8)?`, `getToken(col_idx): CifToken?` | 按 `col_count` 零分配分组迭代每行数据 |
| `CifStats` / `summarizeCif(data)` | `data_block_count`, `tag_pair_count`, `loop_count`, `loop_row_count` | 单遍扫描统计结构摘要，语法错误或不完整行返回 `null` |

---

## 三、使用示例

```cp
import { findCifTag, CifLoopIter } from "cap:cif";

fn inspectCif(data: []const u8) {
    if (entry_id := findCifTag(data, "_entry.id")) {
        * = entry_id;
    }

    let loops = CifLoopIter.init(data);
    while (lp := loops.next()) {
        if (sym_col := lp.colIndex("_atom_site.type_symbol")) {
            let rows = lp.rows();
            while (row := rows.next()) {
                if (sym := row.get(sym_col)) {
                    * = sym;
                }
            }
        }
    }
}
```
