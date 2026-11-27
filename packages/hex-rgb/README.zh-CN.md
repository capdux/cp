[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# hex-rgb

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation CSS hex color (#RGB, #RGBA, #RRGGBB, #RRGGBBAA) parser, formatter, Porter-Duff source-over blender, luminance, and RGB inverter for CP.

## 安装与导入

```bash
cap add hex-rgb
```

```cp
import { parseHexColor } from "hex-rgb";
```

## 核心导出 API

- `export fn parseHexColor(input: string): Rgba;`
- `export fn formatHex6(c: Rgba, uppercase: bool, out: []u8): usize;`
- `export fn formatHex8(c: Rgba, uppercase: bool, out: []u8): usize;`
- `export fn packRgba32(c: Rgba): u32;`
- `export fn unpackRgba32(v: u32): Rgba;`
- `export fn invertRgb(c: Rgba): Rgba;`
- `export fn isDark(c: Rgba): bool;`
- `export fn blendOver(fg: Rgba, bg: Rgba): Rgba;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`hex-rgb@1.0.0`)，许可证：MIT
- **上游生态**：`npm` (`colord@1.0.0`)，许可证：MIT
