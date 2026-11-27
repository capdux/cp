[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# plotters-svg

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap pure CP SVG vector graphics drawing backend (plotters-svg parity)

## 安装与导入

```bash
cap add plotters-svg
```

```cp
import { psvgWriteByte } from "plotters-svg";
```

## 核心导出 API

- `export fn psvgWriteByte(buf: []u8, pos: usize, b: u8): usize;`
- `export fn psvgWriteStr(buf: []u8, pos: usize, s: string): usize;`
- `export fn psvgWriteU32(buf: []u8, pos: usize, val: u32): usize;`
- `export fn psvgWriteI32(buf: []u8, pos: usize, val: i32): usize;`
- `export fn psvgWriteRgb(buf: []u8, pos: usize, c: SvgColor): usize;`
- `export fn psvgWriteXmlEscaped(buf: []u8, pos: usize, text: string): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`plotters-svg@1.0.0`)，许可证：MIT
