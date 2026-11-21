# `cap:exif` — JPEG/TIFF EXIF 方向元数据读取器

源自 Bun `runtime/image/exif.cp`，纯 CP 零依赖实现 JPEG APP1/Exif 与 TIFF IFD0 `0x0112`（Orientation）标签解析，以及将存储像素校正为正立方向所需的几何变换参数。

## 导出符号

- `Orientation`: 枚举（`normal=1`..`rotate270=8`），提供 `transform(): Transform`、`toU8(): u8`、`swapsDimensions(): bool`、`Orientation.fromU16(v: u16): Orientation?`
- `Transform`: `{ flop: bool, flip: bool, rotate: u16 }`，提供 `isIdentity(): bool`、`swapsDimensions(): bool`
- `readJpeg(bytes: []const u8): Orientation`: 遍历 JPEG 标记至 SOS/EOI 查找 APP1/Exif，畸形输入宽容返回 `.normal`
- `parseTiff(tiff: []const u8): Orientation?`: 直接解析小端（`II`）或大端（`MM`）TIFF 头与 IFD0 Orientation 标签
