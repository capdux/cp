# `cap:apfs` — Apple File System (APFS) 超级块与 Fletcher-64 零分配解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:sys`，导入请使用 `import { ... } from "cap:sys"`。

> **模块路径**: `lib/apfs/`  
> **导入协议**: `import { apfsFletcher64, verifyApfsChecksum, parseApfsObjHeader, parseApfsContainer, parseApfsVolume } from "cap:sys";`  
> **动态挂载**: `import cap from "cap";` -> `cap.apfs.*` 编译期按需惰性挂载

---

## 一、 架构概述

`cap:apfs` 是纯 CP 实现、**零堆分配（zero-alloc）**、零 C/POSIX 外部依赖的 Apple File System (APFS) 对象头、Fletcher-64 校验和、Container Superblock (`"NXSB"`) 与 Volume Superblock (`"APSB"`) 解析库：

1. **Apple APFS Fletcher-64 校验和**：
   - 按 32-bit 小端字对 `[8..block.len]` 模 `0xFFFFFFFF` 累加 `sum1` / `sum2`，计算 `(check2 << 32) | check1`，并通过 `verifyApfsChecksum` 与偏移 0 处 `o_cksum` 严格比对。
2. **32B Object Header (`ApfsObjHeader`)**：
   - 提取 `cksum`、`oid`、`xid`、低 16 位 `obj_type`、高 16 位存储标志 `obj_flags`（支持 `isPhysical()` / `isEphemeral()` / `isVirtual()`）与 `subtype`。
3. **Container Superblock (`"NXSB"`)**：
   - 严格校验 `"NXSB"` 魔数、`OBJ_TYPE_NX_SUPERBLOCK`、`block_size >= 4096` 且为 2 的幂、`block.len >= block_size` 及 Fletcher-64 校验和，提供 `totalBytes(): u128`。
4. **Volume Superblock (`"APSB"`)**：
   - 严格校验 `"APSB"` 魔数、`OBJ_TYPE_FS` 与 Fletcher-64 校验和，提取 OMap/Root/ExtentRef/SnapMeta OID、文件/目录/软链接计数、卷 UUID 与 256 字节卷名 `volnameSlice()`，提供 `isUnencrypted()` / `isCaseInsensitive()` 查询。

---

## 二、 核心 API 速查表 (`lib/apfs/apfs.cp`)

| 类别 | 符号 / 方法 | 签名与说明 |
| :--- | :--- | :--- |
| 协议常量 | 魔数与尺寸 | `APFS_OBJ_HEADER_SIZE (32)`, `APFS_NX_MAGIC (0x4253584E)`, `APFS_VOL_MAGIC (0x42535041)`, `APFS_MIN_BLOCK_SIZE (4096)`, `APFS_MAX_FILE_SYSTEMS (100)` |
| 协议常量 | 对象类型与存储标志 | `OBJ_TYPE_NX_SUPERBLOCK`, `OBJ_TYPE_BTREE`, `OBJ_TYPE_BTREE_NODE`, `OBJ_TYPE_SPACEMAN`, `OBJ_TYPE_OMAP`, `OBJ_TYPE_CHECKPOINT_MAP`, `OBJ_TYPE_FS`, `OBJ_VIRTUAL (0x0000)`, `OBJ_PHYSICAL (0x4000)`, `OBJ_EPHEMERAL (0x8000)` |
| 协议常量 | 卷特性标志 | `APFS_FS_UNENCRYPTED (0x01)`, `APFS_FS_ONEKEY (0x08)`, `APFS_FS_CASE_INSENSITIVE (0x01)`, `APFS_FS_CASE_SENSITIVE (0x08)` |
| 校验和 | Fletcher-64 | `apfsFletcher64(block: []const u8): u64`, `verifyApfsChecksum(block: []const u8): bool` |
| 数据模型 | `ApfsObjHeader` | `cksum`, `oid`, `xid`, `obj_type`, `obj_flags`, `subtype`, `isPhysical()`, `isEphemeral()`, `isVirtual()` |
| 数据模型 | `ApfsContainerSuper` | `header`, `block_size`, `block_count`, `features`, `ro_compat_features`, `incompat_features`, `uuid`, `next_oid`, `next_xid`, `xp_desc_blocks`, `xp_data_blocks`, `omap_oid`, `max_file_systems`, `totalBytes(): u128` |
| 数据模型 | `ApfsVolumeSuper` | `header`, `fs_index`, `features`, `ro_compat_features`, `incompat_features`, `omap_oid`, `root_tree_oid`, `extentref_tree_oid`, `snap_meta_tree_oid`, `num_files`, `num_directories`, `num_symlinks`, `vol_uuid`, `volname`, `volnameSlice()`, `isUnencrypted()`, `isCaseInsensitive()` |
| 解析函数 | 顶层解析入口 | `parseApfsObjHeader(data)`, `isApfsContainer(block)`, `parseApfsContainer(block)`, `isApfsVolume(block)`, `parseApfsVolume(block)` |

---

## 三、 使用示例

```typescript
import { parseApfsContainer, parseApfsVolume } from "cap:sys";

fn inspectApfs(nx_block: []const u8, vol_block: []const u8) {
    if (nx := parseApfsContainer(nx_block)) {
        * = nx.totalBytes();
    }
    if (vol := parseApfsVolume(vol_block)) {
        const name = vol.volnameSlice();
        * = name;
    }
}
```
