# `cap:css_transition` — 纯 CP CSS Transitions 1/2 过渡属性族与多过渡列表模型

## 概述

`cap:css_transition` 吸收自 Bun `src/css/properties/transition.cp`，纯 CP 零外部依赖、零堆分配，提供 `TransitionBehavior`、`TransitionProperty`、`TransitionTime`、`TransitionEasing`、`Transition` 单过渡简写与 `TransitionList` 多过渡列表的解析与规范化 `toCss` 序列化。

## 核心类型与 API

- `CssTransitionError`：`InvalidTime` / `InvalidDuration` / `InvalidEasing` / `InvalidProperty` / `InvalidBehavior` / `InvalidTransition` / `TooManyTransitions` / `BufferTooSmall`
- `TransitionBehavior`（`normal` / `allow_discrete`）
- `TransitionPropertyKind` / `TransitionProperty`（`none` / `all` / `custom_ident`）
- `TransitionTime`（毫秒统一表示，支持 `s`/`ms` 解析与最短规范化输出 `0s`/`200ms`/`1.5s`）
- `TransitionEasingKind` / `TransitionEasing`（关键字、`cubic-bezier(x1,y1,x2,y2)`、`steps(n, pos)`）
- `Transition`（任意顺序解析，首 `<time>` 为 duration、次 `<time>` 为 delay；`toCss` 在 delay 非 0 时自动保留 duration）
- `TransitionList`（逗号分隔多过渡列表，忽略函数括号内逗号）

## 示例

```cp
import { TransitionList } from "cap:css_transition";

const list = TransitionList.parse("opacity 200ms ease-in, transform 0.3s cubic-bezier(0.2, 0, 0, 1) 50ms")?;
```
