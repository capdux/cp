# `cap:mobi` — 零分配 Amazon Mobipocket / Kindle (`.mobi`/`.azw`) 解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:text`，导入请使用 `import { ... } from "cap:text"`。

> **模块路径**：`lib/mobi/`（导入方式：`import { ... } from "cap:text";`）  
> **特性**：100% 纯 CP、零堆分配、大端序完整支持 PalmDB 头、多 Record 切片、PalmDOC 头、MOBI 头与 EXTH 元数据流式解析。

---

## 一、 核心常量与枚举

- **结构常量**：`PDB_HEADER_SIZE = 78`、`PDB_RECORD_ENTRY_SIZE = 8`、`MOBI_TYPE_SIG = "BOOK"`、`MOBI_CREATOR_SIG = "MOBI"`、`MOBI_MAGIC = "MOBI"`、`EXTH_MAGIC = "EXTH"`
- **压缩枚举 `MobiCompression`**：`none (1)`、`palmdoc (2)`、`huff_cdic (17480)`、`unknown (0)`
- **编码枚举 `MobiEncoding`**：`cp1252 (1252)`、`utf8 (65001)`、`unknown (0)`
- **常用 EXTH 类型**：`EXTH_AUTHOR (100)`、`EXTH_PUBLISHER (101)`、`EXTH_DESCRIPTION (103)`、`EXTH_ISBN (104)`、`EXTH_SUBJECT (105)`、`EXTH_PUB_DATE (106)`、`EXTH_ASIN (113)`、`EXTH_UPDATED_TITLE (503)`、`EXTH_LANGUAGE (524)`

---

## 二、 核心 API

- `isMobi(data)` — 校验 `"BOOKMOBI"` / `"TEXtREAd"` 魔数与最小长度
- `parsePdbHeader(data): PdbHeader?` — 解析 78B PalmDB 头，提供 `recordOffset(data, idx)` 与 `recordSlice(data, idx)`（严格校验偏移单调递增）
- `parseMobiHeader(data): MobiHeader?` — 解析 Record 0 中的 16B PalmDOC 头与 MOBI 头（提取编码、全名与精确 `exth_offset`）
- `ExthIter.init(data, header)` / `next(*_): ExthRecord?` & `findExthRecord(data, header, record_type): ([]const u8)?`

---

## 三、 使用示例

```typescript
import { parseMobiHeader, findExthRecord, EXTH_AUTHOR } from "cap:text";

fn inspectMobi(bytes: []const u8) {
    if (hdr := parseMobiHeader(bytes)) {
        * = hdr.full_name.len;
        if (author := findExthRecord(bytes, hdr, EXTH_AUTHOR)) {
            * = author.len;
        }
    }
}
```
