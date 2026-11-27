[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# geojson

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap RFC 7946 GeoJSON geometry & feature parser, formatter, and WKT interop built on @cap/wkt (clean-room port of Rust geojson)

## 安装与导入

```bash
cap add geojson
```

```cp
import { gjZeroCoord } from "geojson";
```

## 核心导出 API

- `export fn gjZeroCoord(): Coord2D;`
- `export fn gjEmptyGeom(kind: u8): WktGeometry;`
- `export fn gjIsSpace(c: u8): bool;`
- `export fn gjIsDigit(c: u8): bool;`
- `export fn gjSkipWs(s: string, pos: &mut usize);`
- `export fn gjConsumeChar(s: string, pos: &mut usize, ch: u8): bool;`
- `export fn gjStrEq(a: string, b: string): bool;`
- `export fn gjWriteByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn gjWriteStr(out: []u8, pos: usize, s: string): usize;`
- `export fn gjWriteJsonEscaped(out: []u8, pos: usize, s: string): usize;`
- `export fn gjWriteU64(out: []u8, pos: usize, val: u64): usize;`
- `export fn gjWriteF64(out: []u8, pos: usize, val: f64): usize;`
- `export fn gjWriteCoord(out: []u8, pos: usize, c: Coord2D): usize;`
- `export fn gjParseNumber(s: string, pos: &mut usize, out: &mut f64): bool;`
- `export fn gjParseStringSlice(s: string, pos: &mut usize, start_out: &mut usize, end_out: &mut usize): bool;`
- `export fn gjSkipValue(s: string, pos: &mut usize, depth: u32): bool;`
- `export fn gjParsePosition(s: string, pos: &mut usize, out: &mut Coord2D): bool;`
- `export fn gjParsePosList(s: string, pos: &mut usize, geom: &mut WktGeometry): bool;`
- `export fn formatGeoJsonGeometry(g: WktGeometry, out: []u8): usize;`
- `export fn formatGeoJsonFeature(f: GeoJsonFeature, out: []u8): usize;`
- `export fn parseGeoJsonGeometry(json: string): WktGeometry?;`
- `export fn parseGeoJsonPoint(json: string): WktGeometry?;`
- `export fn wktToGeoJson(wkt_str: string, out: []u8): usize;`
- `export fn geoJsonToWkt(json: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`geojson@1.0.0`)，许可证：MIT OR Apache-2.0
