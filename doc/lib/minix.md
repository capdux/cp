# `cap:minix` — MINIX v1 / v2 / v3 经典文件系统零分配解析库

纯 CP、零堆分配的 MINIX v1 / v2 / v3 文件系统解析库，统一支持全部 5 种魔数（14B/30B/60B 文件名与 v3 可变块大小）、32B v1 与 64B v2/v3 Inode 解析以及 16B/32B/64B 定宽目录项零拷贝迭代。

## 导入

```cp
import {
    MINIX_SUPER_OFFSET,
    MINIX_BLOCK_SIZE,
    MINIX_V1_INODE_SIZE,
    MINIX_V2_INODE_SIZE,
    MINIX_V1_MAGIC,
    MINIX_V1_MAGIC2,
    MINIX_V2_MAGIC,
    MINIX_V2_MAGIC2,
    MINIX_V3_MAGIC,
    MINIX_VALID_FS,
    MINIX_ERROR_FS,
    S_IFMT,
    S_IFDIR,
    S_IFREG,
    S_IFLNK,
    MinixSuperblock,
    isMinixSuperblock,
    parseMinixSuperblock,
    MinixInode,
    parseMinixV1Inode,
    parseMinixV2Inode,
    MinixDirEntry,
    MinixDirIter,
    iterMinixDir,
} from "cap:minix";
```

## 核心 API

- **超级块解析**：
  - `isMinixSuperblock(sb_bytes: []const u8): bool`
  - `parseMinixSuperblock(sb_bytes: []const u8): MinixSuperblock?` — 自动识别 v1 (`0x137F`/`0x138F`)、v2 (`0x2468`/`0x2478`) 与 v3 (`0x4D5A`)，严格校验 `ninodes > 0`、`nzones > 0`、`log_zone_size <= 16` 及 v3 `block_size >= 1024` 且为 2 的幂。
  - `MinixSuperblock` 方法：`zoneSizeBytes()`, `inodeSizeBytes()`, `inodeTableByteOffset()`, `zoneToByteOffset(zone)`, `isClean()`。
- **Inode 解析**：
  - `parseMinixV1Inode(bytes32: []const u8): MinixInode?`
  - `parseMinixV2Inode(bytes64: []const u8): MinixInode?`
  - `MinixInode` 方法：`isDir()`, `isRegular()`, `isSymlink()`, `permissions()`。
- **目录块迭代**：
  - `iterMinixDir(sb: MinixSuperblock, dir_bytes: []const u8): MinixDirIter` — 按 `16B`/`32B` (`u16` ino) 或 `64B` (`u32` ino) 零拷贝迭代，自动跳过 `inode == 0` 空槽位。

## 示例

```cp
import { MINIX_SUPER_OFFSET, parseMinixSuperblock, iterMinixDir } from "cap:minix";

fn inspectMinix(image: []const u8, root_dir_block: []const u8) {
    if (sb := parseMinixSuperblock(image[MINIX_SUPER_OFFSET..])) {
        * = sb.inodeTableByteOffset();
        let it = iterMinixDir(sb, root_dir_block);
        while (true) {
            if (ent := it.next()) {
                * = ent.inode;
                * = ent.name;
            } else {
                break;
            }
        }
    }
}
```
