# `cap:marc` — 零分配 ISO 2709 / MARC 21 图书馆编目二进制解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:text`，导入请使用 `import { ... } from "cap:text"`。

> **模块路径**：`lib/marc/`（入口 `lib/marc/index.cp`）  
> **特性**：纯 CP 实现、零堆分配（Zero-Alloc）、零 C 依赖，完整遵循 ISO 2709 / Library of Congress MARC 21 规范：解析 24 字节定宽 ASCII Leader、校验 `0x1E` 目录终止符与 12 字节目录表对齐、零拷贝提取控制字段（`001..009`）与数据字段（`>= 010` 含指示符与 `0x1F` 子字段），并支持多记录拼接 `.mrc` 编目流式迭代。

---

## 一、导入方式

```cp
import {
    MARC_LEADER_SIZE,
    MARC_DIR_ENTRY_SIZE,
    MARC_RECORD_TERMINATOR,
    MARC_FIELD_TERMINATOR,
    MARC_SUBFIELD_DELIM,
    MarcLeader,
    MarcSubfield,
    MarcSubfieldIter,
    MarcField,
    MarcFieldIter,
    MarcRecord,
    MarcRecordIter,
    isMarcRecord,
    parseMarcLeader,
} from "cap:text";
```

---

## 二、核心常量与数据结构

| 类型 / 常量 | 字段 / 方法 | 说明 |
| :--- | :--- | :--- |
| 常量 | `MARC_LEADER_SIZE (24)`, `MARC_DIR_ENTRY_SIZE (12)`, `MARC_RECORD_TERMINATOR (0x1D)`, `MARC_FIELD_TERMINATOR (0x1E)`, `MARC_SUBFIELD_DELIM (0x1F)` | ISO 2709 定界符与定长常量 |
| `isMarcRecord(data)` | `(data: []const u8): bool` | 校验 Leader 长度、末字节 `0x1D` 及 MARC 21 指示符/子字段计数 `'2'` |
| `MarcLeader` / `parseMarcLeader(data)` | `record_length`, `record_status`, `record_type`, `bib_level`, `char_encoding`, `base_addr_of_data`, `encoding_level`; `isUtf8()`, `fieldCount()` | 解析并严格校验 24B Leader 与目录区边界 |
| `MarcField` / `MarcFieldIter` | `tag: [3]u8`, `ind1`, `ind2`, `data`; `isTag(t3)`, `isControlField()`, `subfields()`, `findSubfield(code)` | 迭代 12B 目录表并零拷贝切片每个控制/数据字段 |
| `MarcSubfield` / `MarcSubfieldIter` | `code: u8`, `value: []const u8` | 按 `0x1F` 切分数据字段内所有子字段 |
| `MarcRecord` / `MarcRecordIter` | `leader`, `raw`; `fields(): MarcFieldIter` | 按每条记录 `record_length` 精确步进迭代 `.mrc` 流 |

---

## 三、使用示例

```cp
import { MarcRecordIter } from "cap:text";

fn inspectCatalog(mrc: []const u8) {
    let recs = MarcRecordIter.init(mrc);
    while (rec := recs.next()) {
        let fields = rec.fields();
        while (f := fields.next()) {
            if (f.isTag("245")) {
                if (title := f.findSubfield('a')) {
                    * = title;
                }
            }
        }
    }
}
```
