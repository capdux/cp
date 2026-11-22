# `cap:tiff` — 纯 CP Baseline TIFF 图像编解码库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:image`，导入请使用 `import { ... } from "cap:image"`。

> **模块路径**: `lib/tiff/`  
> **导入协议**: `import { TiffDocument, TiffPage, TiffEndian, TiffCompression, TiffPixelFormat, TiffError, encodeSingle, encodeMultiPage, decode, decodeWithLimits } from "cap:image";`  
> **动态挂载**: `import cap from "cap";` -> `cap.tiff.*` 编译期按需惰性挂载

---

## 一、 架构概述

`cap:tiff` 是 100% 纯 CP 实现、零外部 C 依赖（无 `std.posix` / `@import("bun")`）的 Baseline TIFF 6.0 多页图像编解码器，直接复用 `cap:lzw` 与 `cap:deflate` 提供无损压缩能力：

1. **双端序完整支持 (`TiffEndian`)**：
   - `Little`（`"II"` 小端序）与 `Big`（`"MM"` 大端序），支持 16/32 位 IFD 标签与内联 `SHORT`/`LONG` 值读写。
2. **多种无损压缩 (`TiffCompression`)**：
   - `None (1)`：无压缩原始像素。
   - `Lzw (5)`：复用 `cap:lzw`（`LzwBitOrder.MsbFirst`, `min_code_size = 8`, `max_code_size = 12`）。
   - `Deflate (8 / 32946)`：复用 `cap:deflate`（RFC 1950 ZLIB 容器 `compressZlib` / `decompressZlib`）。
   - `PackBits (32773)`：纯 CP Macintosh PackBits RLE 编解码。
3. **像素格式与多页/多条带 (`TiffPixelFormat`, `TiffPage`, `TiffDocument`)**：
   - 支持 `Gray8`（灰度 8-bit）、`Rgb24`（RGB 24-bit）与 `Rgba32`（RGBA 32-bit，含 `ExtraSamples` 标签）。
   - 支持多页 TIFF（IFD 链表）与多 Strip 条带拼接（兼容 `SHORT` 与 `LONG` 类型 `StripOffsets` / `StripByteCounts` 数组）。
   - 支持 `Predictor = 2`（Horizontal Differencing）水平差分像素还原。
4. **安全边界防护**：
   - 严格校验 `"II*\0"` / `"MM\0*"` 魔数、循环 IFD 链表检测、越界偏移拦截、尺寸/页数上限（`decodeWithLimits`）与截断数据检测。

---

## 二、 核心 API 速查表 (`lib/tiff/tiff.cp`)

| 符号 / 方法 | 签名与说明 |
| :--- | :--- |
| `TiffError` | `InvalidSignature`, `InvalidHeader`, `InvalidIfd`, `UnsupportedCompression`, `UnsupportedFormat`, `TruncatedData`, `ImageTooLarge`, `TooManyPages`, `OutOfMemory` |
| `TiffEndian` | `Little` (`"II"`), `Big` (`"MM"`) |
| `TiffCompression` | `None = 1`, `Lzw = 5`, `Deflate = 8`, `PackBits = 32773`（解码同时兼容旧版 `32946`） |
| `TiffPixelFormat` | `Gray8`, `Rgb24`, `Rgba32` |
| `TiffPage` | `{ width: u32, height: u32, format: TiffPixelFormat, compression: TiffCompression, pixels: []u8 }`，含 `deinit(allocator)` |
| `TiffDocument` | `{ pages: []TiffPage }`，含 `deinit(allocator)` |
| `encodeSingle(allocator, width, height, format, compression, endian, pixels)` | 编码单页 TIFF，返回 `TiffError![]u8`（调用方负责 `allocator.free`） |
| `encodeMultiPage(allocator, pages, endian)` | 编码多页 TIFF，返回 `TiffError![]u8`（调用方负责 `allocator.free`） |
| `decode(allocator, tiff_bytes)` | 使用默认安全上限（65535x65535, 1024 页）解码 TIFF，返回 `TiffError!TiffDocument` |
| `decodeWithLimits(allocator, tiff_bytes, max_w, max_h, max_pages)` | 带显式宽高与页数上限的安全解码 |

---

## 三、 使用示例

```typescript
const std = @import("std");
import {
    TiffEndian,
    TiffCompression,
    TiffPixelFormat,
    encodeSingle,
    decode,
} from "cap:image";

fn demo(alloc: std.mem.Allocator): !void {
    const rgb: [12]u8 = [255, 0, 0, 0, 255, 0, 0, 0, 255, 255, 255, 0];
    const encoded = encodeSingle(
        alloc,
        2,
        2,
        TiffPixelFormat.Rgb24,
        TiffCompression.Deflate,
        TiffEndian.Big,
        rgb[0..12],
    )?;
    defer alloc.free(encoded);

    let doc = decode(alloc, encoded)?;
    defer doc.deinit(alloc);
}
```
