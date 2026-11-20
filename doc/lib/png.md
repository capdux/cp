# CP 标准库：纯原生 PNG 图像编解码引擎手册 (`cap:png` / `cap.png`)

## 一、 架构总览与设计准则

`cap:png` 是 CP 语言官方内置的纯原生 PNG（Portable Network Graphics，RFC 2083）图像编解码标准库，基于 `cap:deflate`（`compressZlib`、`decompressZlib`、`crc32`）实现完整 IDAT 压缩流打包解包与 Chunk CRC-32 校验。

全模块**零 C `libpng` 依赖、零 `std.posix`、零 `@import("bun")`**，支持 `import ... from "cap:png"` 与 `import cap from "cap"; cap.png.*` 编译期自动惰性挂载。

### 模块文件结构

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/png/png.cp` | `// Source: cp/lib/png/png.cp (Pure CP PNG Image Codec via cap:deflate)` | `PngError`、`ColorType`、`PngImage`、5 种扫描行滤波与 `encode`/`decode` 核心实现 |
| `lib/png/index.cp` | `// Source: cp/lib/png/index.cp (Public Unified Export for cap:png & cap.png)` | `cap:png` 与 `cap.png` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集与色彩枚举

- **`PngError`**：
  - `InvalidSignature`：PNG 8 字节文件头 `\x89PNG\r\n\x1a\n` 校验失败
  - `InvalidHeader`：缺失 `IHDR`、非法的宽高（为 0 或与像素长度不匹配）、缺失 `PLTE` 或调色板越界
  - `ChecksumMismatch`：任意 Chunk 的 IEEE 802.3 CRC-32 校验失败或 Zlib Adler-32 校验失败
  - `UnsupportedFormat`：非 8-bit 位深、不支持的压缩/滤波/交错方式或 `encode` 传入 `Indexed`
  - `InvalidFilterType`：扫描行滤波字节超出 `0..4` 范围
  - `TruncatedData`：文件或 Chunk 数据截断、缺失 `IDAT` 数据
  - `MissingIend`：未检测到终止块 `IEND`
  - `ImageTooLarge`：图像宽高超过 `decodeWithMaxDimensions` 指定上限
  - `OutOfMemory`：分配器内存不足

- **`ColorType`**（`u8`）：
  - `Grayscale = 0`（1 BPP）
  - `Rgb = 2`（3 BPP）
  - `Indexed = 3`（1 BPP 调色板索引，解码时自动展开为 `Rgb` 或含 `tRNS` 时展开为 `Rgba`）
  - `GrayscaleAlpha = 4`（2 BPP）
  - `Rgba = 6`（4 BPP）

### 2. 图像容器与编解码函数

- **`PngImage`**：
  - 字段：`width: u32`、`height: u32`、`bit_depth: u8`、`color_type: ColorType`、`pixels: []u8`
  - 方法：
    - `bytesPerPixel(): usize`：返回当前 `color_type` 对应的每像素字节数
    - `deinit(*_, allocator: std.mem.Allocator)`：释放 `pixels` 切片内存

- **`encode(allocator: std.mem.Allocator, width: u32, height: u32, color_type: ColorType, pixels: []const u8): ([]u8)!PngError`**
  - 支持 8-bit `Grayscale`、`Rgb`、`GrayscaleAlpha`、`Rgba`。
  - 校验 `pixels.len == width * height * bpp`，逐行基于最小绝对差和（SAD）自动选择最优滤波器（`None` / `Sub` / `Up` / `Average` / `Paeth`），经 `compressZlib` 打包 `IHDR` + `IDAT` + `IEND` 并附带严格 CRC-32。

- **`decode(allocator: std.mem.Allocator, png_bytes: []const u8): PngImage!PngError`**
  - 校验 8 字节 PNG 签名、逐块验证 CRC-32、拼接多 `IDAT` 块并调用 `decompressZlib` 解压、执行 5 种扫描行反向滤波重构，并自动展开 `PLTE`（及可选 `tRNS`）调色板图像。

- **`decodeWithMaxDimensions(allocator: std.mem.Allocator, png_bytes: []const u8, max_w: u32, max_h: u32): PngImage!PngError`**
  - 在 `decode` 基础上强制拦截 `width > max_w || height > max_h` 的超大图，返回 `PngError.ImageTooLarge`，防止内存膨胀。

---

## 三、 使用示例

```typescript
const std = @import("std");
import cap from "cap";
import { ColorType, encode, decode, decodeWithMaxDimensions } from "cap:png";

pub fn roundtripPng(alloc: std.mem.Allocator): !void {
    const rgba: [16]u8 = [
        255, 0, 0, 255,   0, 255, 0, 255,
        0, 0, 255, 128,   255, 255, 255, 0,
    ];
    const encoded = encode(alloc, 2, 2, ColorType.Rgba, &rgba)?;
    defer alloc.free(encoded);

    let img = decodeWithMaxDimensions(alloc, encoded, 1024, 1024)?;
    defer img.deinit(alloc);

    // 亦可直接通过 cap.png 动态挂载调用
    let img2 = cap.png.decode(alloc, encoded)?;
    defer img2.deinit(alloc);
}
```
