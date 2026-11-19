# CP 标准库：全栈原生图像编解码与处理管线手册 (`cap:image`)

## 一、 架构总览与设计准则

`cap:image` 是 CP 语言官方内置的纯原生图像处理标准库，全面吸收自 Bun 图像处理子系统（`/data/dev/bun/src/runtime/image/`），集 **魔数格式探测、多格式编解码（PNG/BMP/GIF/JPEG/WebP）、EXIF 方向解析、调色板色彩量化、ThumbHash 极速占位图生成与流式几何变换管线** 于一体，专为 CapSurf 截图渲染、AI 视觉预处理与边缘媒体服务打造。

```text
┌────────────────────────────────────────────────────────────────────────┐
│                      高层流式图像管线 (Image)                           │
│   Image.load / flop / flip / rotate / crop / resize / toPng / toBmp    │
├──────────────────────┬───────────────────────┬─────────────────────────┤
│   完整像素编解码器    │    极速元数据探测器    │    视觉增强与压缩算法    │
│  Png (RGBA/全滤镜)   │  Codecs.sniff / mime  │  Quantizer (调色板量化) │
│  Bmp (24/32bpp DIB)  │  Jpeg.probe (SOF0/2)  │  ThumbHash (25B LQIP)   │
│  Gif (LZW + 交织)    │  Webp.probe (VP8/L/X) │  Exif.readJpeg / Tiff   │
├──────────────────────┴───────────────────────┴─────────────────────────┤
│          底层依赖: cap:pack (Crc32 / Zlib) + cap:fs (文件 I/O)          │
└────────────────────────────────────────────────────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 上游谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/image/index.cp` | `// Source: /data/dev/bun/src/runtime/image/Image.cp` | `cap:image` 统一公开导出入口 |
| `lib/image/codecs.cp` | `// Source: /data/dev/bun/src/runtime/image/codecs.cp` | `ImageFormat`、`ExifOrientation`、`Codecs`（魔数嗅探、MIME、扩展名与方向矩阵） |
| `lib/image/png.cp` | `// Source: /data/dev/bun/src/runtime/image/codec_png.cp` | `Png`、`PngImage`、`RgbaColor`（RFC 2083 全滤镜解码与 Zlib+CRC32 编码） |
| `lib/image/bmp.cp` | `// Source: /data/dev/bun/src/runtime/image/codec_bmp.cp` | `Bmp`、`BmpHeader`（Windows DIB 24/32bpp、Top-Down/Bottom-Up 编解码） |
| `lib/image/gif.cp` | `// Source: /data/dev/bun/src/runtime/image/codec_gif.cp` | `Gif`、`GifHeader`、`GifImage`（GIF87a/89a 探测、4096 节点 LZW 解码与 4-pass 交织） |
| `lib/image/jpeg.cp` | `// Source: /data/dev/bun/src/runtime/image/codec_jpeg.cp` | `Jpeg`、`JpegHeader`（Baseline SOF0 / Progressive SOF2 尺寸与 EXIF 联合探测） |
| `lib/image/webp.cp` | `// Source: /data/dev/bun/src/runtime/image/codec_webp.cp` | `Webp`、`WebpHeader`（RIFF 容器 VP8/VP8L/VP8X 尺寸、Alpha 与动画标志探测） |
| `lib/image/exif.cp` | `// Source: /data/dev/bun/src/runtime/image/exif.cp` | `Exif`（JPEG APP1 段落扫描与大小端 TIFF IFD0 `0x0112` Orientation 解析） |
| `lib/image/quantize.cp` | `// Source: /data/dev/bun/src/runtime/image/quantize.cp` | `Quantizer`、`QuantizeResult`（RGBA8 降维至 256 色以内紧凑调色板与索引图） |
| `lib/image/thumbhash.cp` | `// Source: /data/dev/bun/src/runtime/image/thumbhash.cp` | `ThumbHash`、`DecodedThumbImage`（25 字节微缩占位哈希编码、还原与 Data URL） |
| `lib/image/pipeline.cp` | `// Source: /data/dev/bun/src/runtime/image/Image.cp` | `Image` 统一 RGBA8 处理管线（镜像、旋转、裁切、缩放与导出） |

---

## 二、 核心 API 详解

### 1. 格式嗅探与元信息 (`Codecs` / `Exif` / `Jpeg` / `Webp`)

| API 签名 | 说明 |
| :--- | :--- |
| `Codecs.sniff(bytes): ImageFormat?` | 通过魔数识别 PNG、JPEG、WebP、BMP、GIF、TIFF、AVIF、HEIC |
| `Codecs.mime(fmt): string` | 返回对应标准 MIME 字符串（如 `"image/png"`、`"image/webp"`） |
| `Codecs.fromExtension(path): ImageFormat?` | 根据文件后缀推导图像格式 |
| `Codecs.getOrientationTransform(ori): ExifTransform` | 将 RFC EXIF 1~8 方向码映射为 `{ flop, flip, rotate }` 几何变换 |
| `Exif.readJpeg(bytes): ExifOrientation` | 解析 JPEG APP1 中的 EXIF 方向标号（无标签时返回 `.Normal`） |
| `Exif.parseTiff(tiff): ExifOrientation?` | 直接解析大端 (`MM`) 或小端 (`II`) TIFF IFD0 目录中的 `0x0112` 标签 |
| `Jpeg.probe(bytes): JpegHeader?` | 零解码极速获取 JPEG 宽高、色彩分量数、是否渐进式及 EXIF 方向 |
| `Webp.probe(bytes): WebpHeader?` | 零解码极速获取 WebP VP8/VP8L/VP8X 宽高、Alpha、无损与动图标志 |

