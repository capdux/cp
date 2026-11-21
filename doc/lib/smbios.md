# `cap:smbios` — DMTF SMBIOS 2.1 / 3.0+ Entry Point 与硬件结构表零分配解析库

纯 CP、零堆分配（zero-alloc）的 DMTF SMBIOS 2.1（32-bit）与 3.0+（64-bit）Entry Point 校验及硬件结构表流式解析库，内置 Type 0 BIOS、Type 1 System、Type 17 Memory Device 强类型视图。

## 导入

```cp
import {
    SMBIOS2_ANCHOR,
    SMBIOS2_DMI_ANCHOR,
    SMBIOS3_ANCHOR,
    SMBIOS2_EP_SIZE,
    SMBIOS3_EP_SIZE,
    SMBIOS_STRUCT_HEADER_SIZE,
    TYPE_BIOS,
    TYPE_SYSTEM,
    TYPE_BASEBOARD,
    TYPE_CHASSIS,
    TYPE_PROCESSOR,
    TYPE_MEMORY_DEVICE,
    TYPE_END_OF_TABLE,
    smbiosChecksum,
    Smbios2EntryPoint,
    parseSmbios2Entry,
    Smbios3EntryPoint,
    parseSmbios3Entry,
    SmbiosStruct,
    SmbiosTableIter,
    SmbiosBiosInfo,
    parseBiosInfo,
    SmbiosSystemInfo,
    parseSystemInfo,
    SmbiosMemoryDevice,
    parseMemoryDevice,
} from "cap:smbios";
```

## 核心 API

- **Entry Point 解析与校验**：`smbiosChecksum(data): u8`、`parseSmbios2Entry(data): Smbios2EntryPoint?`（双锚点 `"_SM_"` + `"_DMI_"` 与双重校验和）、`parseSmbios3Entry(data): Smbios3EntryPoint?`（`"_SM3_"` 与 64-bit 表地址）。
- **结构表零拷贝迭代**：`SmbiosTableIter`（`init`, `next`, `isDone`, `hasError`）逐条产出 `SmbiosStruct`（`stringAt(1-based)`、`isEndOfTable()`），遇 Type 127 结束，缺 `\0\0` 终止符或 `length < 4` 报错误。
- **强类型硬件视图**：`parseBiosInfo`（Type 0）、`parseSystemInfo`（Type 1，含 16B UUID）、`parseMemoryDevice`（Type 17，兼容 KB/MB 位与 `0x7FFF` 32-bit 扩展容量）。

## 示例

```cp
import { parseSmbios3Entry, SmbiosTableIter, parseBiosInfo } from "cap:smbios";

fn example(ep: []const u8, table: []const u8) {
    * = parseSmbios3Entry(ep);
    let it = SmbiosTableIter.init(table);
    while (true) {
        if (s := it.next()) {
            if (bios := parseBiosInfo(s)) {
                * = bios.vendor;
            }
        } else {
            break;
        }
    }
}
```
