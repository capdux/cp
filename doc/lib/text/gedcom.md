# `cap:gedcom` — GEDCOM 5.5.1 / 7.0 家谱与谱系数据零分配流式解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:text`，导入请使用 `import { ... } from "cap:text"`。

`cap:gedcom` 提供纯 CP、零堆分配的 FamilySearch GEDCOM 5.5.1 与 7.0 家谱数据流式解析能力，支持 `<level> [@xref@] <TAG> [value]` 行解析、层级跳跃校验、`0 HEAD` 元数据提取、顶层 `INDI`（个人）与 `FAM`（家庭）实体迭代以及单遍结构统计。

## 导入方式

```cp
import {
    GedcomLine,
    GedcomHeader,
    GedcomIndividual,
    GedcomFamily,
    GedcomStats,
    GedcomLineIter,
    GedcomIndiIter,
    GedcomFamIter,
    isGedcom,
    parseGedcomLine,
    parseGedcomHeader,
    summarizeGedcom,
} from "cap:text";
```

## 数据结构

### `GedcomLine`
- `level: u16` — 层级号
- `xref: []const u8` — 可选交叉引用 ID（如 `"@I1@"`）
- `tag: []const u8` — GEDCOM 标签（如 `"HEAD"`, `"INDI"`, `"FAM"`, `"BIRT"`, `"TRLR"`）
- `value: []const u8` — 可选值切片
- 方法 `isPointerValue(): bool` — 判断 `value` 是否为 `@pointer@` 交叉引用指针

### `GedcomHeader`
- `gedc_version: []const u8`（`GEDC -> VERS`，如 `"5.5.1"` / `"7.0"`）
- `charset: []const u8`（`CHAR`，如 `"UTF-8"`）
- `source_system: []const u8`（`SOUR`）
- `language: []const u8`（`LANG`）

### `GedcomIndividual` & `GedcomFamily`
- `GedcomIndividual`：`xref`, `name`, `sex`, `birth_date`, `birth_place`, `death_date`, `death_place`, `famc_xref`, `fams_xref`
- `GedcomFamily`：`xref`, `husb_xref`, `wife_xref`, `marr_date`, `child_count: u32`

### `GedcomStats`
- `line_count`, `indi_count`, `fam_count`, `sour_count`, `note_count: u32`, `max_level: u16`, `has_trailer: bool`

## 核心函数与迭代器

- `isGedcom(data: []const u8): bool` — 跳过可选 UTF-8 BOM 与空行，校验首行匹配 `"0 HEAD"`
- `parseGedcomLine(raw_line: []const u8): GedcomLine?` — 解析单条 GEDCOM 行
- `parseGedcomHeader(data: []const u8): GedcomHeader?` — 提取 `0 HEAD` 头部元数据
- `GedcomLineIter.init(data)` / `.next()` — 逐行迭代（兼容 `\r\n`/`\n`/`\r`，严格校验 `level <= prev_level + 1`）
- `GedcomIndiIter.init(data)` / `.next()` — 零分配迭代全部顶层个人记录
- `GedcomFamIter.init(data)` / `.next()` — 零分配迭代全部顶层家庭记录
- `summarizeGedcom(data): GedcomStats` — 单遍汇总统计

## 快速示例

```cp
import { parseGedcomHeader, GedcomIndiIter, summarizeGedcom } from "cap:text";

fn loadPedigree(ged_bytes: []const u8) {
    if (hdr := parseGedcomHeader(ged_bytes)) {
        * = hdr.gedc_version;
    }
    let indis = GedcomIndiIter.init(ged_bytes);
    while (p := indis.next()) {
        * = p.name;
    }
    const stats = summarizeGedcom(ged_bytes);
    * = stats.has_trailer;
}
```
