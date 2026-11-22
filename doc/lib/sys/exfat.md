# `cap:exfat` — Microsoft exFAT 引导扇区、校验和与目录项零分配解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:sys`，导入请使用 `import { ... } from "cap:sys"`。

纯 CP、零堆分配的 Microsoft exFAT 文件系统解析库，支持 512B Main Boot Sector 严格校验解析、簇到 LBA 换算、11 扇区 Boot Region 32-bit 校验和、目录项集合 16-bit 校验和、File / StreamExtension / FileName (UTF-16LE) 解析以及自动跳过已删除项的 32B 目录项迭代器。

## 导入

```cp
import {
    EXFAT_OEM_ID,
    EXFAT_BOOT_SIGNATURE,
    EXFAT_MIN_BOOT_SIZE,
    EXFAT_DIR_ENTRY_SIZE,
    ENTRY_EOD,
    ENTRY_ALLOC_BITMAP,
    ENTRY_UPCASE_TABLE,
    ENTRY_VOLUME_LABEL,
    ENTRY_FILE,
    ENTRY_STREAM_EXT,
    ENTRY_FILE_NAME,
    ATTR_READ_ONLY,
    ATTR_HIDDEN,
    ATTR_SYSTEM,
    ATTR_DIRECTORY,
    ATTR_ARCHIVE,
    STREAM_ALLOC_POSSIBLE,
    STREAM_NO_FAT_CHAIN,
    ExfatBootSector,
    isExfatBootSector,
    parseExfatBootSector,
    exfatBootChecksum,
    exfatEntrySetChecksum,
    ExfatFileEntry,
    parseFileEntry,
    ExfatStreamExt,
    parseStreamExt,
    decodeFileNameAscii,
    ExfatRawEntry,
    ExfatDirIter,
} from "cap:sys";
```

## 核心 API

- **引导扇区解析**：
  - `isExfatBootSector(data: []const u8): bool`
  - `parseExfatBootSector(sector0: []const u8): ExfatBootSector?` — 严格校验 `"EXFAT   "`、`0xAA55`、`9 <= bytes_per_sector_shift <= 12`、`bytes_per_sector_shift + sectors_per_cluster_shift <= 25`、`number_of_fats in 1..2` 与 `first_cluster_of_root >= 2`。
  - `ExfatBootSector` 方法：`bytesPerSector()`、`sectorsPerCluster()`、`clusterSizeBytes()`、`clusterToLba(cluster)`、`isDirty()`。
- **校验和算法**：
  - `exfatBootChecksum(boot_region_11_sectors: []const u8): u32` — 跳过偏移 `106, 107, 112` 的 32-bit 右旋累加校验和。
  - `exfatEntrySetChecksum(entries_bytes: []const u8): u16` — 跳过首条目偏移 `2, 3` 的 16-bit 右旋累加校验和。
- **目录项与迭代**：
  - `ExfatDirIter.init(data)` / `next(*_)` — 每次步进 32B，遇 `ENTRY_EOD (0x00)` 终止，自动跳过未置 `InUse` 位（`0x80`）的已删除项。
  - `parseFileEntry(raw)`（附 `isDirectory()`）、`parseStreamExt(raw)`（附 `isContiguous()` / `isAllocationPossible()`）、`decodeFileNameAscii(raw, out)`（从 `0xC1` 条目提取最多 15 个 UTF-16LE 字符）。

## 示例

```cp
import { parseExfatBootSector, ExfatDirIter, parseFileEntry, parseStreamExt, decodeFileNameAscii } from "cap:sys";

fn inspectExfat(sector0: []const u8, dir_cluster: []const u8) {
    if (bs := parseExfatBootSector(sector0)) {
        * = bs.clusterSizeBytes();
        * = bs.clusterToLba(bs.first_cluster_of_root);
        let it = ExfatDirIter.init(dir_cluster);
        while (true) {
            if (ent := it.next()) {
                if (ent.isFile()) {
                    * = parseFileEntry(ent.raw[0..]);
                } else if (ent.isStreamExt()) {
                    * = parseStreamExt(ent.raw[0..]);
                } else if (ent.isFileName()) {
                    let name_buf: [15]u8 = [0; 15];
                    * = decodeFileNameAscii(ent.raw[0..], name_buf[0..]);
                }
            } else {
                break;
            }
        }
    }
}
```
