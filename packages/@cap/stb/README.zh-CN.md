[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @cap/stb

> 版本：`2.30.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Single-header image decoding (PNG/JPG/BMP/TGA/GIF), PNG encoding and SIMD image resizing (nothings/stb) for CP

## 安装与导入

```bash
cap add @cap/stb
```

```cp
import { cp_stbi_load_from_memory } from "@cap/stb";
```

## 核心导出 API

- `export fn cp_stbi_load_from_memory(buffer: [*]&u8!, len: i32, x: *i32!?, y: *i32!?, channels_in_file: *i32!?, desired_channels: i32): [*]u8!?;`
- `export fn cp_stbi_image_free(ptr: *auto!?);`
- `export fn cp_stbi_write_png_to_mem(pixels: [*]&u8!, w: i32, h: i32, comp: i32, stride_bytes: i32, out_len: *i32!?): [*]u8!?;`
- `export fn cp_stbiw_free(ptr: *auto!?);`
- `export fn cp_stbir_resize_rgba8(in_pixels: [*]&u8!, in_w: i32, in_h: i32, out_pixels: [*]u8!, out_w: i32, out_h: i32): i32;`
- `export fn resizeRgba8(input: &[]u8, in_w: i32, in_h: i32, output: []u8, out_w: i32, out_h: i32): !void;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`c` (`stb@2.30.0`)，许可证：MIT
