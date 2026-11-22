# `cap:qoi` — 纯 CP 零分配 QOI (Quite OK Image) 无损图像编解码库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:image`，导入请使用 `import { ... } from "cap:image"`。

## 概述

`cap:qoi` 提供纯 CP、零外部依赖、零堆分配的完整 QOI v1.0 无损图像编码器与解码器，实现全部 6 种 QOI 压缩块（`RUN`、`INDEX`、`DIFF`、`LUMA`、`RGB`、`RGBA`）与 64 槽颜色哈希表，适用于高性能轻量图像无损压缩与传输。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `QoiHeader` | 14 字节大端头 `"qoif"`：`width: u32`, `height: u32`, `channels: u8`（`3`=RGB / `4`=RGBA）, `colorspace: u8`（`0`=sRGB / `1`=linear） |
| `isQoi(data)` | 快速校验是否包含合法 QOI 文件头 |
| `parseQoiHeader(data)` | 解析并严格校验 14 字节大端头（`"qoif"` 魔数、`width > 0`、`height > 0`、`channels == 3 || 4`、`colorspace <= 1`） |
| `encodeQoi(pixels, width, height, channels, colorspace, out)` | 零堆分配无损编码 RGB/RGBA 像素至 `out`，返回实际写入切片；参数非法或缓冲不足返回 `null` |
| `decodeQoi(data, out_pixels)` | 零堆分配无损解码 QOI 流至 `out_pixels` 并返回 `QoiHeader`；校验 8 字节尾标记与边界不越界 |

## 示例

```cp
import { encodeQoi, decodeQoi } from "cap:image";

fn roundtrip(pixels: []const u8, w: u32, h: u32, enc_buf: []u8, dec_buf: []u8): bool {
    const encoded = encodeQoi(pixels, w, h, 4, 0, enc_buf) ?? return false;
    const hdr = decodeQoi(encoded, dec_buf) ?? return false;
    return hdr.width == w && hdr.height == h;
}
```
