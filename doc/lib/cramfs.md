# `cap:cramfs` — Linux CramFS 压缩只读文件系统零分配解析库

> **模块路径**: `lib/cramfs/`  
> **导入协议**: `import { isCramfs, parseCramfsSuperblock, parseCramfsInode, iterCramfsDir, cramfsCrc32 } from "cap:cramfs";`  
> **动态挂载**: `import cap from "cap";` -> `cap.cramfs.*` 编译期按需惰性挂载

---

## 一、 架构概述

`cap:cramfs` 是纯 CP 实现、**零堆分配（zero-alloc）**、零 C/POSIX 外部依赖的 Linux CramFS（Compressed ROM File System）镜像解析库：

1. **76B 超级块与 IEEE CRC-32 校验**：
   - 校验小端魔数 `0x28CD3D45`、偏移 16 `"Compressed ROMFS"` 签名及 `76 <= size <= data.len`；
   - 当 `FLAG_FSID_VERSION_2` 置位时，零拷贝将 `[32..36)` 按 `0x00` 计入并校验整镜像 IEEE 802.3 CRC-32；同时向下兼容 v1 无 CRC 镜像。
2. **12B 位域紧缩 Inode 解码 (`CramfsInode`)**：
   - 精确拆解 `mode`、`uid`、24-bit `size`、8-bit `gid`、6-bit `namelen_words` 与 26-bit `offset_words`，提供 `nameBytesLen()`、`dataOffset()`、`isDir()`、`isRegular()`、`isSymlink()`、`permissions()`。
3. **零拷贝目录迭代 (`CramfsDirIter`)**：
   - `iterCramfsDir` 严格校验目录标志与 `[dataOffset .. dataOffset + size]` 边界，按 `12 + nameBytesLen()` 步进产出 `CramfsDirEntry` 并自动剥离尾部填充 NUL。

---

## 二、 核心 API 速查表 (`lib/cramfs/cramfs.cp`)

| 类别 | 符号 / 方法 | 签名与说明 |
| :--- | :--- | :--- |
| 协议常量 | 魔数与尺寸 | `CRAMFS_MAGIC (0x28CD3D45)`, `CRAMFS_SIGNATURE ("Compressed ROMFS")`, `CRAMFS_SUPER_SIZE (76)`, `CRAMFS_INODE_SIZE (12)`, `CRAMFS_CRC_OFFSET (32)` |
| 协议常量 | 标志与 POSIX 模式 | `FLAG_FSID_VERSION_2`, `FLAG_SORTED_DIRS`, `FLAG_HOLES`, `FLAG_WRONG_SIGNATURE`, `FLAG_SHIFTED_ROOT_OFFSET`, `FLAG_EXT_BLOCK_POINTERS`, `S_IFMT`, `S_IFDIR`, `S_IFREG`, `S_IFLNK` |
| 校验和 | `cramfsCrc32` | `cramfsCrc32(image: []const u8): u32` |
| 数据模型 | `CramfsInode` | `mode`, `uid`, `size`, `gid`, `namelen_words`, `offset_words`, `nameBytesLen()`, `dataOffset()`, `isDir()`, `isRegular()`, `isSymlink()`, `permissions()` |
| 数据模型 | `CramfsSuperblock` | `size`, `flags`, `future`, `crc`, `edition`, `blocks`, `files`, `name`, `root`, `nameSlice()`, `isVersion2()`, `hasSortedDirs()`, `hasHoles()` |
| 数据模型 | `CramfsDirEntry` & `CramfsDirIter` | `inode`, `name`, `next(*_): CramfsDirEntry?` |
| 解析函数 | 顶层入口 | `isCramfs(data)`, `parseCramfsInode(bytes12)`, `parseCramfsSuperblock(data)`, `iterCramfsDir(image, dir_inode)` |

---

## 三、 使用示例

```typescript
import { parseCramfsSuperblock, iterCramfsDir } from "cap:cramfs";

fn listRoot(image: []const u8) {
    if (sb := parseCramfsSuperblock(image)) {
        if (it := iterCramfsDir(image, sb.root)) {
            let iter = it;
            while (entry := iter.next()) {
                * = entry.name;
            }
        }
    }
}
```
