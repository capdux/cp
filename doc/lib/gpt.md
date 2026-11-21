# `cap:gpt` — UEFI GUID Partition Table & Protective MBR 零分配解析校验库

纯 CP、零堆分配（zero-alloc）的 UEFI GUID Partition Table（GPT）与 Protective MBR 磁盘分区表解析及 IEEE 802.3 CRC-32 双重校验库。

## 导入

```cp
import {
    GPT_SIGNATURE,
    GPT_REVISION_1_0,
    GPT_MIN_HEADER_SIZE,
    GPT_MIN_ENTRY_SIZE,
    MBR_SECTOR_SIZE,
    MBR_BOOT_SIGNATURE,
    MBR_TYPE_GPT_PROTECTIVE,
    GUID_UNUSED,
    GUID_EFI_SYSTEM,
    GUID_LINUX_FS,
    GUID_LINUX_SWAP,
    GUID_MS_BASIC_DATA,
    gptCrc32,
    formatGuid,
    MbrPartition,
    MbrPartitionTable,
    parseMbrPartitions,
    isProtectiveMbr,
    GptHeader,
    isGptHeader,
    parseGptHeader,
    verifyGptEntriesCrc,
    GptEntry,
    GptEntryIter,
} from "cap:gpt";
```

## 核心 API

- **Protective MBR**：`parseMbrPartitions(sector0): MbrPartitionTable?`、`isProtectiveMbr(sector0): bool`。
- **GPT Header & CRC32**：`gptCrc32(data): u32`（IEEE 802.3 `0xEDB88320`）、`isGptHeader(data)`、`parseGptHeader(data): GptHeader?`（校验 `"EFI PART"`、`header_size >= 92`、`part_entry_size >= 128` 与头部 CRC32）、`verifyGptEntriesCrc(header, entries_bytes): bool`。
- **分区条目迭代**：`GptEntryIter`（`init`, `initFromHeader`, `next`，支持跳过空分区）、`GptEntry`（`isUnused()`, `sectorCount()`, `isRequired()`, `isNoBlockIo()`, `decodeNameAscii(out)`）。
- **UEFI Mixed-Endian GUID**：`formatGuid(guid, out: []u8): usize?` 输出 36 字节标准 UUID 字符串。

## 示例

```cp
import std from "std";
import { parseGptHeader, verifyGptEntriesCrc, GptEntryIter, formatGuid } from "cap:gpt";

fn example(lba1: []const u8, entries: []const u8) {
    if (hdr := parseGptHeader(lba1)) {
        if (verifyGptEntriesCrc(hdr, entries)) {
            let it = GptEntryIter.initFromHeader(hdr, entries, true);
            while (true) {
                if (e := it.next()) {
                    let guid_str: [36]u8 = std.mem.zeroes([36]u8);
                    * = formatGuid(e.type_guid, guid_str[0..]);
                } else {
                    break;
                }
            }
        }
    }
}
```
