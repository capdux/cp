# `cap:xfs` — Linux XFS v4/v5 Zero-Allocation Filesystem Parser

> [!NOTE]
> 本子模块已归并至统一主包 `cap:sys`，导入请使用 `import { ... } from "cap:sys"`。

Pure CP, zero-allocation parser for Linux XFS big-endian on-disk structures: v4/v5 Superblock (`xfs_dsb`) with v5 CRC32C verification, Inode Core (`xfs_dinode_core`), and 128-bit packed BMBT Extent Records (`xfs_bmbt_rec`).

## Protocol Constants

- Magics & Sizes (Big-Endian):
  - `XFS_SB_MAGIC = 0x58465342` (`"XFSB"`)
  - `XFS_SB_MIN_SIZE = 264`, `XFS_SB_SECTOR_SIZE = 512`, `XFS_SB_CRC_OFFSET = 224`
  - `XFS_INODE_MAGIC = 0x494E` (`"IN"`), `XFS_INODE_CORE_SIZE = 96`
  - `XFS_AGF_MAGIC = 0x58414746` (`"XAGF"`), `XFS_AGI_MAGIC = 0x58414749` (`"XAGI"`)
  - `XFS_BMBT_REC_SIZE = 16`
- Superblock Version (`versionnum & 0x000F`):
  - `XFS_SB_VERSION_4 = 4`, `XFS_SB_VERSION_5 = 5`
- Inode Data Fork Formats:
  - `FMT_DEV = 0`, `FMT_LOCAL = 1`, `FMT_EXTENTS = 2`, `FMT_BTREE = 3`
- v5 Feature Flags:
  - `RO_COMPAT_FINOBT = 0x01`, `RO_COMPAT_RMAPBT = 0x02`, `RO_COMPAT_REFLINK = 0x04`
  - `INCOMPAT_FTYPE = 0x01`, `INCOMPAT_BIGTIME = 0x08`

## Core Types & Functions

| Symbol | Description |
| :--- | :--- |
| `xfsCrc32c(data: []const u8): u32` | Standard iSCSI / XFS CRC32C (reflected Castagnoli `0x82F63B78`, final XOR). |
| `xfsCrc32cWithZeroField(data: []const u8, zero_offset: usize): u32` | CRC32C treating `[zero_offset..zero_offset+4]` as zero bytes (used for v5 superblock offset `224`). |
| `isXfsSuperblock(data: []const u8): bool` | Fast length (`>= 264`) and `"XFSB"` magic check. |
| `parseXfsSuperblock(sector0: []const u8): XfsSuperblock?` | Validates magic, version 4/5, power-of-two `blocksize`/`sectsize`/`inodesize` logs, and v5 sector CRC32C. |
| `XfsSuperblock` methods | `version()`, `isV5()`, `hasReflink()`, `hasFtype()`, `totalBytes()`, `labelSlice()`. |
| `parseXfsInodeCore(data: []const u8): XfsInodeCore?` | Parses 96B big-endian `"IN"` inode core (`isV3()`, `isRegular()`, `isDirectory()`). |
| `decodeXfsBmbtRec(rec16: []const u8): XfsBmbtRec?` | Unpacks 128-bit BMBT extent: `is_unwritten` (1b), `startoff` (54b), `startblock` (52b), `blockcount` (21b). |
| `encodeXfsBmbtRec(rec: XfsBmbtRec, out: *[]u8): usize?` | Packs `XfsBmbtRec` into 16 big-endian bytes; rejects undersized buffers or bitfield overflow. |

## Example

```cp
import {
    parseXfsSuperblock,
    decodeXfsBmbtRec,
    encodeXfsBmbtRec,
    XfsBmbtRec,
} from "cap:sys";

fn inspect(sector0: []const u8) {
    if (sb := parseXfsSuperblock(sector0)) {
        * = sb.isV5();
        * = sb.hasReflink();
        * = sb.totalBytes();
        * = sb.labelSlice();
    }
}
```
