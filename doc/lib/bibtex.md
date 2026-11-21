# `cap:bibtex` — 纯 CP BibTeX (`.bib`) 学术文献条目零分配解析与格式化库

## 概述

`cap:bibtex` 提供纯 CP、零外部依赖、零堆分配的 BibTeX `.bib` 学术引用文献条目与字段流式解析器、大小写不敏感字段查询及规范化字段格式化器。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `BibEntry` | 文献条目：`entry_type`, `cite_key`, `body`（零拷贝原输入切片） |
| `BibField` | 字段键值：`name`, `value`（已剥离外层 `{...}` 或 `"..."`） |
| `BibEntryIter` | 文献条目流式迭代器：`init(input)`、`next(*_): BibEntry?`（跳过 `%` 注释与 `@comment`/`@preamble`/`@string`，支持 `{...}` 与 `(...)` 括号平衡匹配） |
| `BibFieldIter` | 字段体流式迭代器：`init(body)`、`next(*_): BibField?`（支持 `{val}`、`"val"` 与裸数字/原子值） |
| `findBibField(entry, field_name)` | ASCII 大小写不敏感查找指定字段值 |
| `formatBibField(name, value, out)` | 输出规范化字段行 `"  name = {value},\n"`，缓冲不足返回 `null` |

## 示例

```cp
import { BibEntryIter, findBibField, formatBibField } from "cap:bibtex";

fn example(bib: []const u8): bool {
    let it = BibEntryIter.init(bib);
    if (entry := it.next()) {
        const title = findBibField(entry, "TITLE") ?? return false;
        let buf: [256]u8 = undefined;
        return formatBibField("title", title, buf[0..]) != null;
    }
    return false;
}
```
