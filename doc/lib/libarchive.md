# `cap:libarchive` — 纯 CP 统一归档格式探测与 Ar / CPIO / USTAR Tar 读写标准库

## 1. 概述

`cap:libarchive`（`cap.libarchive`）提供零 C 外部依赖的纯 CP 工业级归档处理能力，覆盖统一归档格式魔数探测、路径穿越与防炸弹守卫、Unix `ar`（`.a` / `.deb`，含奇偶对齐与 GNU `//`、BSD `#1/` 长文件名）、SVR4 `cpio`（`newc` `070701` / CRC `070702`，含 4 字节对齐与 `"TRAILER!!!"` 终结块）以及 USTAR Tar 编解码。

## 2. 核心类型

- `ArchiveFormat`：`Ar` | `CpioNewc` | `CpioCrc` | `Tar` | `Zip` | `Gzip` | `Zstd` | `Unknown`
- `ArchiveError`：`TruncatedHeader` | `InvalidMagic` | `InvalidHeader` | `InvalidChecksum` | `PathTraversal` | `TooManyEntries` | `SizeLimitExceeded` | `UnsupportedFormat` | `OutOfMemory`
- `ArchiveEntryKind`：`File` | `Directory` | `Symlink` | `Other`
- `ArchiveEntry`：`{ name: []const u8, data: []const u8, mode: u32, uid: u32, gid: u32, mtime: u64, kind: ArchiveEntryKind, link_name: []const u8 }`
- `ArchiveLimits`：`{ max_entries: usize, max_total_bytes: usize, max_entry_bytes: usize, allow_absolute_paths: bool }`

## 3. API 速查

- `detectArchiveFormat(data: []const u8): ArchiveFormat` — 识别 Ar / CPIO / Tar / Zip / Gzip / Zstd 魔数
- `validateArchivePath(path: []const u8, allow_abs: bool): ArchiveError!void` — 拦截空路径、空字节、`..` 穿越与未授权绝对路径
- `freeEntries(alloc: std.mem.Allocator, entries: []ArchiveEntry)` — 释放解码返回的 `[]ArchiveEntry` 及其内部切片
- `encodeAr(alloc, entries)` / `decodeAr(alloc, data, limits)` — Unix `ar` 编解码
- `encodeCpioNewc(alloc, entries)` / `decodeCpioNewc(alloc, data, limits)` — SVR4 `cpio` (`070701`/`070702`) 编解码
- `encodeUstarTar(alloc, entries)` / `decodeUstarTar(alloc, data, limits)` — USTAR Tar 编解码

## 4. 示例

```cp
import { detectArchiveFormat, encodeAr, decodeAr, ArchiveEntry, ArchiveLimits, freeEntries } from "cap:libarchive";
import cap from "cap";

const entries = [_]ArchiveEntry{
    .{ .name = "hello.txt", .data = "world" },
};
const blob = try encodeAr(alloc, &entries);
defer alloc.free(blob);
const fmt = detectArchiveFormat(blob); // .Ar
const decoded = try decodeAr(alloc, blob, ArchiveLimits{});
defer freeEntries(alloc, decoded);
```
