[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# color_quant

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation pure CP 256-color palette quantizer and nearest-color mapper (image-rs color_quant parity)

## 安装与导入

```bash
cap add color_quant
```

```cp
import { clqtEmptyPalette } from "color_quant";
```

## 核心导出 API

- `export fn clqtEmptyPalette(): ColorPalette;`
- `export fn clqtBucketIndex(px: Rgb8): usize;`
- `export fn buildUniformPalette(): ColorPalette;`
- `export fn quantizeHistogram(pixels: &[]Rgb8, max_colors: usize): ColorPalette;`
- `export fn mapPixelsToPalette(pixels: &[]Rgb8, pal: ColorPalette, out_indices: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`color_quant@1.0.0`)，许可证：MIT
