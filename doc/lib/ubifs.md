# `cap:ubifs` — Linux UBI & UBIFS 闪存文件系统零分配解析库

> **模块路径**: `lib/ubifs/`  
> **导入协议**: `import { ubifsCrc32, parseUbiEcHeader, parseUbiVidHeader, parseUbifsNode, parseUbifsSuperblock, parseUbifsMaster } from "cap:ubifs";`  
> **动态挂载**: `import cap from "cap";` -> `cap.ubifs.*` 编译期按需惰性挂载

---

## 一、 架构概述

`cap:ubifs` 是纯 CP 实现、**零堆分配（zero-alloc）**、零 C/POSIX 外部依赖的 Linux UBI 物理块头与 UBIFS 闪存文件系统节点解析库：

1. **UBI / UBIFS 无取反 CRC32 (`ubifsCrc32`)**：
   - 使用 IEEE 802.3 反射多项式 `0xEDB88320`、初值 `0xFFFFFFFF`、无最终取反，与 Linux 内核 `ubi_crc32` / `ubifs_crc32` 逐位一致。
2. **UBI 64B 大端 EC & VID 报头解析**：
   - `parseUbiEcHeader` 校验 `"UBI#"`、`version == 1` 与前 60B CRC32；
   - `parseUbiVidHeader` 校验 `"UBI!"`、`version == 1` 与前 60B CRC32。
3. **UBIFS 24B 小端通用节点头及 Superblock / Master 节点解析**：
   - `parseUbifsNode` 校验 `0x06101831`、`24 <= len <= data.len` 及 `[8..len]` 的 CRC32；
   - `parseUbifsSuperblock`（type 6）与 `parseUbifsMaster`（type 7）提供完整卷参数、UUID、容量 `totalBytes()` 与空间统计视图。

---

## 二、 核心 API 速查表 (`lib/ubifs/ubifs.cp`)

| 类别 | 符号 / 方法 | 签名与说明 |
| :--- | :--- | :--- |
| 协议常量 | UBI / UBIFS 魔数与尺寸 | `UBI_EC_HDR_MAGIC ("UBI#")`, `UBI_VID_HDR_MAGIC ("UBI!")`, `UBI_HDR_SIZE (64)`, `UBIFS_NODE_MAGIC (0x06101831)`, `UBIFS_CH_SIZE (24)`, `UBIFS_CRC32_INIT (0xFFFFFFFF)` |
| 协议常量 | 节点与压缩类型 | `UBIFS_INO_NODE (0)` .. `UBIFS_ORPH_NODE (11)`, `COMPR_NONE (0)`, `COMPR_LZO (1)`, `COMPR_ZLIB (2)`, `COMPR_ZSTD (3)` |
| 校验和 | `ubifsCrc32` | `ubifsCrc32(data: []const u8): u32` |
| UBI 模型 | `UbiEcHeader` & `UbiVidHeader` | `parseUbiEcHeader(data)`, `parseUbiVidHeader(data)` |
| UBIFS 模型 | `UbifsCommonHeader` / `UbifsSuperblock` / `UbifsMasterNode` | `isUbifsNode(data)`, `parseUbifsNode(data)`, `parseUbifsSuperblock(data)` (`totalBytes()`), `parseUbifsMaster(data)` |

---

## 三、 使用示例

```typescript
import { parseUbiEcHeader, parseUbifsSuperblock, parseUbifsMaster } from "cap:ubifs";

fn inspectFlash(ec_hdr: []const u8, sb_node: []const u8, mst_node: []const u8) {
    if (ec := parseUbiEcHeader(ec_hdr)) {
        * = ec.ec;
    }
    if (sb := parseUbifsSuperblock(sb_node)) {
        * = sb.totalBytes();
    }
    if (mst := parseUbifsMaster(mst_node)) {
        * = mst.total_free;
    }
}
```
