[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/geojson

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:geojson` ([https://github.com/georust/geojson](https://github.com/georust/geojson))

Clean-room MIT, zero-heap RFC 7946 GeoJSON geometry & feature parser, formatter, and WKT interop for CP — modeled on crates.io `geojson` v0.24 and built directly on [`@cap/wkt`](../wkt/README.md).

## Features

- **Zero heap allocation**: reuses `@cap/wkt`'s fixed 32-coordinate `WktGeometry`, `Coord2D`, and `WktBbox` types with caller-provided `[]u8` output buffers.
- **RFC 7946 Geometry Formatting & Parsing**: supports `Point`, `LineString`, and `Polygon` with key-order-independent JSON parsing and strict validation of types and coordinate arrays.
- **RFC 7946 Feature Formatting**: `formatGeoJsonFeature` emits `"type":"Feature"`, optional escaped `"id"`, optional `"bbox"` (via `bboxWkt`), `"geometry"`, and `"properties":{}`.
- **One-step GeoJSON ↔ WKT Interop**: `wktToGeoJson` and `geoJsonToWkt` convert seamlessly between WKT and GeoJSON with zero intermediate allocations.

## Dependencies

- `@cap/wkt` `1.0.0`

## API

### Types
- `GeoJsonGeometry { kind: u8, coords: [32]Coord2D, coord_count: usize }` (`fromWkt`, `toWkt`)
- `GeoJsonFeature { id: string, geom: WktGeometry, has_bbox: bool, bbox: WktBbox }` (`init`)

### Functions
- `formatGeoJsonGeometry(g: WktGeometry, out: []u8): usize`
- `formatGeoJsonFeature(f: GeoJsonFeature, out: []u8): usize`
- `parseGeoJsonPoint(json: string): WktGeometry?`
- `parseGeoJsonGeometry(json: string): WktGeometry?`
- `wktToGeoJson(wkt_str: string, out: []u8): usize`
- `geoJsonToWkt(json: string, out: []u8): usize`

## License

MIT
