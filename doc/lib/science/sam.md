# `cap:sam` — 零分配 SAM v1.6 生物信息学序列比对解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:science`，导入请使用 `import { ... } from "cap:science"`。

> **模块路径**：`lib/sam/`（入口 `lib/sam/index.cp`）  
> **特性**：纯 CP 实现、零堆分配（Zero-Alloc）、零 C 依赖，完整遵循 SAM v1.6 官方规范，支持 12 个标准 Bitwise FLAG 位掩码、CIGAR 迭代与参考/读段跨度计算、`@HD`/`@SQ`/`@RG` 头行与标签提取、11 必选列 + 可选 `TAG:TYPE:VALUE` 比对记录流式迭代与单遍统计汇总。

---

## 一、导入方式

```cp
import {
    SAM_FLAG_PAIRED,
    SAM_FLAG_PROPER_PAIR,
    SAM_FLAG_UNMAPPED,
    SAM_FLAG_MATE_UNMAPPED,
    SAM_FLAG_REVERSE,
    SAM_FLAG_MATE_REVERSE,
    SAM_FLAG_READ1,
    SAM_FLAG_READ2,
    SAM_FLAG_SECONDARY,
    SAM_FLAG_QC_FAIL,
    SAM_FLAG_DUPLICATE,
    SAM_FLAG_SUPPLEMENTARY,
    CigarOp,
    SamHeaderLine,
    SamRecord,
    SamStats,
    CigarIter,
    SamHeaderIter,
    SamRecordIter,
    cigarRefLen,
    cigarQueryLen,
    findSamHeaderTag,
    parseSamLine,
    summarizeSam,
} from "cap:science";
```

---

## 二、核心数据结构

| 类型 | 字段 / 方法 | 说明 |
| :--- | :--- | :--- |
| `CigarOp` | `len: u32`, `op: u8` | 单个 CIGAR 操作（支持 `M`/`I`/`D`/`N`/`S`/`H`/`P`/`=`/`X`） |
| `SamHeaderLine` | `record_type: []const u8`, `fields_slice: []const u8` | 零拷贝 `@HD`/`@SQ`/`@RG`/`@PG`/`@CO` 头行 |
| `SamRecord` | `qname`, `flag`, `rname`, `pos`, `mapq`, `cigar`, `rnext`, `pnext`, `tlen`, `seq`, `qual`, `optional_tags` | 11 必选列 + 可选 TAG 区；附 `isMapped()`, `isReverse()`, `isPaired()`, `isProperPair()`, `isSecondary()`, `isDuplicate()`, `isSupplementary()`, `findTag(tag_2b)` |
| `SamStats` | `total_reads`, `mapped_reads`, `unmapped_reads`, `paired_reads`, `proper_pair_reads`, `reverse_reads`, `duplicate_reads`, `secondary_reads` | 单遍比对汇总统计 |

---

## 三、核心 API

- `CigarIter.init(cigar)` / `next(*_)` — 零分配迭代 CIGAR 操作（`"*"` 产出空序列，非法操作符/零长度/溢出置 `had_error = true`）。
- `cigarRefLen(cigar)` / `cigarQueryLen(cigar)` — 累加参考序列消耗 (`M`/`D`/`N`/`=`/`X`) 与读段消耗 (`M`/`I`/`S`/`=`/`X`)。
- `SamHeaderIter.init(data)` / `findSamHeaderTag(fields_slice, tag_2b)` — 迭代 `@` 头行并提取 2B 标签值。
- `parseSamLine(line)` / `SamRecordIter.init(data)` / `summarizeSam(data)` — 解析比对记录、跳过头行/空行并单遍汇总统计。

---

## 四、使用示例

```cp
import { parseSamLine, cigarRefLen, summarizeSam } from "cap:science";

const line = "r001\t99\tchr1\t100\t60\t50M2I10D5S\t=\t250\t200\tACGT\tIIII\tNM:i:2\tMD:Z:50";
if (rec := parseSamLine(line)) {
    const nm = rec.findTag("NM"); // "2"
    const rlen = cigarRefLen(rec.cigar); // 60
    * = nm;
    * = rlen;
}
```
