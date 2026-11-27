[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# ngeohash

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation Geohash Base32 spatial coordinate encoder, decoder, and bounding box library for CP.

## 安装与导入

```bash
cap add ngeohash
```

```cp
import { isValidGeohash } from "ngeohash";
```

## 核心导出 API

- `export fn isValidGeohash(hash: string): bool;`
- `export fn encode(lat: f64, lon: f64, precision: usize, out: []u8): usize;`
- `export fn decodeBounds(hash: string): GeoBounds;`
- `export fn decode(hash: string): GeoPoint;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`ngeohash@1.0.0`)，许可证：MIT
