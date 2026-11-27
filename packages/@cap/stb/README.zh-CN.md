[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @cap/stb

Single-header image decoding (PNG/JPG/BMP/TGA/GIF), PNG encoding and SIMD image resizing (nothings/stb) for CP

| 元信息 | 值 |
| --- | --- |
| 包名 | `@cap/stb` |
| 版本 | `2.30.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @cap/stb
```

```cp
import {
    StbError,
    StbImage,
    PngBuffer,
    cp_stbi_load_from_memory,
    cp_stbi_image_free,
    cp_stbi_write_png_to_mem,
    cp_stbiw_free,
    cp_stbir_resize_rgba8,
} from "@cap/stb";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `error` | `StbError` | `DecodeFailed, EncodeFailed, ResizeFailed, InvalidArg, BufferTooSmall` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `StbImage` | `class` | 字段: `raw: [*]u8!?, width: i32, height: i32, channels: i32` · 方法: `loadFromMemory, pixels, __cp_deinit` |
| `PngBuffer` | `class` | 字段: `raw: [*]u8!?, len: usize` · 方法: `encode, bytes, __cp_deinit` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `cp_stbi_load_from_memory` | `(buffer: [*]&u8!, len: i32, x: *i32!?, y: *i32!?, channels_in_file: *i32!?, desired_channels: i32): [*]u8!?` | 调用 `cp_stbi_load_from_memory` 并返回 `[*]u8!?`。 |
| `cp_stbi_image_free` | `(ptr: *auto!?)` | 就地执行 `cp_stbi_image_free` 操作。 |
| `cp_stbi_write_png_to_mem` | `(pixels: [*]&u8!, w: i32, h: i32, comp: i32, stride_bytes: i32, out_len: *i32!?): [*]u8!?` | 调用 `cp_stbi_write_png_to_mem` 并返回 `[*]u8!?`。 |
| `cp_stbiw_free` | `(ptr: *auto!?)` | 就地执行 `cp_stbiw_free` 操作。 |
| `cp_stbir_resize_rgba8` | `(in_pixels: [*]&u8!, in_w: i32, in_h: i32, out_pixels: [*]u8!, out_w: i32, out_h: i32): i32` | 调用 `cp_stbir_resize_rgba8` 并返回 `i32`。 |
| `resizeRgba8` | `(input: &[]u8, in_w: i32, in_h: i32, output: []u8, out_w: i32, out_h: i32): !void` | 调用 `resizeRgba8` 并返回 `!void`。 |

## 最小调用示例

```cp
import { resizeRgba8 } from "@cap/stb";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let mut buf_1: [256]u8 = undefined;
    let result = resizeRgba8(out_buf[0..256], 0, 0, buf_1[0..256], 0, 0) catch return;
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **零开销原生绑定**：通过 CP FFI 直接调用底层原生 C 库，无 GC 停顿与动态运行时桥接开销。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **确定性资源释放**：底层句柄支持 RAII `__cp_deinit` 或 `defer` 显式释放，防止句柄与内存泄漏。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
