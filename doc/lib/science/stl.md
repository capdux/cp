# `cap:stl` — 纯 CP 3D STL 二进制与 ASCII 三角网格零分配解析与编码库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:science`，导入请使用 `import { ... } from "cap:science"`。

## 概述

`cap:stl` 提供纯 CP、零外部依赖、零堆分配的 3D STL（Stereolithography）二进制与 ASCII 网格格式探测、84 字节二进制头解码、50 字节小端 IEEE 754 `f32` 三角面片流式迭代、零分配二进制 STL 编码器、AABB 轴对齐包围盒统计及 ASCII `facet normal` 计数器。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `StlFormat` | 格式枚举：`binary`, `ascii`, `unknown` |
| `StlVec3` | 3D 单精度浮点向量：`x: f32`, `y: f32`, `z: f32` |
| `StlTriangle` | 三角面片：`normal`, `v1`, `v2`, `v3`, `attr_byte_count: u16` |
| `StlBinaryHeader` | 二进制头：`header_bytes: [80]u8`, `triangle_count: u32` |
| `StlBounds` | 轴对齐包围盒：`min: StlVec3`, `max: StlVec3`, `triangle_count: usize` |
| `detectStlFormat(data)` | 优先按 `84 + 50 * N` 精确长度判二进制，避免以 `"solid "` 开头的二进制头被误判 |
| `parseStlBinaryHeader(data)` | 校验 `>= 84` 字节与防溢出面片边界并解码头部 |
| `StlBinaryIter` | 流式三角面片迭代器：`init(data)`、`next(*_): StlTriangle?` |
| `writeStlBinary(triangles, header_text, out)` | 零分配编码完整二进制 STL，缓冲不足返回 `null` |
| `computeStlBinaryBounds(data)` | 零分配计算二进制 STL 轴对齐包围盒 |
| `countAsciiStlFacets(data)` | 零分配统计 ASCII STL `facet normal` 数量 |

## 示例

```cp
import { detectStlFormat, computeStlBinaryBounds, StlFormat } from "cap:science";

fn checkMesh(stl_bytes: []const u8): bool {
    if (detectStlFormat(stl_bytes) != StlFormat.binary) return false;
    return computeStlBinaryBounds(stl_bytes) != null;
}
```
