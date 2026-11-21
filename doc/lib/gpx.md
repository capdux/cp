# `cap:gpx` — 纯 CP GPX 1.0/1.1 航点/轨迹 XML 零分配解析与距离统计库

## 概述

`cap:gpx` 提供纯 CP、零外部依赖、零堆分配的 GPS Exchange Format（GPX 1.0/1.1）XML 解析与地理轨迹统计能力，支持 UTF-8 BOM/XML 声明跳过、`<metadata>` 提取、`<wpt>`/`<trkpt>` 流式迭代（含单/双引号属性与自闭合标签）、WGS-84 Haversine 球面距离计算，以及跨 `<trkseg>` 隔离的累计距离、爬升/下降与边界框汇总。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `GPX_EARTH_RADIUS_M` | WGS-84 平均地球半径常量（`6371008.8` m） |
| `isGpx(data)` | 跳过可选 BOM/声明/注释后校验 `<gpx` 根标签 |
| `GpxHeader` / `parseGpxHeader(data)` | 提取 `version`, `creator` 及可选 `<metadata>` 的 `name`, `desc`, `time` |
| `GpxPoint` | 航点/轨迹点：`lat`, `lon`, `ele`, `has_ele`, `time`, `name`, `sym`（严检 `-90..90` 与 `-180..180`） |
| `GpxWaypointIter` | 零分配 `<wpt>` 迭代器：`init(data)`, `next(*_): GpxPoint?` |
| `GpxTrackPoint` / `GpxTrackPointIter` | 零分配 `<trkpt>` 迭代器，同步追踪 `track_index` 与 `segment_index` |
| `haversineMeters(lat1, lon1, lat2, lon2): f64` | WGS-84 球面大圆距离（米） |
| `GpxStats` / `summarizeGpx(data)` | 单遍汇总航点/轨迹/段/轨迹点数、段内累计距离、爬升/下降及经纬度边界框 |

## 示例

```cp
import { isGpx, summarizeGpx } from "cap:gpx";

fn trackDistanceKm(xml: []const u8): f64 {
    if (!isGpx(xml)) return 0.0;
    const stats = summarizeGpx(xml);
    return stats.total_distance_m / 1000.0;
}
```
