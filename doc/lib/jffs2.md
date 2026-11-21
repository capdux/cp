# `cap:jffs2` — Linux JFFS2 闪存日志文件系统零分配解析库

> **模块路径**: `lib/jffs2/`  
> **导入协议**: `import { jffs2Crc32, isJffs2, parseJffs2NodeHeader, parseJffs2Inode, parseJffs2Dirent, iterJffs2Nodes } from "cap:jffs2";`  
> **动态挂载**: `import cap from "cap";` -> `cap.jffs2.*` 编译期按需惰性挂载

---

## 一、 架构概述

`cap:jffs2` 是纯 CP 实现、**零堆分配（zero-alloc）**、零 C/POSIX 外部依赖的 Linux JFFS2（Journalling Flash File System version 2）节点流解析库：

1. **JFFS2 无取反 CRC32 (`jffs2Crc32`)**：
   - 使用 IEEE 802.3 反射多项式 `0xEDB88320`、初值 `0x00000000`、无最终取反，与 Linux 内核 `fs/jffs2/crc32.h` 完全一致。
2. **4B 对齐闪存节点流迭代 (`Jffs2NodeIter`)**：
   - 自动跳过 `0xFFFF` 空白擦除区，严格校验每个节点的 `magic == 0x1985`、`12 <= totlen <= remaining` 与 `hdr_crc == jffs2Crc32(raw[0..8])`，遇畸形节点置 `hasError()`。
3. **Inode 与 Dirent 强类型校验解码**：
   - `parseJffs2Inode` 校验 `node_crc`（`raw[0..60]`）与 `data_crc`（`data`），零拷贝暴露压缩负载切片；
   - `parseJffs2Dirent` 校验 `node_crc`（`raw[0..32]`）与 `name_crc`（`name`），内置 `isDeletion()` 检测 `ino == 0` 删除墓碑。

---

## 二、 核心 API 速查表 (`lib/jffs2/jffs2.cp`)

| 类别 | 符号 / 方法 | 签名与说明 |
| :--- | :--- | :--- |
| 协议常量 | 魔数与尺寸 | `JFFS2_MAGIC_BITMASK (0x1985)`, `JFFS2_OLD_MAGIC_BITMASK (0x1984)`, `JFFS2_EMPTY_BITMASK (0xFFFF)`, `JFFS2_COMMON_HEADER_SIZE (12)`, `JFFS2_INODE_HEADER_SIZE (68)`, `JFFS2_DIRENT_HEADER_SIZE (40)` |
| 协议常量 | 节点/压缩/目录类型 | `NODETYPE_DIRENT` .. `NODETYPE_XREF`, `COMPR_NONE` .. `COMPR_LZO`, `DT_UNKNOWN` .. `DT_SOCK` |
| 校验和 | `jffs2Crc32` | `jffs2Crc32(data: []const u8): u32` |
| 数据模型 | `Jffs2NodeHeader` | `magic`, `nodetype`, `totlen`, `hdr_crc`, `offset`, `raw` |
| 数据模型 | `Jffs2RawInode` | `ino`, `version`, `mode`, `uid`, `gid`, `isize`, `atime`, `mtime`, `ctime`, `offset`, `csize`, `dsize`, `compr`, `usercompr`, `flags`, `data_crc`, `node_crc`, `data` |
| 数据模型 | `Jffs2RawDirent` | `pino`, `version`, `ino`, `mctime`, `nsize`, `dirent_type`, `node_crc`, `name_crc`, `name`, `isDeletion()` |
| 解析与迭代 | 顶层函数 | `isJffs2(data)`, `parseJffs2NodeHeader(data)`, `parseJffs2Inode(node)`, `parseJffs2Dirent(node)`, `iterJffs2Nodes(flash_bytes)` |

---

## 三、 使用示例

```typescript
import { iterJffs2Nodes, NODETYPE_INODE, NODETYPE_DIRENT, parseJffs2Inode, parseJffs2Dirent } from "cap:jffs2";

fn scanEraseblock(flash: []const u8) {
    let iter = iterJffs2Nodes(flash);
    while (node := iter.next()) {
        if (node.nodetype == NODETYPE_INODE) {
            if (ino := parseJffs2Inode(node)) {
                * = ino.data;
            }
        } else if (node.nodetype == NODETYPE_DIRENT) {
            if (de := parseJffs2Dirent(node)) {
                * = de.name;
            }
        }
    }
}
```
