# `cap:psd` — 纯 CP Adobe Photoshop PSD/PSB 容器与 `8BIM` 资源块零分配解析库

## 概述

`cap:psd` 提供纯 CP、零外部依赖、零堆分配的 Adobe Photoshop `.psd`（v1）与 Large Document `.psb`（v2）解析能力，支持 26 字节大端文件头校验、Color Mode / Image Resources / Layer & Mask（自动区分 PSD 32-bit 与 PSB 64-bit 区段长）/ Image Data 全区段零拷贝切分，以及 `8BIM` Image Resource Block 2 字节偶对齐流式迭代与按 ID 查询。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `PSD_HEADER_SIZE` / `PSD_VERSION` / `PSB_VERSION` | 常量：`26`、`1`、`2` |
| `PSD_RES_RESOLUTION_INFO` / `PSD_RES_ICC_PROFILE` / `PSD_RES_XMP_METADATA` | 常用资源块 ID：`0x03ED`、`0x040F`、`0x0424` |
| `PsdColorMode` / `PsdCompression` | 色彩模式枚举（Bitmap/Grayscale/Indexed/RGB/CMYK/Multichannel/Duotone/Lab）与压缩算法枚举（Raw/RLE/ZIP/ZIP+Pred） |
| `isPsd(data)` / `PsdHeader` / `parsePsdHeader(data)` | 校验 `"8BPS"` 魔数、版本 `1`/`2`、6B 全零保留字段、通道数（1..56）及位深（1/8/16/32）；附 `isPsb(): bool` |
| `PsdSections` / `parsePsdSections(data)` | 零拷贝切分 4 大区段与图像压缩类型 |
| `PsdResource` / `PsdResourceIter` / `findPsdResource(sections, id)` | 迭代与按 ID 检索 `8BIM` 资源块（自动处理 Pascal 名称与负载的 2B 偶对齐填充） |

## 示例

```cp
import { isPsd, parsePsdSections, findPsdResource, PSD_RES_ICC_PROFILE } from "cap:psd";

fn hasIccProfile(bytes: []const u8): bool {
    if (!isPsd(bytes)) return false;
    const sec = parsePsdSections(bytes) ?? return false;
    if (_icc := findPsdResource(sec, PSD_RES_ICC_PROFILE)) {
        return true;
    }
    return false;
}
```
