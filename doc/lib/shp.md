# `cap:shp` — 纯 CP ESRI Shapefile (`.shp`) 零分配解析库

## 概述

`cap:shp` 提供纯 CP、零外部依赖、零堆分配的 ESRI Shapefile `.shp` 矢量地理数据解析器，严格遵循混合端序规范（文件头前 28B 与 8B 记录头为 Big-Endian，版本/ShapeType/Bounding Box 与几何内容为 Little-Endian）：
- **100B 文件头解析**：校验 `file_code == 9994`、`version == 1000`、`file_length_words * 2 <= data.len`，解码 `ShpType` 与 64B `ShpBBox`；
- **流式记录迭代**：`ShpRecordIter` 从偏移 100 起零拷贝迭代每条记录；
- **几何快捷解码**：`parseShpPoint` 解码二维点，`parseShpPoly` 解码 PolyLine / Polygon 并提供 `partAt`、`pointAt` 零拷贝随机访问。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `SHP_FILE_CODE`, `SHP_VERSION`, `SHP_HEADER_SIZE`, `SHP_RECORD_HEADER_SIZE` | `9994`、`1000`、`100`、`8` 规范常量 |
| `ShpType`, `ShpBBox`, `ShpHeader` | 几何类型枚举、64B 包络盒与 100B 文件头 |
| `parseShpHeader(data)` / `isShp(data)` | 解析与探测合法 `.shp` 文件 |
| `ShpRecord`, `ShpRecordIter` | 零拷贝记录视图与流式迭代器 |
| `ShpPoint` / `parseShpPoint(content)` | 20B 二维点快捷解析 |
| `ShpPolyHeader` / `parseShpPoly(content)` | PolyLine / Polygon 头部与 `partAt` / `pointAt` 随机访问 |

## 示例

```cp
import { parseShpHeader, ShpRecordIter, parseShpPoint } from "cap:shp";

fn countPoints(shp_bytes: []const u8): usize {
    if (parseShpHeader(shp_bytes) == null) return 0;
    let it = ShpRecordIter.init(shp_bytes);
    let n: usize = 0;
    while (rec := it.next()) {
        if (pt := parseShpPoint(rec.content)) {
            * = pt.x;
            n += 1;
        }
    }
    return n;
}
```
