[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# geohash

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation Base32 Geohash encoder, bounding-box decoder, and 8-direction neighbor finder (Rust geohash / ngeohash parity) for CP

## 安装与导入

```bash
cap add geohash
```

```cp
import { isValid } from "geohash";
```

## 核心导出 API

- `export fn isValid(hash: &[]u8): bool;`
- `export fn encode(lat: f64, lon: f64, precision: usize, dst: []u8): usize;`
- `export fn decodeBBox(hash: &[]u8): GeoBBox;`
- `export fn decode(hash: &[]u8): GeoPoint;`
- `export fn neighbor(hash: &[]u8, dlat: i32, dlon: i32, dst: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`geohash@1.0.0`)，许可证：MIT
