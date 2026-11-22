# `cap:ext4` — Linux ext2 / ext3 / ext4 零分配文件系统解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:sys`，导入请使用 `import { ... } from "cap:sys"`。

纯 CP、零堆分配（zero-alloc）的 Linux ext2/ext3/ext4 超级块（支持 64-bit 特性）、Inode、Extent 树（48-bit 物理块与 Uninitialized Extent）及线性目录项解析库。

## 导入

```cp
import {
    EXT4_SUPER_OFFSET,
    EXT4_SUPER_SIZE,
    EXT4_SUPER_MAGIC,
    EXT4_EXTENT_MAGIC,
    EXT4_ROOT_INO,
    COMPAT_HAS_JOURNAL,
    INCOMPAT_FILETYPE,
    INCOMPAT_EXTENTS,
    INCOMPAT_64BIT,
    RO_COMPAT_SPARSE_SUPER,
    RO_COMPAT_LARGE_FILE,
    RO_COMPAT_METADATA_CSUM,
    EXT4_EXTENTS_FL,
    S_IFMT,
    S_IFREG,
    S_IFDIR,
    S_IFLNK,
    FT_UNKNOWN,
    FT_REG_FILE,
    FT_DIR,
    FT_CHRDEV,
    FT_BLKDEV,
    FT_FIFO,
    FT_SOCK,
    FT_SYMLINK,
    Ext4Superblock,
    isExt4Superblock,
    parseExt4Superblock,
    Ext4Inode,
    parseExt4Inode,
    Ext4ExtentHeader,
    parseExt4ExtentHeader,
    Ext4Extent,
    Ext4ExtentIter,
    iterExt4Extents,
    Ext4DirEntry,
    Ext4DirIter,
} from "cap:sys";
```

## 核心 API

- **超级块**：`isExt4Superblock(sb_bytes)`、`parseExt4Superblock(sb_bytes): Ext4Superblock?`（`hasJournal()`, `hasExtents()`, `is64Bit()`, `groupCount()`, `volumeNameSlice()`）。
- **Inode**：`parseExt4Inode(bytes): Ext4Inode?`（合并 64-bit 文件大小与 32-bit UID/GID，`isRegular()`, `isDirectory()`, `isSymlink()`, `usesExtents()`, `permissions()`）。
- **Extent 树**：`parseExt4ExtentHeader(block_area)`、`iterExt4Extents(block_area): Ext4ExtentIter?`（`Ext4Extent` 提供 48-bit `physical_block`、`isUninitialized()`、`blockLen()`）。
- **线性目录块**：`Ext4DirIter`（`init`, `next`, `hasError`，自动跳过 `inode == 0` 空洞并校验 `rec_len >= 8` 与 4 字节对齐）。

## 示例

```cp
import { parseExt4Superblock, parseExt4Inode, iterExt4Extents } from "cap:sys";

fn example(sb_bytes: []const u8, inode_bytes: []const u8) {
    if (sb := parseExt4Superblock(sb_bytes)) {
        * = sb.block_size;
        * = sb.groupCount();
    }
    if (ino := parseExt4Inode(inode_bytes)) {
        if (ino.usesExtents()) {
            if (it_val := iterExt4Extents(ino.block_area[0..])) {
                let it = it_val;
                while (true) {
                    if (ext := it.next()) {
                        * = ext.physical_block;
                    } else {
                        break;
                    }
                }
            }
        }
    }
}
```
