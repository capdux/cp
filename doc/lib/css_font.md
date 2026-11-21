# `cap:css_font` — 纯 CP CSS Fonts Level 4 字体属性族与简写模型

## 概述

`cap:css_font` 吸收自 Bun `src/css/properties/font.cp`，纯 CP 零外部依赖、零堆分配，完整覆盖 CSS Fonts Level 4 全部子属性与 `font` 简写解析、默认值省略及规范化 `toCss` 序列化。

## 核心类型与 API

- `CssFontError`：`InvalidWeight` / `InvalidStyle` / `InvalidStretch` / `InvalidSize` / `InvalidLineHeight` / `InvalidVariantCaps` / `InvalidFamily` / `InvalidFontShorthand` / `TooManyFamilies` / `BufferTooSmall`
- `FontWeight`（`normal`=400 / `bold`=700 / `bolder` / `lighter` / `1..1000`）
- `FontStyle`（`normal` / `italic` / `oblique <angle>`，`-90deg..90deg`）
- `FontStretch`（`ultra-condensed`..`ultra-expanded` 或百分比）
- `AbsoluteFontSize` / `RelativeFontSize` / `FontSize`
- `LineHeight`（`normal` / 无单位倍率 / `px`/`rem`/`em`/`%`）
- `FontVariantCaps`（7 变体）
- `GenericFontFamily`（全部 13 种 CSS Fonts 4 通用字体族）
- `FontFamilyItem` 与 `Font` 简写（支持 `italic bold 16px/1.5 "Helvetica Neue", sans-serif` 等）

## 示例

```cp
import { Font } from "cap:css_font";

const f = Font.parse("italic bold 16px/1.5 \"Helvetica Neue\", sans-serif")?;
```
