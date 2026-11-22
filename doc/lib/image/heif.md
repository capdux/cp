# `cap:heif` — 纯 CP HEIF / HEIC ISO BMFF 图像容器零分配解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:image`，导入请使用 `import { ... } from "cap:image"`。

## 概述

`cap:heif` 提供纯 CP、零外部依赖、零堆分配的 HEIF / HEIC（ISO/IEC 23008-12 + ISO/IEC 14496-15）容器解析能力，支持 `ftyp` 品牌识别（`"heic"` / `"heix"` / `"hevc"` / `"hevx"` / `"heim"` / `"heis"` / `"hevm"` / `"hevs"` / `"mif1"` / `"msf1"`，并自动排除纯 `"avif"` / `"avis"`）、23 字节+ `hvcC`（HEVCDecoderConfigurationRecord）位字段解析，以及深入 `meta` -> `iprp` -> `ipco` 提取 `ispe`、`pixi`、`colr`（`nclx` 或 ICC）、`irot`（0/90/180/270 旋转）与 `auxC` Alpha/Depth 辅助通道检测。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `HEIF_BOX_HEADER_SIZE` / `HEIF_EXT_BOX_HEADER_SIZE` / `HEIF_HVCC_MIN_SIZE` | 常量：`8`、`16`、`23` |
| `isHeif(data: []const u8): bool` | 校验首块为 `ftyp` 且匹配 HEIF/HEIC 品牌，同时排除纯 AVIF/AVIS 容器 |
| `HeifBrandInfo` / `parseHeifBrand(data)` | 解析 `major_brand: [4]u8`, `minor_version: u32`, `is_sequence: bool`, `is_heic: bool` |
| `HevcConfig` / `parseHvcC(payload)` | 解析 23B+ `hvcC`：`configuration_version` (==1), `general_profile_space`, `general_tier_flag`, `general_profile_idc` (1=Main/2=Main10/3=MainStillPicture), `general_level_idc`, `chroma_format_idc` (0..3), `bit_depth_luma`, `bit_depth_chroma`, `length_size_minus_one`, `num_of_arrays` |
| `HeifColr` | 色彩信息：`color_type: [4]u8`（`"nclx"`/`"rICC"`/`"prof"`）, `color_primaries`, `transfer_characteristics`, `matrix_coefficients`, `full_range` |
| `HeifInfo` / `parseHeifInfo(data)` | 遍历 `ftyp` + `meta` -> `iprp` -> `ipco`，汇总 `brand`, `width`, `height`, `pixi_channels`, `pixi_bit_depth`, `hvcc`, `colr`, `has_icc_profile`, `rotation_degrees`, `has_alpha`, `has_depth` |

## 示例

```cp
import { isHeif, parseHeifInfo } from "cap:image";

fn inspectHeif(bytes: []const u8): u32 {
    if (!isHeif(bytes)) return 0;
    const info = parseHeifInfo(bytes) ?? return 0;
    return info.width;
}
```
