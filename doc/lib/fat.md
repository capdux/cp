# `cap:fat` — Microsoft FAT12 / FAT16 / FAT32 BPB 与目录项零分配解析库

> **模块路径**: `lib/fat/`  
> **导入协议**: `import { FatKind, FatBpb, FatDirEntry, FatDirIter, FatDateTime, parseFatBpb, isFatBootSector, decodeDosDateTime, fatShortNameChecksum } from "cap:fat";`  
> **动态挂载**: `import cap from "cap";` -> `cap.fat.*` 编译期按需惰性挂载

---

## 一、 架构概述

`cap:fat` 是纯 CP 实现、**零堆分配（zero-alloc）**、零 C/POSIX 外部依赖的 Microsoft FAT12 / FAT16 / FAT32 文件系统元数据解析库：

1. **BPB 引导扇区严格校验与自动分类 (`parseFatBpb` / `isFatBootSector`)**：
   - 校验 `0xAA55` 签名、`bytes_per_sector ∈ {512, 1024, 2048, 4096}`、`sectors_per_cluster` 为 2 的幂、`reserved_sectors >= 1`、`num_fats >= 1`、`fat_size > 0`。
   - 严格按微软 `fatgen103` 簇数阈值自动判定 `FAT12 (< 4085)`、`FAT16 (< 65525)`、`FAT32 (>= 65525)`，并提供 `fatStartLba()`、`rootDirStartLba()`、`dataStartLba()`、`clusterToLba(cluster)`、`clusterSizeBytes()`。
2. **32B 目录项与 8.3 短名格式化 (`FatDirEntry` / `FatDirIter`)**：
   - 自动拼接 32-bit `first_cluster`（高 16 位 + 低 16 位）、解码 MS-DOS 日期时间、格式化 8.3 文件名（如 `"README  TXT"` -> `"README.TXT"`）。
   - `FatDirIter` 自动跳过 `0xE5` 已删除条目、遇 `0x00` 截停，并支持按需跳过或保留 LFN 长文件名条目。
3. **DOS 日期时间与 LFN 校验和 (`decodeDosDateTime` / `fatShortNameChecksum`)**：
   - 精确实现 1980 纪元 2 秒精度 DOS 时间解码与微软 8-bit 旋转累加短名校验和。

---

## 二、 核心 API 速查表 (`lib/fat/fat.cp`)

| 符号 / 方法 | 签名与说明 |
| :--- | :--- |
| 协议与属性常量 | `FAT_BOOT_SECTOR_SIZE (512)`, `FAT_BOOT_SIGNATURE (0xAA55)`, `FAT_DIR_ENTRY_SIZE (32)`；`ATTR_READ_ONLY`, `ATTR_HIDDEN`, `ATTR_SYSTEM`, `ATTR_VOLUME_ID`, `ATTR_DIRECTORY`, `ATTR_ARCHIVE`, `ATTR_LFN` |
| `FatKind` | `fat12 = 12`, `fat16 = 16`, `fat32 = 32` |
| `FatBpb` | `parseFatBpb(sector0)` / `isFatBootSector(data)`；含 `fatStartLba()`, `rootDirStartLba()`, `dataStartLba()`, `clusterToLba(cluster)`, `clusterSizeBytes()` |
| `FatDateTime` | `decodeDosDateTime(date, time)` |
| `fatShortNameChecksum` | `fatShortNameChecksum(short_name: [11]u8): u8` |
| `FatDirEntry` / `FatDirIter` | `FatDirIter.init(data, skip_lfn)`；`FatDirEntry` 含 `isDirectory()`, `isVolumeId()`, `isLfn()`, `isReadOnly()`, `isHidden()`, `writeTime()`, `formatShortName(out)` |

---

## 三、 使用示例

```typescript
import { parseFatBpb, FatDirIter } from "cap:fat";

fn demo(sector0: []const u8, dir_buf: []const u8) {
    if (bpb := parseFatBpb(sector0)) {
        const lba = bpb.clusterToLba(2);
        * = lba;
    }
    let it = FatDirIter.init(dir_buf, true);
    while (entry := it.next()) {
        let name_buf: [16]u8 = [0; 16];
        const n = entry.formatShortName(name_buf[0..]);
        * = n;
    }
}
```