### 2. 无损与索引像素编解码 (`Png` / `Bmp` / `Gif`)

| API 签名 | 说明 |
| :--- | :--- |
| `Png.encodeRgba(rgba, width, height): []u8` | 将 RGBA8 像素编码为完整 PNG 字节流（IHDR + Zlib IDAT + IEND + CRC32） |
| `Png.decode(bytes): PngImage?` | 解码 PNG（支持 RGBA/RGB/Grayscale/GrayscaleAlpha 与 5 种 RFC 2083 滤波器） |
| `Png.save(path, rgba, width, height): bool` / `Png.load(path): PngImage?` | 文件系统直存/直读 PNG |
| `png_img.getPixel(x, y): RgbaColor` / `setPixel(x, y, r, g, b, a)` | 像素级安全读写 |
| `Bmp.parseHeader(b)` / `Bmp.decode(bytes)` / `Bmp.encodeRgba(rgba, w, h)` | Windows DIB 24/32bpp BMP 解析、解码与 32bpp 编码 |
| `Gif.probe(bytes): GifHeader?` / `Gif.decode(bytes): GifImage?` | GIF87a/89a 探测与 LZW 首帧 RGBA8 解码（支持透明色索引与交织） |

### 3. 图像处理管线与视觉压缩 (`Image` / `Quantizer` / `ThumbHash`)

| API 签名 | 说明 |
| :--- | :--- |
| `Image.init(width, height, rgba)` / `Image.load(path)` | 构造或从磁盘加载 RGBA8 图像对象 |
| `img.flop()` / `img.flip()` / `img.rotate(90 \| 180 \| 270)` | 水平镜像、垂直翻转与顺时针 90/180/270 度旋转 |
| `img.crop(x, y, crop_w, crop_h)` / `img.resize(target_w, target_h)` | 边界安全的区域裁切与图像缩放 |
| `img.thumbhash()` / `img.toPng()` / `img.toBmp()` | 一键生成 ThumbHash、导出 PNG 或 BMP 字节流 |
| `Quantizer.quantize(rgba, w, h, max_colors): QuantizeResult` | 生成不超过 `max_colors`（最大 256）色的紧凑调色板与索引图 |
| `ThumbHash.encode(rgba, w, h)` / `decode(hash)` / `toDataUrl(hash)` | 生成/还原 25 字节微缩占位图（LQIP） |

---

## 三、 可运行代码示例

```typescript
import {
    Codecs, ImageFormat, Png, Bmp, Quantizer, ThumbHash, Image,
} from "cap:image";

pub fn demoImagePipeline() {
    // 1. 构建 2x2 RGBA8 像素画布
    let ptr: [*]u8 = @alloc(16);
    let rgba = ptr[0..16];
    rgba[0] = 255; rgba[1] = 0;   rgba[2] = 0;   rgba[3] = 255; // 红
    rgba[4] = 0;   rgba[5] = 255; rgba[6] = 0;   rgba[7] = 255; // 绿
    rgba[8] = 0;   rgba[9] = 0;   rgba[10] = 255; rgba[11] = 255; // 蓝
    rgba[12] = 255; rgba[13] = 255; rgba[14] = 0; rgba[15] = 255; // 黄

    // 2. 流式几何变换并导出 PNG / BMP
    let img = Image.init(2, 2, rgba);
    let transformed = img.rotate(90).flop().resize(4, 4);
    let png_bytes = transformed.toPng();
    let fmt = Codecs.sniff(png_bytes).?; // ImageFormat.Png
    let mime = Codecs.mime(fmt);         // "image/png"
    _ = mime;

    // 3. PNG 无损回环解码与像素校验
    let decoded = Png.decode(png_bytes).?;
    const px = decoded.getPixel(0, 0);
    _ = px;

    // 4. 色彩量化与 ThumbHash 微缩占位图
    const q = Quantizer.quantize(rgba, 2, 2, 16);
    const hash = img.thumbhash();
    const preview = ThumbHash.decode(hash).?;
    _ = q;
    _ = preview;
}
```

---

## 四、 与 Bun `src/runtime/image/` 对照表

| 功能分类 | Bun (`src/runtime/image/`) | CP 标准库 (`cap:image`) |
| :--- | :--- | :--- |
| **格式嗅探与 MIME** | `codecs.cp` (`sniff`, `mime`) | `Codecs.sniff(bytes)` / `Codecs.mime(fmt)` / `fromExtension(path)` |
| **PNG 编解码** | `codec_png.cp` | `Png.encodeRgba` / `Png.decode` / `Png.save` / `Png.load` |
| **BMP 编解码** | `codec_bmp.cp` | `Bmp.parseHeader` / `Bmp.decode` / `Bmp.encodeRgba` |
| **GIF 探测与 LZW 解码** | `codec_gif.cp` | `Gif.probe` / `Gif.decode` |
| **JPEG / WebP 探测** | `codec_jpeg.cp` / `codec_webp.cp` | `Jpeg.probe` / `Webp.probe` |
| **EXIF 方向解析** | `exif.cp` (`readJpeg`, `parseTiff`) | `Exif.readJpeg` / `Exif.parseTiff` / `Codecs.getOrientationTransform` |
| **调色板量化** | `quantize.cp` | `Quantizer.quantize(rgba, w, h, max_colors)` |
| **ThumbHash LQIP** | `thumbhash.cp` | `ThumbHash.encode` / `ThumbHash.decode` / `ThumbHash.toDataUrl` |
| **链式图像管线** | `Image.cp` (`flop`, `flip`, `rotate`, `crop`, `resize`) | `Image` (`flop`, `flip`, `rotate`, `crop`, `resize`, `toPng`, `toBmp`) |
