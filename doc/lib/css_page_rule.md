# `cap:css_page_rule` — 纯 CP CSS Paged Media `@page` 规则与 `position` 定位属性模型

## 概述

`cap:css_page_rule` 吸收自 Bun `src/css/rules/page.cp` 与 `src/css/properties/position.cp`，纯 CP 零外部依赖，提供 CSS Paged Media Level 3 的 16 种页边盒 `PageMarginBox`、页伪类 `PagePseudoClass`、零堆分配 `@page` 页选择器 `PageSelector` 解析与序列化，以及 CSS `Position` 定位属性解析与布局特性判定。

## 核心类型与 API

### `PagePseudoClass` & `PageMarginBox`

- `PagePseudoClass`：`left` / `right` / `first` / `last` / `blank`
  - `parse(input)`（兼容 `:left` 与 `left`）、`toCss()`、`toSelectorCss()`
- `PageMarginEdge`：`top` / `bottom` / `left` / `right` / `corner`
- `PageMarginBox`：全部 16 种页边盒（`top_left_corner` .. `bottom_right_corner`）
  - `parse(input)`（兼容 `@top-left` 与 `top-left`）、`toCss()`、`toAtRuleCss()`、`isCorner()`、`isTop()`、`isBottom()`、`isLeft()`、`isRight()`、`edge()`

### `PageSelector`（零堆分配）

- `PageSelector.parse(input: string): PageSelector ! CssPageError`（支持 `"toc:first:left"`、`":blank"`、`"landscape"` 等，拦截空白/非法伪类/空选择器）
- `isAnonymous(): bool`、`hasPseudo(target)`、`toCss(buf)`、`formatAtPageRule(buf)`

### `Position`

- 枚举值：`static_pos` / `relative` / `absolute` / `fixed` / `sticky` / `webkit_sticky`
- 方法：`default()`、`parse(input)`、`toCss()`、`isOutOfFlow()`、`isPositioned()`、`isSticky()`、`isWebKitPrefixed()`、`createsStackingContext()`

## 示例

```cp
import { PageMarginBox, PageSelector, Position } from "cap:css_page_rule";

const box = PageMarginBox.parse("@top-right-corner").?; // box.isCorner() == true
const sel = PageSelector.parse("chapter:first:right")?;
const pos = Position.parse("-webkit-sticky").?; // pos.isSticky() && pos.isWebKitPrefixed()
```
