# `cap:gltf` — 纯 CP glTF 2.0 Binary (GLB) 零分配容器与 Chunk 解析库

## 概述

`cap:gltf` 提供纯 CP、零外部依赖、零堆分配的 Khronos glTF 2.0 Binary (`.glb`) 容器解析器，支持 12 字节 GLB 头部校验、4 字节对齐与严格边界防护的 Chunk 流式迭代（支持 `JSON`/`BIN` 并安全跳过未知扩展 Chunk）、`parseGlb` / `extractJsonChunk` / `extractBinChunk` 快捷提取以及 `ComponentType` / `AccessorType` 字节步长查询。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `GLB_MAGIC` / `GLB_VERSION_2` | GLB 魔数 `0x46546C67` (`"glTF"`) 与版本 `2` |
| `CHUNK_TYPE_JSON` / `CHUNK_TYPE_BIN` | 标准 Chunk 类型 `0x4E4F534A` / `0x004E4942` |
| `COMPONENT_*` | glTF 2.0 ComponentType 常量（`5120`..`5126`） |
| `ChunkType` / `AccessorType` | Chunk 分类枚举与 Accessor 元素类型枚举（`scalar`..`mat4`） |
| `GlbHeader` / `GlbChunk` / `GlbDocument` | 零拷贝头部、Chunk 与完整文档视图 |
| `isGlb(data)` / `parseGlbHeader(data)` | 校验 12 字节头部魔数、版本、4 字节对齐与总长度边界 |
| `GlbChunkIter` | 流式 Chunk 迭代器：`init(data)`、`next(*_): GlbChunk?` |
| `extractJsonChunk(data)` / `extractBinChunk(data)` / `parseGlb(data)` | 快捷提取 JSON/BIN 负载切片与完整文档校验 |
| `componentByteSize(ct)` / `accessorComponentCount(at)` / `parseAccessorType(s)` / `accessorElementStride(ct, at)` | Accessor 分量宽度与紧凑字节步长查询 |

## 示例

```cp
import { parseGlb, accessorElementStride, COMPONENT_FLOAT, AccessorType } from "cap:gltf";

fn inspectGlb(glb_bytes: []const u8): bool {
    if (doc := parseGlb(glb_bytes)) {
        const stride = accessorElementStride(COMPONENT_FLOAT, AccessorType.vec3) ?? 0;
        return doc.json.len > 0 && stride == 12;
    }
    return false;
}
```
