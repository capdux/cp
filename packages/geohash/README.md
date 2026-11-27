[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# geohash

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:geohash` ([https://github.com/georust/geohash](https://github.com/georust/geohash))

Zero-allocation Base32 Geohash encoder, bounding-box decoder, and 8-direction neighbor finder for CP — clean-room MIT parity with Rust `geohash` and npm `ngeohash`.

纯 CP 零堆分配 Geohash 经纬度网格编解码、包围盒计算与 8 方向相邻网格查找引擎，适用于 LBS 附近搜索、地理围栏与空间索引。

## Install / 安装

```bash
cap add geohash
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `GeoPoint` | `struct { lat: f64, lon: f64 }` | Decoded latitude/longitude center point |
| `GeoBBox` | `struct { min_lat, max_lat, min_lon, max_lon, valid }` | Exact Geohash cell bounding box |
| `encode` | `(lat: f64, lon: f64, precision: usize, dst: []u8) -> usize` | Encode coordinates to Base32 Geohash (`1..12` chars) |
| `decodeBBox` | `(hash: []const u8) -> GeoBBox` | Decode cell bounding box |
| `decode` | `(hash: []const u8) -> GeoPoint` | Decode cell center point |
| `neighbor` | `(hash: []const u8, dlat: i32, dlon: i32, dst: []u8) -> usize` | Compute 8-way neighbor with ±180° longitude wrap-around |
| `isValid` | `(hash: []const u8) -> bool` | Validate Geohash alphabet & length |

## Usage / 使用示例

```cp
import { encode, decode, neighbor } from "geohash";

let buf: [16]u8 = undefined;
const n = encode(57.64911, 10.40744, 11 as usize, buf[0..16]);
// buf[0..n] == "u4pruydqqvj"
```
