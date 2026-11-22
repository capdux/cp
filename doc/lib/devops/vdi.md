# `cap:vdi` — VirtualBox VDI 1.1 虚拟磁盘镜像零分配解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:devops`，导入请使用 `import { ... } from "cap:devops"`。

> **模块路径**: `lib/vdi/`  
> **导入协议**: `import { VDI_SIGNATURE, isVdi, parseVdiHeader, decodeVdiBlockEntry, lookupVdiBlock } from "cap:devops";`  
> **动态挂载**: `import cap from "cap";` -> `cap.vdi.*` 编译期按需惰性挂载

---

## 一、 架构概述

`cap:vdi` 是纯 CP 实现、**零堆分配（zero-alloc）**、零 C/POSIX 外部依赖的 VirtualBox VDI 1.1 虚拟磁盘镜像报头与 Block Allocation Map（BAM）解析库：

1. **VDI 1.1 报头严格解析**：
   - 校验偏移 64 的 32 位小端魔数 `0xBEDA107F`、`version == 1.1`、`header_size >= 400`、`image_type in 1..4`、`sector_size > 0`、`block_size > 0`。
   - 完整提取镜像几何参数、块表偏移、数据区偏移、256 字节 NUL 终止注释以及 4 组 16 字节 UUID（`uuid_create` / `uuid_modify` / `uuid_linkage` / `uuid_parent_modify`）。
2. **语义查询与差分盘检测**：
   - 内置 `isDynamic()` / `isStatic()` / `isDiff()`、`hasParent()`（检测 `uuid_linkage` 非全零）、`commentSlice()` 与 `blockMapBytes()`。
3. **Block Allocation Map 零拷贝查表**：
   - `decodeVdiBlockEntry` 与 `lookupVdiBlock` 支持特殊标记 `VDI_BLOCK_FREE (0xFFFFFFFF)`、`VDI_BLOCK_ZERO (0xFFFFFFFE)` 及已分配物理块文件偏移计算 `offset_data + phys_block * (block_size + block_extra_data)`，并严格校验逻辑块号边界与缓冲截断。

---

## 二、 核心 API 速查表 (`lib/vdi/vdi.cp`)

| 类别 | 符号 / 方法 | 签名与说明 |
| :--- | :--- | :--- |
| 协议常量 | 魔数与尺寸 | `VDI_SIGNATURE (0xBEDA107F)`, `VDI_MIN_FILE_HEADER_SIZE (512)`, `VDI_V1_1_MIN_HEADER_SIZE (400)`, `VDI_DEFAULT_SECTOR_SIZE (512)`, `VDI_DEFAULT_BLOCK_SIZE (1048576)` |
| 协议常量 | 镜像类型与块标记 | `TYPE_DYNAMIC (1)`, `TYPE_STATIC (2)`, `TYPE_UNDO (3)`, `TYPE_DIFF (4)`, `VDI_BLOCK_FREE (0xFFFFFFFF)`, `VDI_BLOCK_ZERO (0xFFFFFFFE)` |
| 报头模型 | `VdiHeader` | `version_minor`, `version_major`, `header_size`, `image_type`, `image_flags`, `offset_blocks`, `offset_data`, `sector_size`, `disk_size`, `block_size`, `block_extra_data`, `blocks_in_hdd`, `blocks_allocated`, `uuid_create`, `uuid_modify`, `uuid_linkage`, `uuid_parent_modify`, `comment`, `isDynamic()`, `isStatic()`, `isDiff()`, `hasParent()`, `commentSlice()`, `blockMapBytes()` |
| 块映射 | `VdiBlockMapping` | `is_allocated: bool`, `is_zero: bool`, `phys_block: u32`, `file_offset: u64` |
| 解析与查表 | 顶层函数 | `isVdi(data)`, `parseVdiHeader(data)`, `decodeVdiBlockEntry(header, raw_entry)`, `lookupVdiBlock(header, block_map, logical_block)` |

---

## 三、 使用示例

```typescript
import { parseVdiHeader, lookupVdiBlock } from "cap:devops";

fn inspectVdi(header_512: []const u8, block_map: []const u8) {
    if (hdr := parseVdiHeader(header_512)) {
        const comment = hdr.commentSlice();
        * = comment;
        if (mapping := lookupVdiBlock(hdr, block_map, 0)) {
            if (mapping.is_allocated) {
                * = mapping.file_offset;
            }
        }
    }
}
```
