# `cap:pnm` — 纯 CP 零分配 Netpbm (P1–P6) 图像编解码库

## 概述

`cap:pnm` 提供纯 CP、零外部依赖、零堆分配的 Netpbm（P1–P6：PBM / PGM / PPM）图像文件头解析器、P5/P6 原始像素零拷贝切片提取器与二进制 P5/P6 编码器，与 `cap:qoi` 配套用于轻量图像处理与测试管线。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `PnmFormat` | 格式枚举：`p1_pbm_ascii`, `p2_pgm_ascii`, `p3_ppm_ascii`, `p4_pbm_raw`, `p5_pgm_raw`, `p6_ppm_raw` |
| `PnmHeader` | 文件头：`format`, `width: u32`, `height: u32`, `maxval: u16`（P1/P4 为 `1`）, `data_offset: usize`；方法 `channels(): u8`、`isBinary(): bool`、`expectedRawBytes(): usize?` |
| `parsePnmHeader(data)` | 跳过 `#` 注释与空白解析 P1–P6 头（`maxval` 校验 `1..=65535`，末字段后仅消耗单空白保护二进制像素首字节） |
| `pnmRawPixels(data)` | 对 P5/P6 校验像素负载长度足够后返回零拷贝原始像素切片 |
| `encodePpmP6(rgb, width, height, out)` | 零堆分配编码 `P6\n<w> <h>\n255\n<rgb>`；长度不符或缓冲不足返回 `null` |
| `encodePgmP5(gray, width, height, out)` | 零堆分配编码 `P5\n<w> <h>\n255\n<gray>`；长度不符或缓冲不足返回 `null` |

## 示例

```cp
import { encodePpmP6, parsePnmHeader, pnmRawPixels } from "cap:pnm";

fn roundtripRgb(rgb: []const u8, w: u32, h: u32, out: []u8): bool {
    const encoded = encodePpmP6(rgb, w, h, out) ?? return false;
    const hdr = parsePnmHeader(encoded) ?? return false;
    const pixels = pnmRawPixels(encoded) ?? return false;
    return hdr.width == w && hdr.height == h && pixels.len == rgb.len;
}
```
