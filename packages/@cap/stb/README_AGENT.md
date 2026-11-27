# @cap/stb@2.30.0 (Agent Quickref)

> Single-header image decoding (PNG/JPG/BMP/TGA/GIF), PNG encoding and SIMD image resizing (nothings/stb) for CP | License: MIT
> Import: `import { ... } from "@cap/stb";` | Full types: `src/index.d.cp`

## Functions
- `export fn cp_stbi_load_from_memory(buffer: [*]&u8!, len: i32, x: *i32!?, y: *i32!?, channels_in_file: *i32!?, desired_channels: i32): [*]u8!?;`
- `export fn cp_stbi_image_free(ptr: *auto!?);`
- `export fn cp_stbi_write_png_to_mem(pixels: [*]&u8!, w: i32, h: i32, comp: i32, stride_bytes: i32, out_len: *i32!?): [*]u8!?;`
- `export fn cp_stbiw_free(ptr: *auto!?);`
- `export fn cp_stbir_resize_rgba8(in_pixels: [*]&u8!, in_w: i32, in_h: i32, out_pixels: [*]u8!, out_w: i32, out_h: i32): i32;`
- `export fn resizeRgba8(input: &[]u8, in_w: i32, in_h: i32, output: []u8, out_w: i32, out_h: i32): !void;`
