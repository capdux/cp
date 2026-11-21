# `cap:erofs` — Linux EROFS Zero-Allocation Filesystem Parser

Pure CP, zero-allocation parser for Linux EROFS (Enhanced Read-Only File System) on-disk structures: 128-byte Superblock (`erofs_super_block`), CRC32C checksum verification, 32-byte Compact / 64-byte Extended Inodes, and zero-copy Directory Block (`erofs_dirent`) iteration.

## Protocol Constants

- Layout & Magic (Little-Endian):
  - `EROFS_SUPER_OFFSET = 1024`, `EROFS_SUPER_SIZE = 128`, `EROFS_SUPER_MAGIC = 0xE0F5E1E2`
  - `EROFS_INODE_COMPACT_SIZE = 32`, `EROFS_INODE_EXTENDED_SIZE = 64`, `EROFS_DIRENT_SIZE = 12`
- Feature Flags:
  - Compat: `FEATURE_COMPAT_SB_CHKSUM = 0x00000001`
  - Incompat: `FEATURE_INCOMPAT_ZERO_PADDING (0x0001)`, `FEATURE_INCOMPAT_COMPR_CFGS (0x0002)`, `FEATURE_INCOMPAT_BIG_PCLUSTER (0x0002)`, `FEATURE_INCOMPAT_CHUNKED_FILE (0x0004)`, `FEATURE_INCOMPAT_DEVICE_TABLE (0x0008)`, `FEATURE_INCOMPAT_ZTAILPACKING (0x0010)`, `FEATURE_INCOMPAT_FRAGMENTS (0x0020)`
- Inode Data Layouts (`(i_format >> 1) & 0x7`):
  - `LAYOUT_FLAT_PLAIN (0)`, `LAYOUT_FLAT_COMPRESSION_LEGACY (1)`, `LAYOUT_FLAT_INLINE (2)`, `LAYOUT_FLAT_COMPRESSION (3)`, `LAYOUT_CHUNK_BASED (4)`
- File Types:
  - `FT_UNKNOWN (0)`, `FT_REG_FILE (1)`, `FT_DIR (2)`, `FT_CHRDEV (3)`, `FT_BLKDEV (4)`, `FT_FIFO (5)`, `FT_SOCK (6)`, `FT_SYMLINK (7)`

## Core Types & Functions

| Symbol | Description |
| :--- | :--- |
| `erofsCrc32c(data: []const u8): u32` | Castagnoli CRC32C (`0x82F63B78`, init `0xFFFFFFFF`, no final XOR) treating `[4..8]` as zero. |
| `isErofsSuperblock(data: []const u8): bool` | Fast length (`>= 128`) and `0xE0F5E1E2` magic check. |
| `parseErofsSuperblock(sb_bytes: []const u8): ErofsSuperblock?` | Validates magic, `9 <= blkszbits <= 16`, and CRC32C when `FEATURE_COMPAT_SB_CHKSUM` is set. |
| `ErofsSuperblock` methods | `blockSize()`, `hasChecksum()`, `hasChunkedFile()`, `hasFragments()`, `volumeNameSlice()`, `nidToOffset(nid)`. |
| `parseErofsInode(data: []const u8): ErofsInode?` | Unified parser for 32B Compact (`version == 0`) and 64B Extended (`version == 1`) inodes (`isCompressed()`, `isInline()`, `isRegular()`, `isDirectory()`). |
| `iterErofsDirBlock(block_bytes: []const u8): ErofsDirIter?` | Zero-copy directory block iterator validating `dirent[0].nameoff % 12 == 0` and bounds. |

## Example

```cp
import {
    parseErofsSuperblock,
    parseErofsInode,
    iterErofsDirBlock,
} from "cap:erofs";

fn inspect(sb_bytes: []const u8, dir_block: []const u8) {
    if (sb := parseErofsSuperblock(sb_bytes)) {
        * = sb.blockSize();
        * = sb.nidToOffset(sb.root_nid as u64);
        * = sb.volumeNameSlice();
    }
    if (iter := iterErofsDirBlock(dir_block)) {
        let it = iter;
        while (ent := it.next()) {
            * = ent.nid;
            * = ent.name;
        }
    }
}
```
