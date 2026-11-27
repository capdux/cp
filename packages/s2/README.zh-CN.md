[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# s2

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap Google S2 spherical geometry: 64-bit S2CellId face/level/parent/child/contains, LatLng validation, and spherical distance (crates.io s2 equivalent)

## 安装与导入

```bash
cap add s2
```

```cp
import { s2TrailingZeros } from "s2";
```

## 核心导出 API

- `export fn s2TrailingZeros(v: u64): u8;`
- `export fn s2MaskAbove(lsb_val: u64): u64;`
- `export fn s2Sin(x_in: f64): f64;`
- `export fn s2Cos(x_in: f64): f64;`
- `export fn s2Sqrt(x: f64): f64;`
- `export fn s2Asin(x: f64): f64;`
- `export fn haversineMeters(a: LatLng, b: LatLng): f64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`s2@1.0.0`)，许可证：Apache-2.0
