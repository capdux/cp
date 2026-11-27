[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @cap/libavif

> 版本：`1.2.1` | 许可证：`BSD-2-Clause` | 类型声明：[src/index.d.cp](./src/index.d.cp)

AOMediaCodec libavif (v1.2.1) AVIF/HEIF image container & pixel format library for CP

## 安装与导入

```bash
cap add @cap/libavif
```

```cp
import { cp_avif_version } from "@cap/libavif";
```

## 核心导出 API

- `export fn cp_avif_version(): [*:0]&u8!?;`
- `export fn cp_avif_pixel_format_to_string(fmt: i32): [*:0]&u8!?;`
- `export fn cp_avif_image_create(width: u32, height: u32, depth: u32, yuv_format: i32): *auto!?;`
- `export fn cp_avif_image_destroy(img: *auto!?);`
- `export fn cp_avif_image_width(img: *auto!?): u32;`
- `export fn cp_avif_image_height(img: *auto!?): u32;`
- `export fn cp_avif_image_depth(img: *auto!?): u32;`
- `export fn cp_avif_image_create_and_destroy(width: u32, height: u32, depth: u32): i32;`
- `export fn version(): string;`
- `export fn pixelFormatName(fmt: i32): string;`
- `export fn imageCreateAndDestroy(width: u32, height: u32, depth: u32): !void;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`c` (`libavif@1.2.1`)，许可证：BSD-2-Clause
