# `cap:zip` — 纯 CP PKZIP 归档构建与安全解析库

> **模块路径**: `lib/zip/`  
> **导入协议**: `import { ZipBuilder, ZipArchive, ZipEntry, CompressionMethod, ZipError, readArchive, readArchiveWithLimits } from "cap:zip";`  
> **动态挂载**: `import cap from "cap";` -> `cap.zip.*` 编译期按需惰性挂载

---

## 一、 架构概述

`cap:zip` 是 100% 纯 CP 实现、零外部 C 依赖（无 `std.posix` / `@import("bun")`）的标准 PKZIP 归档构建与安全读取库，底层直接复用 `cap:deflate` 的 `compressDeflate`、`decompressDeflateWithMaxSize` 与 `crc32`：

1. **标准 PKZIP 序列化 (`ZipBuilder`)**：
   - 支持无压缩 `Stored` (`0`) 与 DEFLATE 压缩 `Deflated` (`8`) 两种模式；
   - 自动写入完整的 Local File Header (`0x04034b50`)、Central Directory Header (`0x02014b50`) 与 End of Central Directory Record (`0x06054b50`)；
   - 支持空文件、多级目录路径及 UTF-8 文件名标记（`0x0800`）。
2. **安全归档读取 (`readArchive`, `readArchiveWithLimits`)**：
   - 定位 EOCD 并遍历 Central Directory 与对应 Local File Header；
   - **路径穿越拦截 (`UnsafePathTraversal`)**：严格拒绝 `..` 路径段、前导 `/`、反斜杠 `\`、空文件名与 `\0` 字节；
   - **完整性与边界校验**：严格校验 IEEE 802.3 CRC-32（`ChecksumMismatch`）、截断归档（`TruncatedArchive`）、无效魔数（`InvalidArchive`）及不支持的压缩/加密标志（`UnsupportedCompression`）；
   - **资源耗尽防护**：通过 `max_entries`（`TooManyEntries`）与 `max_entry_bytes`（`EntryTooLarge`）在分配与解压阶段双重拦截 Zip Bomb。

---

## 二、 核心 API 速查表 (`lib/zip/zip.cp`)

| 符号 / 方法 | 签名与说明 |
| :--- | :--- |
| `ZipError` | `InvalidArchive`, `UnsupportedCompression`, `ChecksumMismatch`, `TruncatedArchive`, `UnsafePathTraversal`, `TooManyEntries`, `EntryTooLarge`, `OutOfMemory` |
| `CompressionMethod` | `Stored = 0`, `Deflated = 8` |
| `ZipEntry` | `{ name: []const u8, compression: CompressionMethod, crc32: u32, compressed_size: u32, uncompressed_size: u32, data: []const u8 }` |
| `ZipArchive` | `{ entries: []ZipEntry }`，提供 `find(name): (*const ZipEntry)?` 与 `deinit(*_, allocator)` |
| `ZipBuilder.init(allocator)` | 初始化 PKZIP 归档构建器 |
| `ZipBuilder.deinit(*_)` | 释放构建器内部缓冲的所有条目 |
| `ZipBuilder.addStored(*_, name, data)` | 添加 `Stored (0)` 未压缩条目 |
| `ZipBuilder.addDeflated(*_, name, data)` | 添加 `Deflated (8)` 压缩条目 |
| `ZipBuilder.finish(allocator)` | 序列化生成完整 PKZIP 字节切片（调用方负责 `allocator.free`） |
| `readArchive(allocator, zip_bytes)` | 使用默认上限（最多 65535 条目，单条目最大 256 MiB）安全解析 ZIP |
| `readArchiveWithLimits(allocator, zip_bytes, max_entries, max_entry_bytes)` | 使用显式条目数与单条目字节上限安全解析 ZIP |

---

## 三、 使用示例

```typescript
const std = @import("std");
import {
    ZipBuilder,
    ZipArchive,
    readArchive,
    readArchiveWithLimits,
} from "cap:zip";

fn demo(alloc: std.mem.Allocator): !void {
    let builder = ZipBuilder.init(alloc);
    defer builder.deinit();

    builder.addStored("meta/version.txt", "1.0.0")?;
    builder.addDeflated("logs/app.log", "repeated line\nrepeated line\nrepeated line\n")?;

    const zip_bytes = builder.finish(alloc)?;
    defer alloc.free(zip_bytes);

    let archive = readArchiveWithLimits(alloc, zip_bytes, 64, 1024 * 1024)?;
    defer archive.deinit(alloc);

    if (archive.find("meta/version.txt")) |entry| {
        _ = entry.data;
    }
}
```
