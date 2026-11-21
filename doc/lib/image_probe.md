# `cap:image_probe` — 零解码极速图片头探测器

> 溯源：从 `/data/dev/bun/src/runtime/image/{codec_png,codec_jpeg,codec_webp,codec_bmp}.cp` 吸收并规范化为纯 CP 标准库模块。

`cap:image_probe` 提供零外部 C 库、零堆分配、零像素解码的图片头部元数据探测器，支持 **PNG / JPEG / WebP (VP8/VP8L/VP8X) / BMP / GIF (87a/89a)**：

- 仅读取头部数十字节即可精确获取格式、宽高、色深、色彩类型、Alpha 通道、动画标志、渐进式/隔行标志及 BMP 扫描方向；
- 严格校验截断头、零宽高、非标准 FourCC、`i32::MIN` BMP 负高度溢出等异常边界。

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
    const pixels = info.pixelCount();
    * = mime;
    * = pixels;
}
```
