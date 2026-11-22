# `cap:ntfs` — Microsoft NTFS VBR、MFT 记录与 Data Runlist 零分配解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:sys`，导入请使用 `import { ... } from "cap:sys"`。

> **模块路径**: `lib/ntfs/`  
> **导入协议**: `import { NTFS_VBR_SIZE, NTFS_OEM_ID, ATTR_STANDARD_INFO, ATTR_DATA, parseNtfsVbr, isNtfsVbr, parseNtfsMftRecord, NtfsRunIter } from "cap:sys";`  
> **动态挂载**: `import cap from "cap";` -> `cap.ntfs.*` 编译期按需惰性挂载

---

## 一、 架构概述

`cap:ntfs` 是纯 CP 实现、**零堆分配（zero-alloc）**、零 C/POSIX 外部依赖的 Microsoft NTFS 文件系统底层元数据解析库：

1. **512B Volume Boot Record (`parseNtfsVbr` / `isNtfsVbr`)**：
   - 严格校验 `"NTFS    "` OEM ID、`0xAA55` 签名、合法扇区大小与 2 次幂每簇扇区数。
   - 自动按 NTFS 规范计算 `mftRecordSizeBytes()`（负数 `clusters_per_mft_record` 按 `1 << (-v)` 计算，如 `-10 -> 1024B`）与 `mftByteOffset()`。
2. **MFT `"FILE"` 记录与常驻/非常驻属性迭代 (`parseNtfsMftRecord` / `NtfsAttrIter`)**：
   - 零拷贝解析常驻属性（`resident_value`）与非常驻属性（`real_size` / `alloc_size` / `runlist_bytes`）以及 UTF-16LE 属性名。
   - 遇 `0xFFFFFFFF` 正常结束，严格校验 `total_len >= 16` 与记录边界防死循环。
3. **变长 Data Runlist 解码 (`NtfsRunIter`)**：
   - 按低/高 nibble `(len_sz, off_sz)` 零分配解码无符号簇长度与有符号增量 LCN 偏移，精准识别稀疏区（`off_sz == 0`）。

---

## 二、 核心 API 速查表 (`lib/ntfs/ntfs.cp`)

| 符号 / 方法 | 签名与说明 |
| :--- | :--- |
| 协议常量 | `NTFS_VBR_SIZE (512)`, `NTFS_OEM_ID ("NTFS    ")`, `NTFS_BOOT_SIGNATURE (0xAA55)`, `NTFS_FILE_MAGIC ("FILE")`, `NTFS_ATTR_END (0xFFFFFFFF)`；`MFT_FLAG_IN_USE`, `MFT_FLAG_DIRECTORY`；`ATTR_STANDARD_INFO (0x10)` .. `ATTR_REPARSE_POINT (0xC0)` |
| `NtfsVbr` | `isNtfsVbr(data)`、`parseNtfsVbr(sector0)`；含 `clusterSizeBytes()`, `mftRecordSizeBytes()`, `mftByteOffset()` |
| `NtfsMftRecord` | `parseNtfsMftRecord(record_bytes)`；含 `isInUse()`, `isDirectory()`, `attrs()` |
| `NtfsAttr` / `NtfsAttrIter` | 常驻/非常驻属性视图与迭代器（含 `runs()`, `hasError()`） |
| `NtfsDataRun` / `NtfsRunIter` | `NtfsRunIter.init(runlist_bytes)`；支持正/负增量 LCN 与稀疏 Run (`is_sparse`) |

---

## 三、 使用示例

```typescript
import { parseNtfsVbr, parseNtfsMftRecord, ATTR_DATA } from "cap:sys";

fn demo(sector0: []const u8, mft_rec: []const u8) {
    if (vbr := parseNtfsVbr(sector0)) {
        const mft_off = vbr.mftByteOffset();
        * = mft_off;
    }
    if (rec := parseNtfsMftRecord(mft_rec)) {
        let it = rec.attrs();
        while (attr := it.next()) {
            if (attr.attr_type == ATTR_DATA && attr.non_resident) {
                let rit = attr.runs();
                while (run := rit.next()) {
                    * = run;
                }
            }
        }
    }
}
```
