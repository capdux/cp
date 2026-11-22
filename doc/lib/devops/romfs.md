# `cap:romfs` — Linux 内核 `romfs` 只读文件系统零分配解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:devops`，导入请使用 `import { ... } from "cap:devops"`。

纯 CP、零堆分配的 Linux `romfs`（`-rom1fs-` 大端 16B 对齐只读文件系统）解析库，支持超级块与文件头 32-bit 大端校验和校验、16B 对齐目录树零拷贝遍历、文件/符号链接零拷贝内容切片以及环形 `next_offset` 防死循环保护。

## 导入

```cp
import {
    ROMFS_MAGIC,
    ROMFS_ALIGN,
    ROMFS_MAX_CHECKSUM_LEN,
    ROMFS_TYPE_MASK,
    ROMFS_EXEC_BIT,
    ROMFS_NEXT_MASK,
    TYPE_HARD_LINK,
    TYPE_DIRECTORY,
    TYPE_REGULAR,
    TYPE_SYMLINK,
    TYPE_BLOCK_DEV,
    TYPE_CHAR_DEV,
    TYPE_SOCKET,
    TYPE_FIFO,
    romfsChecksum,
    RomfsSuperblock,
    RomfsEntry,
    RomfsDirIter,
    RomfsImage,
    isRomfs,
    parseRomfs,
} from "cap:devops";
```

## 核心 API

- **校验和与镜像解析**：
  - `romfsChecksum(data: []const u8): u32` — 大端 32-bit 环绕字累加（合法超级块与文件头求和为 `0`）。
  - `isRomfs(data: []const u8): bool`
  - `parseRomfs(data: []const u8): RomfsImage?` — 严格校验 `"-rom1fs-"`、`full_size <= data.len`、前 `min(512, full_size)` 字节校验和为 `0` 与 `volume_name` NUL 终止。
- **目录与文件访问**：
  - `RomfsImage`：`rootEntries()`、`entryAt(offset)`、`dirEntries(dir_entry)`、`findInDir(dir_offset, name)`。
  - `RomfsEntry`：`isDir()`、`isRegular()`、`isSymlink()`、`firstChildOffset()`，零拷贝暴露 `name` 与 `data` 切片。
  - `RomfsDirIter`：沿 `next_offset` 迭代同级条目，内置递增防环保护与最大步数保护（`<= 4096`）。

## 示例

```cp
import { parseRomfs } from "cap:devops";

fn readInit(img_bytes: []const u8) {
    if (img := parseRomfs(img_bytes)) {
        if (ent := img.findInDir(img.superblock.first_file_offset, "init")) {
            if (ent.isRegular()) {
                * = ent.data;
            }
        }
    }
}
```
