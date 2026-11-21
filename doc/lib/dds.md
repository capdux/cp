# `cap:dds` — 纯 CP 零分配 Microsoft DDS GPU 纹理头与 Mip 尺寸计算库

## 概述

`cap:dds` 提供纯 CP、零外部依赖、零堆分配的 Microsoft DirectDraw Surface (`.dds`) 128 字节标准头解析、可选 20 字节 DX10 扩展头解析，以及 DXT/BCn 块压缩 Mip 层字节大小防溢出计算器，与 `cap:wgsl`、`cap:glsl` 配套用于 GPU 纹理资源管线。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `DdsPixelFormat` | 32 字节 `DDS_PIXELFORMAT`：`flags`, `fourcc: [4]u8`, `rgb_bit_count`, `r_mask`/`g_mask`/`b_mask`/`a_mask`；方法 `isFourcc(): bool`、`isDx10(): bool` |
| `DdsDx10Header` | 20 字节 `DDS_HEADER_DXT10`：`dxgi_format`, `resource_dimension`, `misc_flag`, `array_size`, `misc_flags2` |
| `DdsHeader` | 128/148 字节纹理头：`flags`, `height`, `width`, `pitch_or_linear_size`, `depth`, `mip_map_count`, `pixel_format`, `caps`, `caps2`, `dx10: DdsDx10Header?`, `data_offset: usize`；方法 `isCubemap(): bool`、`isVolume(): bool` |
| `isDds(data)` | 校验 4 字节 `"DDS "` 魔数 |
| `parseDdsHeader(data)` | 校验 `>= 128` 字节、`dwSize == 124`、`ddspf.dwSize == 32`，若为 `DX10` 则校验 `>= 148` 字节并解析扩展头 |
| `calcDdsMipSize(width, height, block_bytes)` | 按 `max(1, (w+3)/4) * max(1, (h+3)/4) * block_bytes` 防溢出计算 BC1 (8B) / BC3/BC7 (16B) 单层 Mip 字节大小 |

## 示例

```cp
import { isDds, parseDdsHeader, calcDdsMipSize } from "cap:dds";

fn inspectBc7Mip0(dds_bytes: []const u8): u64? {
    if (!isDds(dds_bytes)) return null;
    const hdr = parseDdsHeader(dds_bytes) ?? return null;
    return calcDdsMipSize(hdr.width, hdr.height, 16);
}
```
