# `cap:ply` — 纯 CP 零分配 Stanford PLY 3D 点云与网格头解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:science`，导入请使用 `import { ... } from "cap:science"`。

## 概述

`cap:ply` 提供纯 CP、零外部依赖、零堆分配的 Stanford Polygon File Format (`.ply`) 3D 网格与点云文件头解析器、元素与属性计数迭代器以及标量类型字节宽度查询，与 `cap:stl`、`cap:obj` 组成完整的 3D 网格资产解析能力。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `PlyFormat` | 数据编码格式枚举：`ascii`, `binary_little_endian`, `binary_big_endian`, `unknown` |
| `PlyHeader` | 文件头摘要：`format`, `version: []const u8`, `vertex_count: u64`, `face_count: u64`, `element_count: u32`, `data_offset: usize` |
| `PlyElement` | 元素定义：`name: []const u8`, `count: u64`, `property_count: u32` |
| `isPly(data)` | 校验首行是否为 `"ply\n"` 或 `"ply\r\n"` |
| `parsePlyHeader(data)` | 解析 `format`、`element`、`property`、`comment` 直至 `end_header`，拒绝缺失 `format` 或未闭合 `end_header` |
| `PlyElementIter` | 元素流式迭代器：`init(data)`, `next(*_): PlyElement?`（逐个产出 `element <name> <count>` 并统计其下属 `property` 数量） |
| `plyScalarSize(type_name)` | 返回 PLY 标准标量类型字节宽度：`1` / `2` / `4` / `8`，未知返回 `null` |

## 示例

```cp
import { isPly, parsePlyHeader, PlyElementIter } from "cap:science";

fn inspectPly(data: []const u8): u64? {
    if (!isPly(data)) return null;
    const hdr = parsePlyHeader(data) ?? return null;
    let it = PlyElementIter.init(data);
    while (elem := it.next()) {
        if (elem.property_count == 0) return null;
    }
    return hdr.vertex_count;
}
```
