# @cap/libavif@1.2.1 (Agent Quickref)

> AOMediaCodec libavif (v1.2.1) AVIF/HEIF image container & pixel format library for CP | License: BSD-2-Clause
> Import: `import { ... } from "@cap/libavif";` | Full types: `src/index.d.cp`

## Functions
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
