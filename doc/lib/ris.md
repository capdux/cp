# `cap:ris` — Research Information Systems (`.ris`) 学术文献引文零分配解析库

`cap:ris` 提供纯 CP、零堆分配的 Research Information Systems (`.ris`) 学术文献引文解析与统计能力，支持 `TY  -` / `ER  -` 记录流切分、多行续行标签值、UTF-8 BOM 与 CRLF/LF 换行兼容、自定义标签查询及单遍文献集统计。

## 导入方式

```cp
import {
    RisTag,
    RisRecord,
    RisStats,
    RisTagIter,
    RisIter,
    isRis,
    summarizeRis,
} from "cap:ris";
```

## 数据结构

### `RisTag`
- `code: [2]u8` — 2 字节大写标签码（如 `"TY"`, `"AU"`, `"TI"`, `"T1"`, `"JO"`, `"PY"`, `"DO"`, `"AB"`, `"KW"`, `"SN"`, `"ER"`）
- `value: []const u8` — 零拷贝值切片（自动合并后续非标签续行）
- 方法 `isCode(tag_2b: []const u8): bool` — 判断标签码是否匹配

### `RisRecord`
- `ref_type: []const u8`（`TY`，如 `"JOUR"`, `"BOOK"`, `"CONF"`, `"THES"`, `"ELEC"`, `"RPRT"`）
- `title: []const u8`（`TI` / `T1`）
- `first_author: []const u8`（首条 `AU` / `A1`）、`author_count: u32`
- `journal: []const u8`（`JO` / `JF` / `T2`）
- `year: []const u8`（`PY` / `Y1`）
- `volume` (`VL`), `issue` (`IS`), `start_page` (`SP`), `end_page` (`EP`), `doi` (`DO`), `url` (`UR`), `abstract_text` (`AB` / `N2`), `raw_record: []const u8`
- 方法 `tags(): RisTagIter` — 迭代该条记录内全部 RIS 标签
- 方法 `findTag(tag_2b: []const u8): ([]const u8)?` — 查询任意 2 字节标签（如 `"SN"`, `"PB"`）

### `RisStats`
- `record_count`, `journal_count`, `book_count`, `conf_count`, `with_doi_count`, `total_authors`, `total_keywords: u32`

## 核心函数与迭代器

- `isRis(data: []const u8): bool` — 跳过可选 UTF-8 BOM 与空行，检测 `"TY  - "` 开头
- `RisTagIter.init(record_slice)` / `.next()` — 单条记录内零分配标签迭代
- `RisIter.init(data)` / `.next()` — 多条文献记录零分配迭代（兼容缺失 `ER  -` 容错）
- `summarizeRis(data: []const u8): RisStats` — 单遍统计文献库摘要

## 快速示例

```cp
import { RisIter, summarizeRis } from "cap:ris";

fn scanCitations(ris_bytes: []const u8) {
    let it = RisIter.init(ris_bytes);
    while (rec := it.next()) {
        * = rec.title;
        if (issn := rec.findTag("SN")) {
            * = issn;
        }
    }
    const stats = summarizeRis(ris_bytes);
    * = stats.with_doi_count;
}
```
