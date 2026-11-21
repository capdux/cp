# `cap:squashfs` — Linux SquashFS v4.0 只读文件系统零分配解析库

纯 CP、零堆分配的 Linux SquashFS v4.0 镜像解析库，支持 96 字节 Superblock 解析与严格一致性校验、`InodeRef` 48-bit/13-bit 编解码、2 字节 Metadata Block 压缩/非压缩头解析、Data Block 稀疏/压缩位解码以及 16 字节 Fragment Table Entry 解析。

## 导入

```cp
import {
    SQUASHFS_MAGIC,
    SQUASHFS_SUPER_SIZE,
    SQUASHFS_MAJOR,
    SQUASHFS_MINOR,
    SQUASHFS_METADATA_MAX_SIZE,
    SQUASHFS_META_UNCOMPRESSED_BIT,
    SQUASHFS_DATA_UNCOMPRESSED_BIT,
    SQUASHFS_INVALID_OFFSET,
    COMP_GZIP,
    COMP_LZMA,
    COMP_LZO,
    COMP_XZ,
    COMP_LZ4,
    COMP_ZSTD,
    FLAG_UNCOMPRESSED_INODES,
    FLAG_UNCOMPRESSED_DATA,
    FLAG_UNCOMPRESSED_FRAGMENTS,
    FLAG_NO_FRAGMENTS,
    FLAG_ALWAYS_FRAGMENTS,
    FLAG_DUPLICATES,
    FLAG_EXPORTABLE,
    FLAG_UNCOMPRESSED_XATTRS,
    FLAG_NO_XATTRS,
    FLAG_COMPRESSOR_OPTIONS,
    FLAG_UNCOMPRESSED_IDS,
    INODE_DIR,
    INODE_REG,
    INODE_SYMLINK,
    INODE_BLKDEV,
    INODE_CHRDEV,
    INODE_FIFO,
    INODE_SOCK,
    SquashfsSuperblock,
    isSquashfs,
    parseSquashfsSuperblock,
    SquashfsInodeRef,
    decodeInodeRef,
    encodeInodeRef,
    SquashfsMetaHeader,
    parseMetaBlock,
    SquashfsDataBlockSize,
    decodeDataBlockSize,
    SquashfsFragmentEntry,
    parseFragmentEntry,
} from "cap:squashfs";
```

## 核心 API

- **Superblock 解析**：`isSquashfs(data): bool`、`parseSquashfsSuperblock(data): SquashfsSuperblock?`（附 `compressionName()`, `hasXattrs()`, `hasFragments()`, `hasExportTable()`, `inodesUncompressed()`, `dataUncompressed()`）。
- **Inode 引用编解码**：`decodeInodeRef(ref: u64): SquashfsInodeRef`、`encodeInodeRef(block_offset: u64, inner_offset: u16): u64?`。
- **Metadata / Data / Fragment**：`parseMetaBlock(data): SquashfsMetaHeader?`、`decodeDataBlockSize(raw: u32): SquashfsDataBlockSize`、`parseFragmentEntry(data): SquashfsFragmentEntry?`。

## 示例

```cp
import { parseSquashfsSuperblock, decodeInodeRef } from "cap:squashfs";

fn example(img: []const u8) {
    if (sb := parseSquashfsSuperblock(img)) {
        * = sb.compressionName();
        const root = decodeInodeRef(sb.root_inode_ref);
        * = root.block_offset;
    }
}
```
