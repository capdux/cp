# `cap:css_background` — 纯 CP CSS Background 属性族与多图层简写模型

## 概述

`cap:css_background` 吸收自 Bun `src/css/properties/background.cp`，纯 CP 零外部依赖、零堆分配，完整实现 CSS Backgrounds and Borders Level 3/4 全部子属性与多图层简写解析、默认值判定和规范化 `toCss` 序列化。

## 核心类型与 API

- `CssBackgroundError`：`InvalidRepeat` / `InvalidSize` / `InvalidPosition` / `InvalidAttachment` / `InvalidOrigin` / `InvalidClip` / `InvalidBackground` / `TooManyLayers` / `BufferTooSmall`
- `BackgroundRepeatKeyword`（`repeat`/`space`/`round`/`no_repeat`）与 `BackgroundRepeat`（支持 `repeat-x`/`repeat-y` 及双轴归一化）
- `LengthPercentageAutoKind` / `LengthPercentageAuto`
- `BackgroundSizeKind` / `BackgroundSize`（`cover` / `contain` / 宽高显式值）
- `BackgroundAttachment`（`scroll` / `fixed` / `local`）
- `BackgroundOrigin`（`padding-box` / `border-box` / `content-box`）
- `BackgroundClip`（`border-box` / `padding-box` / `content-box` / `text` / `border-area`）
- `BackgroundPosition`（支持关键字与长度/百分比双轴定位）
- `BackgroundLayer` 与 `Background`（支持 `position / size` 斜杠语法、单/双 `<box>` origin/clip 规则、逗号分隔多图层与颜色仅限末层约束）

## 示例

```cp
import { Background } from "cap:css_background";

const bg = Background.parse("url(a.png) center / cover no-repeat, #112233 linear-gradient(red, blue)")?;
```
