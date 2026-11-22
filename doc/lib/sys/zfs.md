# `cap:zfs` — OpenZFS Zero-Allocation On-Disk Format Parser

> [!NOTE]
> 本子模块已归并至统一主包 `cap:sys`，导入请使用 `import { ... } from "cap:sys"`。

Pure CP, zero-allocation parser for OpenZFS on-disk structures: dual-endian Uberblock (`uberblock_t`), 128-byte Block Pointer (`blkptr_t`) with 3x DVA (`dva_t`), and Fletcher-4 256-bit checksums.

## Protocol Constants

- Magic & Sizes:
  - `ZFS_UBERBLOCK_MAGIC = 0x00BAB10C`, `ZFS_UBERBLOCK_MAGIC_SWAPPED = 0x0CB1BA00`
  - `ZFS_UBERBLOCK_SIZE = 1024`, `ZFS_BLKPTR_SIZE = 128`, `ZFS_SECTOR_SHIFT = 9`
  - `ZFS_VDEV_LABEL_SIZE = 262144` (256 KiB), `ZFS_VDEV_BOOT_OFFSET = 0x400000` (4 MiB)
- Compression IDs:
  - `COMP_INHERIT (0)`, `COMP_ON (1)`, `COMP_OFF (2)`, `COMP_LZJB (3)`, `COMP_EMPTY (4)`, `COMP_GZIP_1 (5)`, `COMP_LZ4 (15)`, `COMP_ZSTD (16)`
- Checksum IDs:
  - `CKSUM_INHERIT (0)`, `CKSUM_ON (1)`, `CKSUM_OFF (2)`, `CKSUM_FLETCHER_2 (6)`, `CKSUM_FLETCHER_4 (7)`, `CKSUM_SHA256 (8)`, `CKSUM_SHA512 (11)`, `CKSUM_SKEIN (12)`, `CKSUM_EDONR (13)`, `CKSUM_BLAKE3 (14)`

## Core Types & Functions

| Symbol | Description |
| :--- | :--- |
| `ZfsChecksum` | 256-bit checksum (`words: [4]u64`). |
| `zfsFletcher4Le(data: []const u8): ZfsChecksum` | Fletcher-4 256-bit checksum over little-endian 32-bit words (`a += w; b += a; c += b; d += c`). |
| `zfsFletcher4Be(data: []const u8): ZfsChecksum` | Fletcher-4 256-bit checksum over big-endian 32-bit words. |
| `ZfsDva` | 16-byte Data Virtual Address (`byteOffset()` adds 4 MiB VDEV label offset, `asizeBytes()`, `isValid()`). |
| `ZfsBlkPtr` | 128-byte Block Pointer (`dvas: [3]ZfsDva`, `lsize_bytes`, `psize_bytes`, `compression`, `checksum_type`, `isHole()`, etc.). |
| `ZfsUberblock` | Parsed Uberblock (`is_big_endian`, `version`, `txg`, `guid_sum`, `timestamp`, `rootbp`, `software_version`). |
| `isZfsUberblock(data: []const u8): bool` | Fast length (`>= 1024`) and dual-endian magic check. |
| `parseZfsUberblock(data: []const u8): ZfsUberblock?` | Auto-detects byte order, validates `version >= 1 && txg >= 1`, and decodes embedded `rootbp`. |
| `parseZfsBlkPtr(data: []const u8, is_big_endian: bool): ZfsBlkPtr?` | Decodes a 128-byte `blkptr_t`. |
| `selectActiveUberblock(ub1: ZfsUberblock?, ub2: ZfsUberblock?): ZfsUberblock?` | Returns the Uberblock with the higher `txg`. |

## Example

```cp
import {
    parseZfsUberblock,
    selectActiveUberblock,
    zfsFletcher4Le,
} from "cap:sys";

fn pickActive(slot0: []const u8, slot1: []const u8) {
    const u0 = parseZfsUberblock(slot0);
    const u1 = parseZfsUberblock(slot1);
    if (active := selectActiveUberblock(u0, u1)) {
        * = active.txg;
        * = active.rootbp.dvas[0].byteOffset();
    }
    * = zfsFletcher4Le(slot0);
}
```
