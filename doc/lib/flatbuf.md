# `cap:flatbuf` — 纯 CP Google FlatBuffers 二进制零拷贝解析库

## 概述

`cap:flatbuf` 提供纯 CP、零外部依赖、零堆分配的 Google FlatBuffers 二进制格式解析器，完整支持：
- **File Identifier 与 Size Prefix**：`fbFileIdentifier`、`fbHasIdentifier` 与 `fbStripSizePrefix`；
- **安全 VTable 定位与标量读取**：`fbRootTable` + `FbTable` 支持正/负 `soffset_t` 定位 VTable、缺失字段安全返回 `null`，提供全套 `getBool`/`getU8`..`getF64`；
- **零拷贝字符串、嵌套表与向量**：`getString`、`getTable`、`getVector` 与 `FbVector`（支持 `byteSlice`、标量向量、字符串向量与嵌套表向量，全程严格边界校验）。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `FB_SIZE_PREFIX_BYTES`, `FB_FILE_ID_OFFSET`, `FB_FILE_ID_LEN` | 尺寸前缀与文件标识偏移常量 |
| `fbFileIdentifier`, `fbHasIdentifier`, `fbStripSizePrefix`, `fbRootTable` | 标识探测、去前缀与根表定位 |
| `FbTable` | 零拷贝表视图：`vtablePos()`、`fieldCount()`、`fieldOffset(slot)`、`getBool`..`getF64`、`getString`、`getTable`、`getVector` |
| `FbVector` | 零拷贝向量视图：`byteSlice()`、`getU8`/`getU16`/`getU32`/`getI32`/`getU64`/`getF32`/`getString`/`getTable` |

## 示例

```cp
import { fbHasIdentifier, fbRootTable } from "cap:flatbuf";

fn readMonsterHp(buf: []const u8): i16 {
    if (!fbHasIdentifier(buf, "MONS")) return 0;
    if (root := fbRootTable(buf)) {
        return root.getI16(4) ?? 100;
    }
    return 0;
}
```
