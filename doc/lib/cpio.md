# `cap:cpio` — 纯 CP 零分配 Unix CPIO (`newc`/`crc`/`odc`) 归档解析库

## 概述

`cap:cpio` 提供纯 CP、零外部依赖、零堆分配的 Unix CPIO 归档流式解析器，完整支持 Linux initramfs / RPM 使用的 SVR4 `newc` (`070701`)、`crc` (`070702`) 4 字节对齐格式及 POSIX `odc` (`070707`) 八进制格式。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| 常量 | `CPIO_MAGIC_NEWC = "070701"`, `CPIO_MAGIC_CRC = "070702"`, `CPIO_MAGIC_ODC = "070707"`, `CPIO_NEWC_HEADER_SIZE = 110`, `CPIO_ODC_HEADER_SIZE = 76`, `CPIO_TRAILER = "TRAILER!!!"` |
| `CpioFormat` | 格式枚举：`newc`, `crc`, `odc`, `unknown` |
| `detectCpioFormat(data)` / `isCpio(data)` | 魔数格式识别 |
| `CpioEntry` | 归档条目：`format`, `ino`, `mode`, `uid`, `gid`, `nlink`, `mtime`, `filesize`, `checksum`, `name`（去 `\0`）, `data`（零拷贝）；只读方法 `isDir(): bool`、`isRegular(): bool`、`isSymlink(): bool` |
| `CpioIter` | 流式迭代器：`init(data)`, `next(*_): CpioEntry?`（遇 `TRAILER!!!` 停止，严格校验 hex/octal 与边界） |
| `findCpioEntry(data, name)` / `countCpioEntries(data)` | 按路径名查找条目与统计有效条目总数 |

## 示例

```cp
import { isCpio, findCpioEntry, countCpioEntries } from "cap:cpio";

fn inspectInitramfs(archive: []const u8): ([]const u8)? {
    if (!isCpio(archive) || countCpioEntries(archive) == 0) return null;
    if (init_bin := findCpioEntry(archive, "init")) {
        return init_bin.data;
    }
    return null;
}
```
