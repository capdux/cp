[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# morton-encoding

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation 2D/3D Z-Order (Morton) bit-interleaving codec and 2D Hilbert space-filling curve mapper for CP

## 安装与导入

```bash
cap add morton-encoding
```

```cp
import { splitBy1 } from "morton-encoding";
```

## 核心导出 API

- `export fn splitBy1(v: u32): u64;`
- `export fn compactBy1(v: u64): u32;`
- `export fn encode2D(x: u32, y: u32): u64;`
- `export fn decode2D(code: u64): Point2D;`
- `export fn splitBy2(v: u32): u64;`
- `export fn compactBy2(v: u64): u32;`
- `export fn encode3D(x: u32, y: u32, z: u32): u64;`
- `export fn decode3D(code: u64): Point3D;`
- `export fn clampOrder(order: u32): u32;`
- `export fn hilbertRot(n: u32, x_in: u32, y_in: u32, rx: u32, ry: u32): Point2D;`
- `export fn hilbert2DEncode(order: u32, x_in: u32, y_in: u32): u64;`
- `export fn hilbert2DDecode(order: u32, d_in: u64): Point2D;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`morton-encoding@1.0.0`)，许可证：MIT
