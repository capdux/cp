# `cap:tga` — 纯 CP 零分配 Truevision TGA 图像编解码库

## 概述

`cap:tga` 提供纯 CP、零外部依赖、零堆分配的 Truevision TGA 18 字节文件头解析器、未压缩与 RLE 压缩（24/32-bit TrueColor 与 8-bit 灰度）RGBA 解码器，以及 24/32-bit top-origin TGA 编码器，与 `cap:qoi`、`cap:pnm` 配套。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `TgaImageType` | 图像类型枚举：`no_image`, `color_mapped`, `true_color`, `grayscale`, `rle_color_mapped`, `rle_true_color`, `rle_grayscale`, `unknown` |
| `TgaHeader` | 18 字节小端头：`id_length`, `color_map_type`, `image_type`, `width: u16`, `height: u16`, `pixel_depth: u8`（`8`/`24`/`32`）, `descriptor: u8`；方法 `isTopOrigin(): bool`、`isRle(): bool`、`bytesPerPixel(): u8`、`pixelDataOffset(): usize` |
| `parseTgaHeader(data)` | 严格校验 `>= 18` 字节、非零尺寸、合法色深与 `pixelDataOffset() <= data.len` |
| `encodeTgaTrueColor(rgba_or_rgb, width, height, channels, out)` | 将 RGB/RGBA 转换为 BGR/BGRA 并编码为 top-origin 未压缩 TGA；缓冲不足返回 `null` |
| `decodeTgaToRgba(data, out_rgba)` | 解码未压缩与 RLE 压缩的 24/32-bit TrueColor 与 8-bit 灰度 TGA 为标准 top-origin RGBA（自动处理 bottom-origin 翻转），严格防越界 |

## 示例

```cp
import { encodeTgaTrueColor, decodeTgaToRgba } from "cap:tga";

fn roundtripTga(rgba: []const u8, w: u16, h: u16, tga_buf: []u8, out_rgba: []u8): bool {
    const encoded = encodeTgaTrueColor(rgba, w, h, 4, tga_buf) ?? return false;
    const hdr = decodeTgaToRgba(encoded, out_rgba) ?? return false;
    return hdr.width == w && hdr.height == h;
}
```
