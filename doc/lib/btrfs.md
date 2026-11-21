# `cap:btrfs` — Linux Btrfs Zero-Allocation Filesystem Parser

Pure CP, zero-allocation parser for Linux Btrfs on-disk structures: 4KB Superblock (`btrfs_super_block`), Btrfs CRC32C checksum verification, `sys_chunk_array` system chunk iteration, 17-byte B-Tree keys (`btrfs_disk_key`), and 101-byte B-Tree node/leaf headers (`btrfs_header`).

## Protocol Constants

- Layout & Magic:
  - `BTRFS_SUPER_OFFSET = 65536` (`0x10000`, primary superblock byte offset)
  - `BTRFS_SUPER_SIZE = 4096`
  - `BTRFS_MAGIC = "_BHRfS_M"` (at superblock offset `64`)
  - `BTRFS_TREE_HEADER_SIZE = 101`
  - `BTRFS_KEY_SIZE = 17`
  - `BTRFS_SYS_CHUNK_ARRAY_OFFSET = 811` (`0x32B`)
  - `BTRFS_SYS_CHUNK_ARRAY_MAX = 2048`
  - `BTRFS_CHUNK_FIXED_SIZE = 48`
  - `BTRFS_STRIPE_SIZE = 32`
- Checksum Types:
  - `CSUM_CRC32C = 0`, `CSUM_XXHASH64 = 1`, `CSUM_SHA256 = 2`, `CSUM_BLAKE2B = 3`
- Common B-Tree Key Types:
  - `KEY_INODE_ITEM = 1`, `KEY_DIR_ITEM = 84`, `KEY_EXTENT_DATA = 108`, `KEY_ROOT_ITEM = 132`, `KEY_CHUNK_ITEM = 228`
- Block Group / Chunk Profile Flags:
  - `BLOCK_GROUP_DATA = 0x01`, `BLOCK_GROUP_SYSTEM = 0x02`, `BLOCK_GROUP_METADATA = 0x04`
  - `BLOCK_GROUP_RAID0 = 0x08`, `BLOCK_GROUP_RAID1 = 0x10`, `BLOCK_GROUP_DUP = 0x20`
  - `BLOCK_GROUP_RAID10 = 0x40`, `BLOCK_GROUP_RAID5 = 0x80`, `BLOCK_GROUP_RAID6 = 0x100`

## CRC32C Note

`btrfsCrc32c(data: []const u8): u32` implements the exact on-disk Btrfs CRC32C:
- Reflected Castagnoli polynomial `0x82F63B78`
- Initial value `0xFFFFFFFF`
- **No final bitwise inversion** (standard iSCSI CRC32C equals `~btrfsCrc32c(data)`).

For superblocks with `csum_type == CSUM_CRC32C`, `parseBtrfsSuperblock` computes `btrfsCrc32c(sb4k[32..4096])` and verifies it matches the little-endian `u32` stored at `sb4k[0..4]`.

## Core Types & Functions

| Symbol | Description |
| :--- | :--- |
| `btrfsCrc32c(data: []const u8): u32` | Computes raw Btrfs on-disk CRC32C (no final XOR). |
| `isBtrfsSuperblock(sb4k: []const u8): bool` | Fast length (`>= 4096`) and `"_BHRfS_M"` magic check at offset 64. |
| `parseBtrfsSuperblock(sb4k: []const u8): BtrfsSuperblock?` | Validates magic, power-of-two `sectorsize`/`nodesize`, `sys_chunk_array_size <= 2048`, and CRC32C (when `csum_type == CSUM_CRC32C`). |
| `BtrfsSuperblock.labelSlice(): []const u8` | Returns NUL-terminated label slice from `label[0..256]`. |
| `BtrfsSuperblock.csumName(): []const u8` | Returns `"crc32c"`, `"xxhash64"`, `"sha256"`, `"blake2b"`, or `"unknown"`. |
| `BtrfsSuperblock.sysChunks(): BtrfsSysChunkIter` | Returns a zero-copy iterator over `sys_chunk_array`. |
| `BtrfsSysChunkIter.next(*_): BtrfsSysChunk?` | Decodes next `17B key + 48B chunk + num_stripes * 32B` entry. |
| `parseBtrfsKey(data: []const u8): BtrfsKey?` | Parses a 17-byte `btrfs_disk_key`. |
| `parseBtrfsTreeHeader(node: []const u8): BtrfsTreeHeader?` | Parses a 101-byte `btrfs_header` (`isLeaf()` returns `level == 0`). |

## Example

```cp
import {
    parseBtrfsSuperblock,
    parseBtrfsTreeHeader,
} from "cap:btrfs";

fn inspectSuperblock(sb4k: []const u8) {
    if (sb := parseBtrfsSuperblock(sb4k)) {
        const label = sb.labelSlice();
        const algo = sb.csumName();
        let iter = sb.sysChunks();
        while (chunk := iter.next()) {
            * = chunk.key.offset;
            * = chunk.first_devid;
            * = chunk.first_offset;
        }
        * = label;
        * = algo;
    }
}
```
