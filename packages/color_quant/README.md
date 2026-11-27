[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/color-quant

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:color_quant` ([https://github.com/image-rs/color_quant](https://github.com/image-rs/color_quant))

Zero-allocation pure CP 256-color palette quantizer, 6x6x6 Web-safe + grayscale uniform palette builder, 3-3-2 histogram quantizer, and nearest-color pixel mapper — clean-room parity with `image-rs/color_quant` (173.6M+ downloads on crates.io, MIT).

纯 CP 零堆分配 256 色图像调色板量化器，提供标准 6×6×6 Web-safe + 40 级灰度 256 色调色板、3-3-2 量化桶聚合与均值代表色提取，以及欧氏距离平方最小最近色索引映射。

## Install / 安装

```bash
cap add @cap/color-quant
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `Rgb8` | `struct { r: u8; g: u8; b: u8; }` | 24-bit RGB color |
| `ColorPalette` | `struct { colors: [256]Rgb8; len: usize; fn nearestIndex(px: Rgb8): u8 }` | Fixed-capacity 256-color palette with nearest-color lookup |
| `buildUniformPalette` | `(): ColorPalette` | Build 6x6x6 Web-safe + 40-step grayscale 256-color palette |
| `quantizeHistogram` | `(pixels: []const Rgb8, max_colors: usize): ColorPalette` | 3-3-2 bucket histogram quantization with mean representative colors |
| `mapPixelsToPalette` | `(pixels: []const Rgb8, pal: ColorPalette, out_indices: []u8): usize` | Map RGB pixels to palette indices |

## Usage / 使用示例

```cp
import { Rgb8, buildUniformPalette, quantizeHistogram, mapPixelsToPalette } from "@cap/color-quant";

const pal = buildUniformPalette();
const idx = pal.nearestIndex(Rgb8{ r: 255, g: 0, b: 0 });
```
