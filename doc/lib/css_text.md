# `cap:css_text` — 纯 CP CSS Text 3/4 与 Text Decoration 属性族

## 概述

`cap:css_text` 吸收自 Bun `src/css/properties/text.cp`，纯 CP 零外部依赖、零堆分配，完整实现文本变换、空白处理、断词换行、对齐以及文本装饰线位标志与 `text-decoration` 简写解析和规范化序列化。

## 核心类型与 API

- `TextTransformCase` + `TextTransform`（含 `full-width`/`full-size-kana` 位标志）
- `WhiteSpace`（6 变体，附 `preservesNewlines()` / `allowsWrapping()`）
- `WordBreak` / `LineBreak` / `Hyphens` / `OverflowWrap` / `TextAlign`（8 变体）
- `TextDecorationLine`（4 位标志：`underline` / `overline` / `line-through` / `blink`）
- `TextDecorationStyle`（`solid`/`double`/`dotted`/`dashed`/`wavy`）、`TextDecorationThickness`、`TextDecoration` 简写

## 示例

```cp
import { TextTransform, TextDecoration } from "cap:css_text";

const tt = TextTransform.parse("uppercase full-width")?;
const td = TextDecoration.parse("wavy red underline overline 2px")?;
```
