# `cap:css_media` — 纯 CP CSS Media Queries Level 4 解析与环境求值引擎

## 概述

`cap:css_media` 吸收自 Bun `src/css/rules/media.cp`，纯 CP 零外部依赖、零堆分配，提供 CSS Media Queries Level 4 的媒体类型、限定符、视口与用户偏好特征解析、`MediaEnv` 环境求值、逗号分隔多查询列表 `MediaQueryList` 以及规范化 `toCss` 序列化。

## 核心类型与 API

- `CssMediaError`：`InvalidMediaType` / `InvalidMediaFeature` / `InvalidFeatureValue` / `InvalidMediaQuery` / `TooManyFeatures` / `TooManyQueries` / `BufferTooSmall`
- `MediaType`（`all`/`screen`/`print`）、`MediaQualifier`（`none`/`only`/`not_kw`）
- 特征值枚举：`ColorScheme`（`light`/`dark`）、`ReducedMotion`（`no_preference`/`reduce`）、`HoverCapability`（`none`/`hover`）、`PointerCapability`（`none`/`coarse`/`fine`）、`MediaOrientation`（`portrait`/`landscape`）
- `MediaEnv`：视口宽高、媒体类型与设备/偏好状态结构
- `MediaFeature`：`parse(input)`、`evaluate(env)`、`toCss(buf)`
- `MediaQuery`：`parse(input)`、`alwaysMatches()`、`neverMatches()`、`evaluate(env)`、`toCss(buf)`
- `MediaQueryList`：`parse(input)`、`alwaysMatches()`、`neverMatches()`、`evaluate(env)`、`toCss(buf)`

## 示例

```cp
import { MediaQueryList, MediaEnv, ColorScheme } from "cap:css_media";

const mq = MediaQueryList.parse("screen and (min-width: 768px) and (prefers-color-scheme: dark), print")?;
const dark_desktop = MediaEnv{ width: 1440.0, height: 900.0, color_scheme: ColorScheme.dark };
// mq.evaluate(dark_desktop) == true
```
