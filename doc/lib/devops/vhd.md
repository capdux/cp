# `cap:vhd` — Microsoft VHD 1.0 虚拟硬盘零分配解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:devops`，导入请使用 `import { ... } from "cap:devops"`。

> **模块路径**: `lib/vhd/`  
> **导入协议**: `import { isVhdFooter, parseVhdFooter, parseVhdDynamicHeader, lookupVhdBatEntry, vhdChecksum } from "cap:devops";`  
> **动态挂载**: `import cap from "cap";` -> `cap.vhd.*` 编译期按需惰性挂载

---

## 一、 架构概述

`cap:vhd` 是纯 CP 实现、**零堆分配（zero-alloc）**、零 C/POSIX 外部依赖的 Microsoft VHD 1.0（Fixed / Dynamic / Differencing）虚拟硬盘大端元数据结构解析库：

1. **一补码字节和校验 (`vhdChecksum`)**：
   - 遵循微软 VHD 1.0 规范，对 Footer（512B，跳过偏移 64）与 Dynamic Disk Header（1024B，跳过偏移 36）执行零拷贝逐字节求和并按位取反校验。
2. **512B Hard Disk Footer 解析**：
   - 严格校验 `"conectix"` Cookie、`format_version == 0x00010000`、`disk_type in 2..4` 与大端一补码校验和；
   - 提供 `isFixed()` / `isDynamic()` / `isDifferencing()` 及 2000-01-01 纪元至 Unix 时间戳转换 `unixTimestamp()`。
3. **1024B Dynamic Disk Header 与 BAT 查表**：
   - 严格校验 `"cxsparse"` Cookie、`header_version == 0x00010000`、`block_size >= 512` 且为 2 的幂与偏移 36 校验和；
   - 提供 `blockSectorCount()`、`batSizeBytes()` 以及大端 32-bit BAT 扇区查表 `lookupVhdBatEntry`（遇 `0xFFFFFFFF` 未分配块或越界返回 `null`）。

---

## 二、 核心 API 速查表 (`lib/vhd/vhd.cp`)

| 类别 | 符号 / 方法 | 签名与说明 |
| :--- | :--- | :--- |
| 协议常量 | Cookie 与尺寸 | `VHD_FOOTER_COOKIE ("conectix")`, `VHD_DYNAMIC_COOKIE ("cxsparse")`, `VHD_FOOTER_SIZE (512)`, `VHD_DYNAMIC_HEADER_SIZE (1024)`, `VHD_FORMAT_VERSION (0x00010000)`, `VHD_FIXED_DATA_OFFSET (0xFFFFFFFFFFFFFFFF)`, `VHD_EPOCH_UNIX_DELTA (946684800)`, `VHD_BAT_UNUSED (0xFFFFFFFF)` |
| 协议常量 | 类型与特性 | `DISK_TYPE_FIXED (2)`, `DISK_TYPE_DYNAMIC (3)`, `DISK_TYPE_DIFFERENCING (4)`, `FEATURE_TEMPORARY (0x01)`, `FEATURE_RESERVED (0x02)` |
| 校验和 | `vhdChecksum` | `vhdChecksum(data: []const u8, checksum_offset: usize): u32` |
| 数据模型 | `VhdGeometry` | `cylinders: u16`, `heads: u8`, `sectors_per_track: u8` |
| 数据模型 | `VhdFooter` | `features`, `format_version`, `data_offset`, `timestamp`, `creator_app`, `creator_version`, `creator_host_os`, `original_size`, `current_size`, `geometry`, `disk_type`, `checksum`, `unique_id`, `saved_state`, `isFixed()`, `isDynamic()`, `isDifferencing()`, `unixTimestamp()` |
| 数据模型 | `VhdDynamicHeader` | `data_offset`, `table_offset`, `header_version`, `max_table_entries`, `block_size`, `checksum`, `parent_unique_id`, `parent_timestamp`, `parent_unicode_name`, `blockSectorCount()`, `batSizeBytes()` |
| 解析与查表 | 顶层函数 | `isVhdFooter(data)`, `parseVhdFooter(footer512)`, `parseVhdDynamicHeader(hdr1024)`, `lookupVhdBatEntry(bat_bytes, block_idx)` |

---

## 三、 使用示例

```typescript
import { parseVhdFooter, parseVhdDynamicHeader, lookupVhdBatEntry } from "cap:devops";

fn inspectVhd(footer512: []const u8, dyn1024: []const u8, bat: []const u8) {
    if (footer := parseVhdFooter(footer512)) {
        * = footer.unixTimestamp();
        if (footer.isDynamic()) {
            if (dyn_hdr := parseVhdDynamicHeader(dyn1024)) {
                * = dyn_hdr.blockSectorCount();
                if (sector := lookupVhdBatEntry(bat, 0)) {
                    * = sector;
                }
            }
        }
    }
}
```
