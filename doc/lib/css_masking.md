# `cap:css_masking` — 纯 CP CSS 遮罩与裁剪属性模型

## 概述

`cap:css_masking` 吸收自 Bun `src/css/properties/masking.cp`，纯 CP 零外部依赖，完整覆盖 CSS Masking Level 1 核心属性枚举、大小写无关解析、CSS 序列化、WebKit 兼容映射、特性位标志以及聚合配置结构。

## 核心类型与 API

### 几何与遮罩枚举（均提供 `parse(input)` / `toCss()` / `default()`）

- `GeometryBox`：`border-box` / `padding-box` / `content-box` / `margin-box` / `fill-box` / `stroke-box` / `view-box`（附 `isSvgBox()` / `isCssBox()` / `intoMaskClip()`）
- `MaskOrigin`：`border-box` / `padding-box` / `content-box` / `fill-box` / `stroke-box` / `view-box`（附 `toGeometryBox()`）
- `MaskClip`：`<geometry-box> | no-clip`（附 `isClipped()`）
- `MaskMode`：`luminance` / `alpha` / `match-source`（附 `toWebKitSourceType()`）
- `MaskType`：`luminance` / `alpha`
- `MaskComposite`：`add` / `subtract` / `intersect` / `exclude`（附 `toWebKit()`）
- `WebKitMaskComposite`：`clear` / `copy` / `source-over` / `source-in` / `source-out` / `source-atop` / `destination-over` / `destination-in` / `destination-out` / `destination-atop` / `xor`（附 `toStandard()`）
- `WebKitMaskSourceType`：`auto` / `luminance` / `alpha`（附 `toStandard()`）
- `MaskBorderMode`：`luminance` / `alpha`
- `MaskBorderRepeat`：`stretch` / `repeat` / `round` / `space`
- `FillRule`：`nonzero` / `evenodd`

### 位标志、配置与 WebKit 兼容映射

- `MaskFeatureFlags` + `MASK_FLAG_*` 位常量：`empty()` / `isEmpty()` / `has(flag)` / `add(flag)` / `remove(flag)` / `needsWebKitFallback()`
- `MaskConfig` / `MaskBorderConfig`：`default()` / `isDefault()`
- `getWebKitMaskPropertyName(standard_prop: string): string?`

## 示例

```cp
import {
    GeometryBox,
    MaskMode,
    MaskComposite,
    getWebKitMaskPropertyName,
} from "cap:css_masking";

const box = GeometryBox.parse("view-box").?; // box.isSvgBox() == true
const wk = MaskComposite.intersect.toWebKit(); // WebKitMaskComposite.source_in
const prop = getWebKitMaskPropertyName("mask-border-slice").?; // "-webkit-mask-box-image-slice"
```
