[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# polyline

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap Google Encoded Polyline (precision 5/6) GPS coordinate encoder and decoder for CP (compatible with @mapbox/polyline and Rust polyline)

## 安装与导入

```bash
cap add polyline
```

```cp
import { precisionScale } from "polyline";
```

## 核心导出 API

- `export fn precisionScale(precision: u32): f64;`
- `export fn roundToI64(v: f64): i64;`
- `export fn encodeSigned(val: i64, out: []u8): usize;`
- `export fn decodeSigned(data: string, start: usize, out_val: &mut i64): usize;`
- `export fn encodePoints(points: &[]LatLng, precision: u32, out: []u8): usize;`
- `export fn decodePoints(encoded: string, precision: u32, out: []LatLng): usize;`
- `export fn countPoints(encoded: string): usize;`
- `export fn isValid(encoded: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`polyline@1.0.0`)，许可证：MIT
