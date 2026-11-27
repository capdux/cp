[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/wkt

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:wkt` ([https://github.com/georust/wkt](https://github.com/georust/wkt))

Clean-room MIT, zero-heap GIS Well-Known Text (WKT) parser, formatter, and bounding-box calculator for CP — modeled on crates.io `wkt` v0.11.

## Features

- **Zero heap allocation**: fixed 32-coordinate `WktGeometry` stack struct and caller-provided `[]u8` formatting buffer.
- **Case-insensitive WKT parsing**: supports `POINT`, `LINESTRING`, `POLYGON`, `MULTIPOINT` (both nested `((x y), ...)` and flat `(x y, ...)`), and `EMPTY` forms with signed decimal & scientific coordinates.
- **Strict validation**: rejects unknown geometry types, malformed/unmatched parentheses, empty coordinate lists, and trailing garbage.
- **Formatting & Bounding Box**: `formatWkt` emits clean canonical WKT; `bboxWkt` computes the axis-aligned 2D bounding box.

## API

### Constants
- `WKT_POINT = 1`, `WKT_LINESTRING = 2`, `WKT_POLYGON = 3`, `WKT_MULTIPOINT = 4`, `WKT_MAX_COORDS = 32`

### Types
- `Coord2D { x: f64, y: f64 }`
- `WktGeometry { kind: u8, is_empty: bool, coords: [32]Coord2D, coord_count: usize }`
- `WktBbox { min_x: f64, min_y: f64, max_x: f64, max_y: f64, valid: bool }`

### Functions
- `parseWkt(input: string): WktGeometry?`
- `formatWkt(g: WktGeometry, out: []u8): usize`
- `bboxWkt(g: WktGeometry): WktBbox`

## License

MIT
