# CP 标准库：BMP 位图编解码器 (`cap:bmp` / `cap.bmp`)

## 一、 架构总览与设计准则

`cap:bmp` 是 CP 语言官方内置的纯原生 BMP（Windows Bitmap）位图编解码标准库，支持 24-bit BGR、32-bit BGRA 无损编码与解码，以及 8-bit 调色板索引位图与 `top-down`（负高度）/ `bottom-up`（正高度）行序的统一归一化解码。

全模块**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，仅使用纯 CP 与 `std` 基础库，支持 `import ... from "cap:bmp"` 与 `import cap from "cap"; cap.bmp.*` 编译期自动惰性挂载。

### 模块文件结构

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/bmp/bmp.cp` | `// Source: cp/lib/bmp/bmp.cp (Pure CP BMP Bitmap Image Codec: 24/32-bit BGR/BGRA & 8-bit Palette)` | `BmpError`、`BmpPixelFormat`、`BmpImage`、`encode24`、`encode32`、`decode`、`decodeWithMaxDimensions` |
| `lib/bmp/index.cp` | `// Source: cp/lib/bmp/index.cp (Public Unified Export for cap:bmp & cap.bmp)` | `cap:bmp` 与 `cap.bmp` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集与像素格式

- **`BmpError`**：
  - `InvalidSignature`：文件头缺少 `"BM"` 签名
  - `InvalidHeader`：DIB 头大小 `< 40`、`planes != 1`、像素偏移越界或调色板索引非法
  - `UnsupportedBitDepth`：不支持的色深（仅支持 `8`、`24`、`32` 位）
  - `UnsupportedCompression`：不支持的压缩方式（仅支持未压缩 `BI_RGB = 0`，或 32-bit `BI_BITFIELDS = 3`）
  - `TruncatedData`：文件头、调色板或像素数据区截断
  - `ImageTooLarge`：图像宽高超过 `max_w` / `max_h` 上限或总字节数溢出
  - `InvalidDimensions`：宽度 `<= 0`、高度 `== 0`、高度为 `minInt(i32)` 或编码输入切片长度不匹配
  - `OutOfMemory`：分配器内存不足

- **`BmpPixelFormat` (`u8`)**：
  - `Rgb24 = 3`：24-bit Top-Down RGB（每像素 3 字节）
  - `Rgba32 = 4`：32-bit Top-Down RGBA（每像素 4 字节）

### 2. `BmpImage` 容器

- 字段：`width: u32`、`height: u32`、`format: BmpPixelFormat`、`pixels: []u8`（统一按 Top-Down RGB/RGBA 字节序存储）
- **`bytesPerPixel(): usize`**：返回当前格式每像素字节数（`3` 或 `4`）
- **`deinit(*_, allocator: std.mem.Allocator)`**：释放 `pixels` 像素缓冲

### 3. 编码与解码函数

- **`encode24(allocator: std.mem.Allocator, width: u32, height: u32, rgb_pixels: []const u8): ([]u8)!BmpError`**
  - 写入标准 14 字节 `BITMAPFILEHEADER`（`"BM"`）+ 40 字节 `BITMAPINFOHEADER`，将 Top-Down RGB 转为 Bottom-Up BGR，自动按 4 字节行步长补零对齐并校验输入切片长度。
- **`encode32(allocator: std.mem.Allocator, width: u32, height: u32, rgba_pixels: []const u8): ([]u8)!BmpError`**
  - 写入标准 54 字节文件头与信息头，将 Top-Down RGBA 转为 Bottom-Up BGRA 并校验输入切片长度。
- **`decode(allocator: std.mem.Allocator, bmp_bytes: []const u8): BmpImage!BmpError`**
  - 使用默认最大维度上限（`65535 x 65535`）解码 24-bit、32-bit 或 8-bit 调色板 BMP 为统一 Top-Down `BmpImage`。
- **`decodeWithMaxDimensions(allocator: std.mem.Allocator, bmp_bytes: []const u8, max_w: u32, max_h: u32): BmpImage!BmpError`**
  - 带安全尺寸上限校验的解码入口，防止超大畸形图像导致内存耗尽。

---

## 三、 使用示例

```typescript
const std = @import("std");
import cap from "cap";
import { BmpImage, BmpPixelFormat, encode24, encode32, decode, decodeWithMaxDimensions } from "cap:bmp";

pub fn roundtripExample(alloc: std.mem.Allocator): !void {
    const rgb: [18]u8 = [
        255, 0, 0,   0, 255, 0,   0, 0, 255,
        10, 20, 30,  40, 50, 60,  70, 80, 90,
    ];
    const encoded = encode24(alloc, 3, 2, &rgb)?;
    defer alloc.free(encoded);

    let img = decode(alloc, encoded)?;
    defer img.deinit(alloc);
    std.debug.assert(img.format == BmpPixelFormat.Rgb24);
}
```
