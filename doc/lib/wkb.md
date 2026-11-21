# `cap:wkb` — 纯 CP OGC WKB / PostGIS EWKB 空间几何二进制零分配编解码库

## 概述

`cap:wkb` 提供纯 CP、零外部依赖、零堆分配的 OGC Simple Features WKB、ISO WKB（+1000/+2000/+3000 维度偏移）与 PostGIS EWKB（`Z`/`M`/`SRID` 标志位）空间几何二进制编解码器，支持 XDR（大端 `0x00`）与 NDR（小端 `0x01`）双端序，内置 `MAX_WKB_DEPTH = 32` 递归深度限制与零拷贝迭代器。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `WKB_XDR` / `WKB_NDR` / `EWKB_FLAG_Z` / `EWKB_FLAG_M` / `EWKB_FLAG_SRID` | 字节序与 PostGIS EWKB 标志位常量 |
| `WkbGeomType` | 几何类型枚举：`point=1`, `linestring=2`, `polygon=3`, `multipoint=4`, `multilinestring=5`, `multipolygon=6`, `geometrycollection=7`, `unknown=0` |
| `WkbPoint` | 2D/3D/4D 坐标结构：`x`, `y`, `z`, `m`, `has_z`, `has_m`；附工厂 `xy`/`xyz`/`xym`/`xyzm` |
| `WkbHeader` / `parseWkbHeader(data)` | 解析字节序、ISO/EWKB 类型与维度、可选 `srid: u32?`、`coord_dims` 及 `body_offset` |
| `wkbByteLength(data): usize?` | 严格计算完整 WKB 几何字节长度（防越界与乘法溢出） |
| `WkbPointIter` / `WkbRingIter` / `WkbChildIter` | LineString/Ring 坐标点、Polygon 环、Multi*/GeometryCollection 子几何零分配迭代器 |
| `WkbReader` | 快捷解析入口：`readPoint`, `linePoints`, `polygonRings`, `children` |
| `WkbWriter` | 零分配写入器：`init`/`initNdr`/`initXdr`、`writePoint`/`writeLineString`/`writePolygonHeader`/`writeRing`/`writeCollectionHeader` |

## 示例

```cp
import { WkbPoint, WkbWriter, WkbReader, parseWkbHeader } from "cap:wkb";

fn encodePoint(buf: []u8): f64 {
    let w = WkbWriter.initNdr(buf);
    if (!w.writePoint(WkbPoint.xyz(121.47, 31.23, 15.0), 4326)) return 0.0;
    const pt = WkbReader.readPoint(w.written()) ?? return 0.0;
    return pt.x;
}
```
