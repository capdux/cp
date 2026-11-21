# `cap:vhdx` — Microsoft VHDX 1.0 虚拟硬盘零分配解析库

> **模块路径**: `lib/vhdx/`  
> **导入协议**: `import { isVhdx, parseVhdxFileId, parseVhdxHeader, selectActiveHeader, parseVhdxRegionTable, decodeVhdxBatEntry, vhdxCrc32c } from "cap:vhdx";`  
> **动态挂载**: `import cap from "cap";` -> `cap.vhdx.*` 编译期按需惰性挂载

---

## 一、 架构概述

`cap:vhdx` 是纯 CP 实现、**零堆分配（zero-alloc）**、零 C/POSIX 外部依赖的 Microsoft VHDX 1.0（MS-VHDX 规范）虚拟硬盘元数据解析与校验库：

1. **File Type Identifier 解析**：
   - 校验偏移 `0` 的 8 字节 `"vhdxfile"` 签名，零拷贝切片提取 512 字节 UTF-16LE Creator 字段，支持 `decodeCreatorAscii` 安全解码至调用方缓冲区。
2. **CRC32C（Castagnoli `0x82F63B78` 反射）零拷贝校验**：
   - 提供 `vhdxCrc32c` 与 `vhdxCrc32cWithZeroField(data, zero_offset)`，计算 4KB Header / 64KB Region Table 校验和时直接跳过 4 字节 checksum 字段按 `0x00` 计入，不修改原始只读缓冲。
3. **双冗余 4KB Header 验证与选主**：
   - `parseVhdxHeader` 严格校验 `"head"` 签名、`version == 1` 与完整 4KB CRC32C；
   - `selectActiveHeader(h1, h2)` 根据 `sequence_number` 与有效性自动选取当前活跃主头。
4. **64KB Region Table 与 Well-Known GUID 查找**：
   - `parseVhdxRegionTable` 严格校验 `"regi"` 签名、`entry_count <= 2047` 与完整 64KB CRC32C；
   - `VhdxRegionIter` 零拷贝遍历 32 字节区域条目，内置 `findBat()` 与 `findMetadata()` 查找标准 mixed-endian GUID。
5. **64-bit BAT 条目位域解码**：
   - `decodeVhdxBatEntry` 提取低 3 位 Payload State（`0..7`）与高 44 位 `file_offset_mb`（`raw >> 20`），提供 `fileOffsetBytes()`、`isPresent()`、`isZero()` 语义查询。

---

## 二、 核心 API 速查表 (`lib/vhdx/vhdx.cp`)

| 类别 | 符号 / 方法 | 签名与说明 |
| :--- | :--- | :--- |
| 协议常量 | 签名与尺寸偏移 | `VHDX_FILE_ID_SIG ("vhdxfile")`, `VHDX_HEADER_SIG ("head")`, `VHDX_REGION_SIG ("regi")`, `VHDX_METADATA_SIG ("metadata")`, `VHDX_HEADER_SIZE (4096)`, `VHDX_REGION_SIZE (65536)`, `VHDX_HEADER1_OFFSET (65536)`, `VHDX_HEADER2_OFFSET (131072)`, `VHDX_REGION1_OFFSET (196608)`, `VHDX_REGION2_OFFSET (262144)` |
| 协议常量 | Well-Known GUID | `REGION_GUID_BAT` (`2DC27766-F623-4200-9D64-115E9BFD4A08`), `REGION_GUID_METADATA` (`8B7CA206-4790-4B9A-B8FE-575F050F886E`) |
| 协议常量 | BAT State | `BAT_NOT_PRESENT (0)`, `BAT_UNDEFINED (1)`, `BAT_ZERO (2)`, `BAT_UNMAPPED (3)`, `BAT_FULLY_PRESENT (6)`, `BAT_PARTIALLY_PRESENT (7)` |
| 校验和 | `vhdxCrc32c` | `vhdxCrc32c(data: []const u8): u32` |
| 校验和 | `vhdxCrc32cWithZeroField` | `vhdxCrc32cWithZeroField(data: []const u8, zero_offset: usize): u32` |
| 文件标识 | `VhdxFileId` | `creator_utf16le: []const u8`, `decodeCreatorAscii(out: []u8): usize` |
| 头部模型 | `VhdxHeader` | `checksum`, `sequence_number`, `file_write_guid`, `data_write_guid`, `log_guid`, `log_version`, `version`, `log_length`, `log_offset` |
| 区域条目 | `VhdxRegionEntry` | `guid`, `file_offset`, `length`, `required`, `isBat(): bool`, `isMetadata(): bool` |
| 区域表 | `VhdxRegionTable` & `VhdxRegionIter` | `entries(): VhdxRegionIter`, `findByGuid(target)`, `findBat()`, `findMetadata()`, `next(*_): VhdxRegionEntry?` |
| BAT 条目 | `VhdxBatEntry` | `state: u8`, `file_offset_mb: u64`, `fileOffsetBytes(): u64`, `isPresent(): bool`, `isZero(): bool` |
| 解析函数 | 顶层解析入口 | `isVhdx(data)`, `parseVhdxFileId(data)`, `parseVhdxHeader(block4k)`, `selectActiveHeader(h1, h2)`, `parseVhdxRegionTable(block64k)`, `decodeVhdxBatEntry(raw)` |

---

## 三、 使用示例

```typescript
import {
    parseVhdxFileId,
    parseVhdxHeader,
    selectActiveHeader,
    parseVhdxRegionTable,
    decodeVhdxBatEntry,
} from "cap:vhdx";

fn inspectVhdx(file_id_buf: []const u8, hdr1_4k: []const u8, hdr2_4k: []const u8, reg_64k: []const u8) {
    if (fid := parseVhdxFileId(file_id_buf)) {
        let creator_ascii: [64]u8 = [0; 64];
        const n = fid.decodeCreatorAscii(creator_ascii[0..]);
        * = n;
    }

    const active = selectActiveHeader(parseVhdxHeader(hdr1_4k), parseVhdxHeader(hdr2_4k));
    if (hdr := active) {
        * = hdr.sequence_number;
    }

    if (rt := parseVhdxRegionTable(reg_64k)) {
        if (bat := rt.findBat()) {
            * = bat.file_offset;
        }
    }

    const entry = decodeVhdxBatEntry((16 as u64 << 20) | 6);
    * = entry.fileOffsetBytes();
}
```
