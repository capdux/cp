# `cap:macho` — 纯 CP Apple Mach-O 32/64-bit 与 Fat Universal 二进制解析库

## 概述

`cap:macho` 提供纯 CP、零外部依赖、零堆分配的 Apple Mach-O（Thin 32/64-bit，支持大端与小端字节序）及 Fat Universal Binary（32/64-bit）二进制魔数探测、头部解码、Load Command 安全迭代与架构切片列表解析器，全面防护畸形 `cmdsize` 与缓冲区越界。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `MachoKind` | 二进制格式枚举：`thin32`, `thin64`, `fat32`, `fat64`, `unknown` |
| `MachoHeader` | 头部元数据：`is_64bit`, `is_big_endian`, `cputype`, `cpusubtype`, `filetype`, `ncmds`, `sizeofcmds`, `flags`；方法：`isArm64()`, `isX86_64()`, `isExecutable()`, `isDylib()`, `headerSize()` |
| `MachoLoadCmd` | 零拷贝 Load Command：`cmd: u32`, `cmdsize: u32`, `payload: []const u8` |
| `FatArchEntry` | Fat 架构条目：`cputype`, `cpusubtype`, `offset: u64`, `size: u64`, `align_pow2: u32` |
| `detectMachoKind(data)` | 识别 8 种标准 Mach-O/Fat 魔数 |
| `parseMachoHeader(data)` | 校验头部长度并按字节序解码 `MachoHeader` |
| `MachoLoadCmdIter` | 流式 Load Command 迭代器：`init(data)`、`next(*_): MachoLoadCmd?`（严格校验 `cmdsize >= 8` 与边界） |
| `FatArchIter` | Fat Universal 架构列表迭代器：`init(data)`、`next(*_): FatArchEntry?` |

## 示例

```cp
import { detectMachoKind, parseMachoHeader, MachoLoadCmdIter, MachoKind } from "cap:macho";

fn isArm64Executable(bin: []const u8): bool {
    if (detectMachoKind(bin) != MachoKind.thin64) return false;
    if (hdr := parseMachoHeader(bin)) {
        return hdr.isArm64() && hdr.isExecutable();
    }
    return false;
}
```
