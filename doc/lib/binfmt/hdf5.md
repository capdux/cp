# `cap:hdf5` — 零分配 HDF5 容器 Superblock & OHDR 解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:binfmt`，导入请使用 `import { ... } from "cap:binfmt"`。

> **模块路径**：`lib/hdf5/`（导入方式：`import { ... } from "cap:binfmt";`）  
> **特性**：100% 纯 CP、零堆分配、完整支持 Superblock v0/v1/v2/v3、User Block 自动跳搜与 v2 对象头（`OHDR`）零拷贝解析。

---

## 一、 核心常量

| 常量名 | 值 | 说明 |
| :--- | :--- | :--- |
| `HDF5_SIGNATURE` | `"\x89HDF\r\n\x1a\n"` | 8 字节 HDF5 文件签名 |
| `HDF5_UNDEF_ADDR_64` | `0xFFFFFFFFFFFFFFFF` | 64 位未定义地址哨兵值 |
| `HDF5_OHDR_MAGIC` | `"OHDR"` | Version 2 对象头魔数 |
| `HDF5_OHDR_FLAG_PHASE_CHANGE` | `0x10` | `OHDR` 含属性存储阶段切换阈值（4B） |
| `HDF5_OHDR_FLAG_TIMESTAMPS` | `0x20` | `OHDR` 含 4 个 32 位时间戳（16B） |

---

## 二、 核心数据结构

### `Hdf5Superblock`
统一承载 Superblock v0 / v1 / v2 / v3 解析结果：
- `signature_offset: usize` — 签名所在偏移（即前置 User Block 字节大小）
- `version: u8` — Superblock 版本（0/1/2/3）
- `offset_size: u8` / `length_size: u8` — 地址与长度字节宽度（严格校验仅允许 2/4/8）
- `group_leaf_node_k: u16` / `group_internal_node_k: u16`（v0/v1）
- `consistency_flags: u32`
- `base_address: u64`、`free_space_address: u64`（v0/v1）、`superblock_ext_address: u64`（v2/v3）、`eof_address: u64`、`driver_info_address: u64`（v0/v1）、`root_group_addr: u64`
- `checksum: u32`（v2/v3）

### `Hdf5ObjHeaderV2`
- `version: u8`（`1`）、`flags: u8`
- `has_timestamps: bool`、`access_time`、`mod_time`、`change_time`、`birth_time`
- `chunk0_size: u64`、`chunk0: []const u8`（零拷贝首块消息负载切片）

---

## 三、 核心函数

- `findHdf5Signature(data: []const u8): usize?` — 依次在 `0, 512, 1024, 2048, ...` 搜索 8B 签名
- `isHdf5(data: []const u8): bool` — 判断是否为合法 HDF5 数据
- `parseHdf5Superblock(data: []const u8): Hdf5Superblock?` — 解析 Superblock v0/v1/v2/v3
- `parseHdf5ObjHeaderV2(data: []const u8, offset: usize): Hdf5ObjHeaderV2?` — 解析指定偏移处的 `OHDR` v2 对象头

---

## 四、 使用示例

```typescript
import {
    isHdf5,
    parseHdf5Superblock,
    parseHdf5ObjHeaderV2,
} from "cap:binfmt";

fn inspectHdf5(bytes: []const u8) {
    if (!isHdf5(bytes)) return;
    if (sb := parseHdf5Superblock(bytes)) {
        if (ohdr := parseHdf5ObjHeaderV2(bytes, sb.root_group_addr as usize)) {
            * = ohdr.chunk0.len;
        }
    }
}
```
