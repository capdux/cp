[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# geo-uri

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation RFC 5870 geo: URI parser, coordinate validator, and formatter for CP.

## 安装与导入

```bash
cap add geo-uri
```

```cp
import { isValidLatLng } from "geo-uri";
```

## 核心导出 API

- `export fn isValidLatLng(lat: f64, lng: f64): bool;`
- `export fn parseGeoUri(uri: string): GeoUri;`
- `export fn formatGeoUri(lat: f64, lng: f64, has_alt: bool, alt: f64, has_u: bool, u: f64, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`geo-uri@1.0.0`)，许可证：MIT
