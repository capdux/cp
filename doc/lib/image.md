# CP 标准库：全栈原生图像编解码与处理管线手册 (`cap:image`)

> 参考来源：`/data/dev/bun/src/runtime/image/Image.cp`、`codecs.cp`、`exif.cp`、`thumbhash.cp` | 吸收日期：2026-11-22

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
| `ImageFormat.detect(bytes): ImageFormat?` / `Codecs.sniff(bytes)` | 通过魔数/XML 嗅探自动识别 PNG、JPEG、GIF、WebP、BMP、ICO、SVG、TIFF、AVIF、HEIC |
| `probeDimensions(bytes): ImageDimensions?` / `Codecs.probeDimensions(bytes)` | **零像素全量解码**，极速读取 PNG、JPEG (全部 SOF 帧)、GIF、WebP (VP8/VP8L/VP8X)、BMP、ICO、SVG 宽高 |
| `Codecs.mime(fmt): string` | 返回对应标准 MIME 字符串（含 `"image/x-icon"`、`"image/svg+xml"` 等） |
| `Codecs.fromExtension(path): ImageFormat?` | 根据文件后缀（大小写无关）推导图像格式 |
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
| `Image.init(width, height, rgba)` / `Image.decode(bytes)` / `Image.load(path)` | 构造或自动嗅探解码 PNG/BMP/GIF 图像对象 |
| `img.flop()` / `img.flip()` / `img.rotate(90 \| 180 \| 270)` / `applyExifOrientation(ori)` | 水平镜像、垂直翻转、顺时针旋转与 EXIF 自动方向矫正 |
| `img.crop(x, y, crop_w, crop_h)` / `img.resize(target_w, target_h)` / `resizeNearest(...)` / `thumbnail(max_w, max_h)` | 边界安全裁切、双线性/最邻近缩放与等比例缩略图生成 |
| `img.grayscale()` / `invert()` / `brightness(delta)` / `sepia()` / `blur()` | 灰度（BT.601）、反色、亮度调节、复古棕褐色与 3x3 均值模糊滤镜 |
| `img.thumbhash()` / `toPng()` / `toBmp()` / `savePng(path)` / `saveBmp(path)` | 一键生成 ThumbHash、导出字节流或持久化写入磁盘 |
| `Quantizer.quantize(rgba, w, h, max_colors): QuantizeResult` | 生成不超过 `max_colors`（最大 256）色的紧凑调色板与索引图 |
| `ThumbHash.encode(rgba, w, h)` / `decode(hash)` / `toDataUrl(hash)` | 生成/还原 25 字节空间网格 ThumbHash 及完整合法 `data:image/png;base64,...` URL |

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

---

## 五、 上游行为契约、边界防御与性能注意事项

1. **零像素全量解码的元数据探测（防解压内存耗尽）**：
   - 在解码外部不可信图片之前，强烈建议先调用 `Codecs.sniff(bytes)` + `probeDimensions(bytes)`（或 `Jpeg.probe` / `Webp.probe` / `Gif.probe`）校验宽高上限，仅解析文件头而不分配全量 RGBA8 帧缓冲，避免畸形大尺寸图片触发资源耗尽。
2. **EXIF 方向自动矫正顺序**：
   - `Exif.readJpeg` 自动扫描 JPEG `APP1` 段落并兼容大端（`MM`）与小端（`II`）TIFF IFD0 `0x0112` 标签；
   - 对相机直出图片应使用 `img.applyExifOrientation(ori)`（内部遵循 `Codecs.getOrientationTransform` 的 `flop`/`flip`/`rotate` 精确组合顺序），避免手动二次旋转导致镜像错乱。
3. **色彩量化与 ThumbHash 边界**：
   - `Quantizer.quantize` 将 `max_colors` 限制在 256 色以内以适配索引调色板格式；
   - `ThumbHash.encode` 适合生成约 25 字节的极低开销占位图（LQIP），并通过 `ThumbHash.toDataUrl` 输出内联 `data:image/png;base64,...` 供前端首屏即时渲染。

---

## 六、 扩展图像、矢量绘图与条码格式（Pure-CP 零冲突归并）

以下 15 个纯 CP 图像/容器/矢量/条码格式模块已统一收敛至 `lib/image/` 并零冲突导出至 `cap:image`：

