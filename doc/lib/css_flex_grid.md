# `cap:css_flex_grid` — 纯 CP CSS Flexbox、Box Alignment 与 Grid 布局属性模型

## 概述

`cap:css_flex_grid` 吸收自 Bun `src/css/properties/{flex,align,grid}.cp`，纯 CP 零外部依赖、零堆分配，提供 Flexbox 方向/换行/简写、Box Alignment 四大对齐属性以及 Grid 自动流与轨道宽度模型。

## 核心类型与 API

### Flexbox
- `FlexDirection`（`row`/`row_reverse`/`column`/`column_reverse`，附 `isHorizontal()`/`isVertical()`/`isReversed()`）
- `FlexWrap`（`nowrap`/`wrap`/`wrap_reverse`，附 `isMultiLine()`）
- `FlexFlow`：`parse(input)`、`toCss(buf)`
- `FlexBasisKind` / `FlexBasis`：`auto()` / `px(v)` / `percent(v)` / `parse()` / `toCss(buf)`
- `FlexShorthand`：`initial()` / `auto()` / `none()` / `parse(input)` / `toCss(buf)`

### Box Alignment
- `JustifyContent` / `AlignItems` / `AlignSelf` / `AlignContent`（均含 `default()` / `parse()` / `toCss()`）

### Grid
- `GridAutoFlow`（`row` / `column` / `row_dense` / `column_dense`，附 `isDense()` / `isColumn()` / `isRow()`）
- `TrackBreadthKind` / `TrackBreadth`（`auto` / `min-content` / `max-content` / `<fr>` / `<px>` / `<%>`，附 `isFlexible()` / `isIntrinsic()` / `toCss(buf)`）

## 示例

```cp
import { FlexFlow, FlexShorthand, GridAutoFlow, TrackBreadth } from "cap:css_flex_grid";

const flow = FlexFlow.parse("column wrap")?;
const flex = FlexShorthand.parse("2 1 120px")?;
const gaf = GridAutoFlow.parse("dense column")?; // GridAutoFlow.column_dense
const tb = TrackBreadth.parse("1.5fr")?; // tb.isFlexible() == true
```
