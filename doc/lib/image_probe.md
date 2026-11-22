# `cap:image_probe` — 零解码极速图片头探测器

> 参考来源：/data/dev/bun/src/runtime/image/codec_png.cp, /data/dev/bun/src/runtime/image/codec_jpeg.cp, /data/dev/bun/src/runtime/image/codec_webp.cp, /data/dev/bun/src/runtime/image/codec_bmp.cp | 吸收日期：2026-11-22

## 概述

`cap:image_probe` 吸收自 Bun 图像子系统各格式编解码器的头部规范定义，并彻底剥离 `libspng` / `libjpeg` / `libwebp` 等 C 库依赖，提供 100% 纯 CP、零堆分配、零像素解码的图片头部元数据探测器，支持 **PNG / JPEG / WebP (VP8/VP8L/VP8X) / BMP / GIF (87a/89a)**。

## 行为契约与安全防御

- **微秒级零解码探测**：仅读取输入头部数十字节即可精确获取格式、宽高、色深、色彩类型、Alpha 通道、动画标志、渐进式/隔行标志及 BMP 扫描方向，非常适合 HTTP 上传预检、媒体库索引与防超大尺寸图像资源耗尽拦截。
- **格式专项解析规则**：
  - **PNG**：校验 8 字节魔数与 `IHDR` 块（宽、高、色深、色彩类型、Adam7 隔行标志），自动识别带 Alpha 的色彩类型（4 / 6）；
  - **JPEG**：跳过 `APPn` / `COM` 等任意元数据段，定位 `SOF0`（基线）/`SOF2`（渐进式）等帧头提取精确宽高与色深；
  - **WebP**：解析 `RIFF....WEBP` 容器内的 `VP8 `（有损）、`VP8L`（无损，含 Alpha 标志）与 `VP8X`（扩展头，含 24-bit 宽高、Alpha 与动画标志）；
  - **BMP**：支持 `BITMAPINFOHEADER` 及 V4/V5 头，正确区分正高度（bottom-up）与负高度（top-down，设 `top_down = true`）；
  - **GIF**：解析 `GIF87a` / `GIF89a` 逻辑屏幕描述符获取宽高。
- **畸形输入防御**：严格校验截断头（`TruncatedHeader`）、零宽或零高（`InvalidHeader`）、非标准 FourCC（`UnsupportedVariant`）以及 BMP 负高度等于 `i32::MIN`（`-2147483648`）时取绝对值的整数溢出防护。

---

## 快速导入

```cp
import {
    ImageFormat,
    WebpKind,
    ImageInfo,
    Error,
    detectFormat,
    probe,
    probePng,
    probeJpeg,
    probeWebp,
    probeBmp,
    probeGif,
} from "cap:image_probe";
```

---

## API 参考

### 1. 类型与错误集

| 符号 | 说明 |
| :--- | :--- |
| `Error` | `UnknownFormat`, `TruncatedHeader`, `InvalidHeader`, `UnsupportedVariant` |
| `ImageFormat` | `png`, `jpeg`, `webp`, `bmp`, `gif`（提供 `mimeType()`、`extension()`） |
| `WebpKind` | `none`, `vp8`, `vp8l`, `vp8x` |
| `ImageInfo` | `{ format, width, height, bit_depth, color_type, has_alpha, is_animated, is_progressive, top_down, webp_kind }` + `pixelCount()`、`mimeType()` |

### 2. 探测函数

| 函数 | 签名 | 说明 |
| :--- | :--- | :--- |
| `detectFormat` | `pub fn detectFormat(bytes: []const u8): ImageFormat?` | 仅凭魔数快速检测图片格式 |
| `probe` | `pub fn probe(bytes: []const u8): ImageInfo!` | 自动识别格式并探测头部元数据 |
| `probePng` | `pub fn probePng(bytes: []const u8): ImageInfo!` | 解析 PNG IHDR（宽高/色深/色彩类型/Alpha/Adam7） |
| `probeJpeg` | `pub fn probeJpeg(bytes: []const u8): ImageInfo!` | 扫描跳过 APP 段并解析 SOF0/SOF2 帧头 |
| `probeWebp` | `pub fn probeWebp(bytes: []const u8): ImageInfo!` | 解析 RIFF WEBP 的 VP8 / VP8L / VP8X 头 |
| `probeBmp` | `pub fn probeBmp(bytes: []const u8): ImageInfo!` | 解析 BITMAPINFOHEADER / V4+ 正负高度与 24/32bpp |
| `probeGif` | `pub fn probeGif(bytes: []const u8): ImageInfo!` | 解析 GIF87a / GIF89a 逻辑屏幕描述符 |

---

## 使用示例

```cp
import { probe } from "cap:image_probe";

fn inspectImage(header_bytes: []const u8)! {
    const info = probe(header_bytes)?;
    const mime = info.mimeType();
    const pixels = info.pixelCount(); // u64，避免 width * height 32 位溢出
    * = mime;
    * = pixels;
}
```
