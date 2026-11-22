# `cap:exif` — JPEG/TIFF EXIF 方向元数据读取器

> 参考来源：/data/dev/bun/src/runtime/image/exif.cp | 吸收日期：2026-11-22

## 概述

`cap:exif` 吸收自 Bun 图像处理子系统（`src/runtime/image/exif.cp`），纯 CP 零依赖、零堆分配实现 JPEG APP1/Exif 与 TIFF IFD0 `0x0112`（Orientation）标签解析，并计算将存储像素校正为正立方向所需的几何变换参数。

## 上游设计哲学与二进制布局契约

- **精准聚焦 Orientation（`0x0112`）**：正如 Bun 上游源码说明，EXIF 中唯一改变像素几何含义（也是图像管线 `autoOrient` 唯一需要）的字段就是 `Orientation` 标签（Tag `0x0112`，Type `3 = SHORT`，Count `1`）。由于其载荷仅占 2 字节，直接内联打包在 IFD 条目最后 4 字节的值字段前 2 字节中，**无需额外指针偏移追踪**，实现极速单遍读取；GPS、机型、时间戳等其余标签均被跳过。
- **二进制解析路径**：
  - JPEG：`FF D8`（SOI）→ 逐段跳过标记（自动兼容无长度字段的填充 `0xFF` 与重启标记 `0xD0..0xD8`），遇到 `0xDA`（SOS 扫描数据开始）或 `0xD9`（EOI）立即停止；
  - APP1/Exif：命中 `0xE1` 段且前 6 字节为 `"Exif\x00\x00"` 时，将后续切片交由 `parseTiff` 解析；
  - TIFF：校验字节序标记 `"II"`（小端）或 `"MM"`（大端）与魔数 `42`，跳转至 `IFD0` 偏移后遍历每个 12 字节条目寻找 `0x0112`。
- **变换组合顺序契约**：对于 `flop_rotate90`（EXIF 5 = transpose）与 `flop_rotate270`（EXIF 7 = transverse），`Transform` 严格约定**先执行水平镜像（`flop`），再执行顺时针旋转（`rotate`）**：
  - `flop_rotate90`（5）→ `flop: true, rotate: 270`（$\text{flop} \circ \text{rot}_{270} = \text{transpose}$）；
  - `flop_rotate270`（7）→ `flop: true, rotate: 90`（$\text{flop} \circ \text{rot}_{90} = \text{transverse}$）。
- **宽容无错策略（Advisory Metadata）**：EXIF 仅作为建议性显示方向元数据；任何截断、非法魔数、越界偏移、非 `SHORT/1` 类型或超出 `1..8` 的值均不会报错，而是宽容返回 `.normal`（`readJpeg`）或 `null`（`parseTiff`），绝不阻断正常图像解码。

## 导出符号与 API

### `Transform`

```cp
pub struct Transform {
    flop: bool,   // 是否先水平镜像
    flip: bool,   // 是否垂直镜像
    rotate: u16,  // 顺时针旋转角度（0 / 90 / 180 / 270）

    fn isIdentity(): bool
    fn swapsDimensions(): bool
}
```

### `Orientation`

对应 EXIF 规范 `1..8` 方向值：

- 枚举成员：`normal = 1`、`flop = 2`、`rotate180 = 3`、`flip = 4`、`flop_rotate90 = 5`、`rotate90 = 6`、`flop_rotate270 = 7`、`rotate270 = 8`
- 方法：
  - `transform(): Transform` — 返回将存储像素转为正立的几何变换（先 `flop`/`flip`，后顺时针 `rotate`）。
  - `toU8(): u8` — 返回标准 EXIF 数值 `1..8`。
  - `swapsDimensions(): bool` — 判断校正后宽高是否互换（值 5、6、7、8 返回 `true`）。
  - `static fn fromU16(v: u16): Orientation?` — 校验 `1..8` 范围并转换，越界返回 `null`。

### 独立解析函数

- `readJpeg(bytes: []const u8): Orientation` — 遍历 JPEG 标记至 SOS/EOI 查找 APP1/Exif，缺失或畸形时宽容返回 `.normal`。
- `parseTiff(tiff: []const u8): Orientation?` — 直接解析小端（`II`）或大端（`MM`）TIFF 头与 IFD0 Orientation 标签，缺失或畸形返回 `null`。

## 边界注意事项

1. **格式范围**：`readJpeg` 面向手机与相机拍摄旋转图的主流载体 JPEG；若需处理 PNG `eXIf` 或 WebP `EXIF` 块，可先提取其中原始 TIFF 载荷后直接调用 `parseTiff`。
2. **越界与整数安全**：所有 `rd16` / `rd32` 与段长检查均在读取前校验切片边界，畸形 `seglen < 2` 或超大 `ifd0` 偏移会立即安全退出，不会发生越界访问。

## 使用示例

```cp
import { Orientation, readJpeg, parseTiff } from "cap:exif";

fn inspectOrientation(jpeg_bytes: []const u8) {
    const orient = readJpeg(jpeg_bytes);
    const tf = orient.transform();
    if (!tf.isIdentity()) {
        // 按先 flop/flip 后顺时针 rotate 角度校正像素
        // 若 tf.swapsDimensions() == true，则输出图像宽高互换
        * = tf;
    }
    * = Orientation.fromU16(6); // Orientation.rotate90
    * = parseTiff(jpeg_bytes);  // 非 TIFF 数据安全返回 null
}
```
