# `cap:avif` — 纯 CP AVIF / MIAF ISO BMFF 图像容器零分配解析库

## 概述

`cap:avif` 提供纯 CP、零外部依赖、零堆分配的 AVIF / MIAF（ISO/IEC 14496-12 + AOMedia AVIF）容器解析能力，支持 `ftyp` 品牌识别（`"avif"` / `"avis"` / `"mif1"` / `"miaf"`）、`av1C`（AV1CodecConfigurationRecord）位字段解析，以及深入 `meta` -> `iprp` -> `ipco` 提取 `ispe`（宽高）、`pixi`（通道数与位深）、`colr`（`nclx` HDR 色域或 `rICC`/`prof` ICC 标志）、`irot`（0/90/180/270 旋转角度）与 `auxC` Alpha 通道检测。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `AVIF_BOX_HEADER_SIZE` / `AVIF_EXT_BOX_HEADER_SIZE` | ISO BMFF 基础 Box 头（`8`）与 64-bit 扩展头（`16`）长度常量 |
| `isAvif(data: []const u8): bool` | 校验首块为 `ftyp` 且主品牌或兼容品牌包含 `"avif"` 或 `"avis"` |
| `AvifBrandInfo` / `parseAvifBrand(data)` | 解析 `major_brand: [4]u8`, `minor_version: u32`, `is_sequence: bool`, `has_mif1: bool`, `has_miaf: bool` |
| `Av1Config` / `parseAv1C(payload)` | 解析 4B+ `av1C` 配置记录：`marker_ok`, `version`, `seq_profile` (0=Main/1=High/2=Prof), `seq_level_idx_0`, `seq_tier_0`, `high_bitdepth`, `twelve_bit`, `monochrome`, `chroma_subsampling_x`, `chroma_subsampling_y`, `chroma_sample_position`；附 `bitDepth(): u8`（8/10/12） |
| `AvifColr` | 色彩信息结构：`color_type: [4]u8`（`"nclx"`/`"rICC"`/`"prof"`）, `color_primaries`, `transfer_characteristics`, `matrix_coefficients`, `full_range` |
| `AvifInfo` / `parseAvifInfo(data)` | 遍历顶层 `ftyp` 与 `meta` -> `iprp` -> `ipco`，汇总 `brand`, `width`, `height`, `pixi_channels`, `pixi_bit_depth`, `av1c`, `colr`, `has_icc_profile`, `rotation_degrees`, `has_alpha` |

## 示例

```cp
import { isAvif, parseAvifInfo } from "cap:avif";

fn inspectAvif(bytes: []const u8): u32 {
    if (!isAvif(bytes)) return 0;
    const info = parseAvifInfo(bytes) ?? return 0;
    return info.width;
}
```