| 子模块文件 | 领域分类 | 核心导出符号 |
| :--- | :--- | :--- |
| `avif.cp` | AVIF / MIAF 容器解析 | `isAvif`, `parseAvifBrand`, `parseAv1C`, `parseAvifInfo`, `AvifInfo`, `Av1Config` |
| `exr.cp` | OpenEXR HDR 头部解析 | `isExr`, `parseExrHeader`, `ExrAttrIter`, `ExrHeader`, `ExrCompression`, `ExrBox2i` |
| `heif.cp` | HEIF / HEIC 容器解析 | `isHeif`, `parseHeifBrand`, `parseHvcC`, `parseHeifInfo`, `HeifInfo`, `HevcConfig` |
| `ico.cp` | Windows ICO / CUR 容器 | `isIcoOrCur`, `parseIcoHeader`, `IcoEntryIter`, `findBestIcoEntry`, `IcoKind`, `IcoHeader` |
| `pnm.cp` | Netpbm P1–P6 编解码 | `parsePnmHeader`, `pnmRawPixels`, `encodePpmP6`, `encodePgmP5`, `PnmFormat`, `PnmHeader` |
| `psd.cp` | Adobe PSD / PSB 解析 | `isPsd`, `parsePsdHeader`, `parsePsdSections`, `PsdResourceIter`, `findPsdResource`, `PsdHeader` |
| `qoi.cp` | QOI 无损图像编解码 | `isQoi`, `parseQoiHeader`, `encodeQoi`, `decodeQoi`, `QoiHeader` |
| `tga.cp` | Truevision TGA 编解码 | `parseTgaHeader`, `encodeTgaTrueColor`, `decodeTgaToRgba`, `TgaImageType`, `TgaHeader` |
| `tiff.cp` | Baseline / Multi-Page TIFF | `encodeSingle`, `encodeMultiPage`, `decode`, `decodeWithLimits`, `TiffDocument`, `TiffPage` |
| `dicom.cp` | DICOM PS3.10 医学影像 | `isDicom`, `parseDicomSummary`, `DicomIter`, `DicomTag`, `DicomElement`, `DicomSummary` |
| `svgCanvas.cp` / `svgCharts.cp` | SVG 1.1 画布与统计图表 | `SvgCanvas`, `SvgPathBuilder`, `SvgStyle`, `SvgCharts`, `ChartDatum`, `LineSeries` |
| `barcode.cp` | 1D 条码 (Code128/EAN13/UPCA) | `BarcodeEncoder`, `BarcodeRenderer`, `BarcodeFormat`, `BarcodeBitPattern`, `BarcodeError` |
| `qrcodeEcc.cp` / `qrcodeEncoder.cp` / `qrcodeRender.cp` | ISO/IEC 18004 二维码 | `QrCode`, `QrMatrix`, `QrRenderer`, `QrEccLevel`, `ReedSolomon`, `QrError` |
| `jxl.cp` | JPEG XL 码流与 ISOBMFF 容器 | `JxlContainerKind`, `JxlSizeHeader`, `JxlBox`, `JxlInfo`, `isJxl`, `detectJxlKind`, `parseJxlSizeHeader`, `JxlBoxIter`, `parseJxlInfo` |
| `xcf.cp` | GIMP XCF 多图层图像解析 | `XCF_MAGIC`, `XcfBaseType`, `XcfCompression`, `XcfPrecision`, `XcfHeader`, `XcfLayerInfo`, `XcfPropIter`, `XcfPointerIter`, `isXcf`, `parseXcfHeader`, `parseXcfLayerHeader` |

> **重叠参考归档说明**：`bmp_.cp`、`gif_.cp`、`png_.cp`、`exif_.cp`、`thumbhash_.cp`、`image_probe_.cp` 为与 Bun 权威实现功能重叠的对照参考副本（文件首行带 `// Reference Only:` 声明），不在 `lib/image/index.cp` 冲突导出。

---

## 收敛子模块深度文档

- [avif.md](image/avif.md)
- [barcode.md](image/barcode.md)
- [dicom.md](image/dicom.md)
- [exr.md](image/exr.md)
- [heif.md](image/heif.md)
- [ico.md](image/ico.md)
- [jxl.md](image/jxl.md)
- [pnm.md](image/pnm.md)
- [psd.md](image/psd.md)
- [qoi.md](image/qoi.md)
- [qrcode.md](image/qrcode.md)
- [svg.md](image/svg.md)
- [tga.md](image/tga.md)
- [tiff.md](image/tiff.md)
- [xcf.md](image/xcf.md)
