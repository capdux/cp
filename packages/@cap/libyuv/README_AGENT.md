# @cap/libyuv@1.9.0 (Agent Quickref)

> Google libyuv high-performance YUV/RGB pixel conversion, scaling, and rotation for CP | License: BSD-3-Clause
> Import: `import { ... } from "@cap/libyuv";` | Full types: `src/index.d.cp`

## Functions
- `export fn I420ToARGB(src_y: [*]&u8!, src_stride_y: i32, src_u: [*]&u8!, src_stride_u: i32, src_v: [*]&u8!, src_stride_v: i32, dst_argb: [*]u8!, dst_stride_argb: i32, width: i32, height: i32): i32;`
- `export fn ARGBToI420(src_argb: [*]&u8!, src_stride_argb: i32, dst_y: [*]u8!, dst_stride_y: i32, dst_u: [*]u8!, dst_stride_u: i32, dst_v: [*]u8!, dst_stride_v: i32, width: i32, height: i32): i32;`
- `export fn ARGBScale(src_argb: [*]&u8!, src_stride_argb: i32, src_width: i32, src_height: i32, dst_argb: [*]u8!, dst_stride_argb: i32, dst_width: i32, dst_height: i32, filtering: i32): i32;`
- `export fn I420Rotate(src_y: [*]&u8!, src_stride_y: i32, src_u: [*]&u8!, src_stride_u: i32, src_v: [*]&u8!, src_stride_v: i32, dst_y: [*]u8!, dst_stride_y: i32, dst_u: [*]u8!, dst_stride_u: i32, dst_v: [*]u8!, dst_stride_v: i32, width: i32, height: i32, mode: i32): i32;`
- `export fn absI32(v: i32): i32;`
- `export fn i420ToArgb(src_y: &[]u8, src_stride_y: i32, src_u: &[]u8, src_stride_u: i32, src_v: &[]u8, src_stride_v: i32, dst_argb: []u8, dst_stride_argb: i32, width: i32, height: i32): !void;`
- `export fn argbToI420(src_argb: &[]u8, src_stride_argb: i32, dst_y: []u8, dst_stride_y: i32, dst_u: []u8, dst_stride_u: i32, dst_v: []u8, dst_stride_v: i32, width: i32, height: i32): !void;`
- `export fn argbScale(src_argb: &[]u8, src_stride_argb: i32, src_width: i32, src_height: i32, dst_argb: []u8, dst_stride_argb: i32, dst_width: i32, dst_height: i32, filtering: i32): !void;`
- `export fn i420Rotate(src_y: &[]u8, src_stride_y: i32, src_u: &[]u8, src_stride_u: i32, src_v: &[]u8, src_stride_v: i32, dst_y: []u8, dst_stride_y: i32, dst_u: []u8, dst_stride_u: i32, dst_v: []u8, dst_stride_v: i32, width: i32, height: i32, mode: i32): !void;`
