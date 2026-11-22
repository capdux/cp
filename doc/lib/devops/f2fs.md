# `cap:f2fs` — Linux F2FS 闪存友好文件系统零分配解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:devops`，导入请使用 `import { ... } from "cap:devops"`。

> **模块路径**: `lib/f2fs/`  
> **导入协议**: `import { F2FS_SUPER_MAGIC, f2fsCrc32, isF2fsSuperblock, parseF2fsSuperblock, parseF2fsCheckpoint, selectActiveCheckpoint } from "cap:devops";`  
> **动态挂载**: `import cap from "cap";` -> `cap.f2fs.*` 编译期按需惰性挂载

---

## 一、 架构概述

`cap:f2fs` 是纯 CP 实现、**零堆分配（zero-alloc）**、零 C/POSIX 外部依赖的 Linux F2FS（Flash-Friendly File System）超级块、双 Checkpoint 与 CRC32 校验解析库：

1. **Linux 内核兼容 `f2fsCrc32`**：
   - 使用标准 IEEE 802.3 反射多项式 `0xEDB88320`、自定义初值（默认 `F2FS_SUPER_MAGIC = 0xF2F52010`）、无最终按位取反，与 Linux `fs/f2fs/f2fs.h` 中 `f2fs_crc32()` 逐位一致。
2. **F2FS Superblock 严格解析**：
   - 校验魔数 `0xF2F52010`、`log_blocksize == 12` (4KB)、`log_sectorsize + log_sectors_per_block == log_blocksize`；
   - 当 `FEAT_SB_CHKSUM` 置位时自动对 `[0..checksum_offset]` 校验 `f2fsCrc32`；
   - 提供 `blockSize()`、`sectorSize()`、`segmentSizeBytes()`、`totalBytes(): u128`、特性位查询及 UTF-16LE 卷名 ASCII 解码 `decodeVolumeNameAscii(out)`。
3. **双 Checkpoint 校验与选主**：
   - `parseF2fsCheckpoint` 校验 `checksum_offset >= 192`、边界及 `f2fsCrc32`；
   - `selectActiveCheckpoint(c1, c2)` 自动选举 `checkpoint_ver` 更高且校验有效的活跃 Checkpoint。

---

## 二、 核心 API 速查表 (`lib/f2fs/f2fs.cp`)

| 类别 | 符号 / 方法 | 签名与说明 |
| :--- | :--- | :--- |
| 协议常量 | 基础常量 | `F2FS_SUPER_OFFSET (1024)`, `F2FS_SUPER_MAGIC (0xF2F52010)`, `F2FS_BLKSIZE (4096)`, `F2FS_SB_MIN_SIZE (1024)`, `F2FS_CP_MIN_SIZE (192)`, `F2FS_SB_FEATURE_OFFSET (2180)`, `F2FS_SB_CRC_OFFSET (3068)` |
| 协议常量 | 特性与检查点标志 | `FEAT_ENCRYPT`, `FEAT_BLKZONED`, `FEAT_ATOMIC_WRITE`, `FEAT_EXTRA_ATTR`, `FEAT_PRJQUOTA`, `FEAT_INODE_CHKSUM`, `FEAT_FLEXIBLE_INLINE_XATTR`, `FEAT_QUOTA_INO`, `FEAT_INODE_CRTIME`, `FEAT_LOST_FOUND`, `FEAT_VERITY`, `FEAT_SB_CHKSUM`, `FEAT_CASEFOLD`, `FEAT_COMPRESSION`, `CP_UMOUNT_FLAG`, `CP_ORPHAN_PRESENT_FLAG`, `CP_COMPACT_SUM_FLAG`, `CP_ERROR_FLAG`, `CP_FSCK_FLAG`, `CP_FASTBOOT_FLAG`, `CP_CRC_RECOVERY_FLAG`, `CP_TRIMMED_FLAG` |
| 校验和 | `f2fsCrc32` | `f2fsCrc32(seed: u32, data: []const u8): u32` |
| 数据模型 | `F2fsSuperblock` | 几何与布局字段 + `blockSize()`, `sectorSize()`, `segmentSizeBytes()`, `totalBytes()`, `hasEncrypt()`, `hasCompression()`, `hasSbChecksum()`, `decodeVolumeNameAscii(out)` |
| 数据模型 | `F2fsCheckpoint` | 版本/块数/标志字段 + `isCleanUnmount()`, `hasOrphans()`, `isFastboot()`, `hasError()` |
| 解析与选主 | 顶层函数 | `isF2fsSuperblock(data)`, `parseF2fsSuperblock(sb_bytes)`, `parseF2fsCheckpoint(cp_block)`, `selectActiveCheckpoint(c1, c2)` |

---

## 三、 使用示例

```typescript
import { parseF2fsSuperblock, parseF2fsCheckpoint, selectActiveCheckpoint } from "cap:devops";

fn inspectF2fs(sb_bytes: []const u8, cp1_block: []const u8, cp2_block: []const u8) {
    if (sb := parseF2fsSuperblock(sb_bytes)) {
        * = sb.totalBytes();
        let name: [64]u8 = [0; 64];
        * = sb.decodeVolumeNameAscii(name[0..]);
    }
    if (cp := selectActiveCheckpoint(parseF2fsCheckpoint(cp1_block), parseF2fsCheckpoint(cp2_block))) {
        * = cp.checkpoint_ver;
    }
}
```
