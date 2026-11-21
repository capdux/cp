# `cap:css_primitives` — CSS 基础值类型（Alpha / Percentage / NumberOrPercentage / URL 分类）

源自 Bun `css/values/{alpha,percentage,url}.cp`，纯 CP 零堆分配实现 CSS `<percentage>`、`<number> | <percentage>`、`<alpha-value>` 解析与格式化，以及 RFC 3986 CSS `url()` 绝对/相对/片段/Data URI 分类器。

## 导出符号

- `Percentage`: `fromFraction` / `fromPercent` / `asPercent` / `isZero` / `clamp01` / `add` / `mul` / `eql` / `parse` / `toCss`
- `NumOrPctKind`、`NumberOrPercentage`: `fromNumber` / `fromPercentage` / `asUnitValue` / `parse` / `toCss`
- `AlphaValue`: `parse` / `clamped` / `toByte` / `eql` / `toCss`
- `isAbsoluteCssUrl(url)` / `isDataCssUrl(url)`
