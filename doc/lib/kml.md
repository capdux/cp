# `cap:kml` — 纯 CP OGC KML 2.2 地理标记零分配解析库

## 概述

`cap:kml` 提供纯 CP、零外部依赖、零堆分配的 OGC KML 2.2 解析器，支持：
- **根标签探测**：`isKml` 自动跳过 UTF-8 BOM、XML 声明与注释，校验 `<kml` 根元素；
- **流式 `<Placemark>` 迭代**：`KmlPlacemarkIter` 支持嵌套 `<Document>`/`<Folder>`、提取 `id`/`name`/`description`/`styleUrl`（自动剥离 `<![CDATA[...]]>`）并识别几何类型；
- **规范 `lon,lat[,alt]` 坐标解析与全局统计**：`KmlCoordIter` 严格校验 `-180 <= lon <= 180` 与 `-90 <= lat <= 90`，`summarizeKml` 单遍统计地标分布与经纬度 BBox。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `KmlGeomKind` | 几何类型枚举：`point`, `linestring`, `polygon`, `multigeometry`, `unknown` |
| `isKml(data)` | 探测合法 KML 文档 |
| `KmlCoord` / `KmlCoordIter` | 2D/3D 坐标解析（`lon,lat[,alt]`）与经纬度合法性校验 |
| `KmlPlacemark` / `KmlPlacemarkIter` | 零拷贝地标视图与流式迭代器（含 `coords(): KmlCoordIter`） |
| `KmlStats` / `summarizeKml(data)` | 单遍汇总地标计数、坐标总数与经纬度包络盒 |

## 示例

```cp
import { isKml, summarizeKml } from "cap:kml";

fn inspectKml(xml: []const u8): u32 {
    if (!isKml(xml)) return 0;
    const stats = summarizeKml(xml);
    return stats.placemark_count;
}
```
