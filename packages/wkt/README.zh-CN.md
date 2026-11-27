[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# wkt

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap GIS Well-Known Text (WKT) parser, formatter, and bounding-box calculator for POINT, LINESTRING, POLYGON, and MULTIPOINT geometries (clean-room port of Rust wkt)

## 安装与导入

```bash
cap add wkt
```

```cp
import { wktZeroCoord } from "wkt";
```

## 核心导出 API

- `export fn wktZeroCoord(): Coord2D;`
- `export fn wktEmptyGeom(kind: u8): WktGeometry;`
- `export fn wktIsSpace(c: u8): bool;`
- `export fn wktIsAlpha(c: u8): bool;`
- `export fn wktIsDigit(c: u8): bool;`
- `export fn wktToLower(c: u8): u8;`
- `export fn wktSkipWs(s: string, pos: &mut usize);`
- `export fn wktWordEq(buf: &[]u8, len: usize, lit: string): bool;`
- `export fn wktReadWord(s: string, pos: &mut usize, out: []u8): usize;`
- `export fn wktParseNumber(s: string, pos: &mut usize, out: &mut f64): bool;`
- `export fn wktParseCoord(s: string, pos: &mut usize, out: &mut Coord2D): bool;`
- `export fn wktConsumeChar(s: string, pos: &mut usize, ch: u8): bool;`
- `export fn wktWriteStr(out: []u8, pos: usize, s: string): usize;`
- `export fn wktWriteU64(out: []u8, pos: usize, val: u64): usize;`
- `export fn wktWriteF64(out: []u8, pos: usize, val: f64): usize;`
- `export fn wktWriteCoord(out: []u8, pos: usize, c: Coord2D): usize;`
- `export fn parseWkt(input: string): WktGeometry?;`
- `export fn formatWkt(g: WktGeometry, out: []u8): usize;`
- `export fn bboxWkt(g: WktGeometry): WktBbox;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`wkt@1.0.0`)，许可证：MIT OR Apache-2.0
