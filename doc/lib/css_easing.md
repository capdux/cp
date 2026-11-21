# `cap:css_easing` — 纯 CP CSS 缓动函数解析、规范化与曲线求值引擎

## 概述

`cap:css_easing` 吸收自 Bun `src/css/values/easing.cp`，纯 CP 零外部依赖，实现 W3C CSS Easing Functions Level 1 规范中的关键字、`cubic-bezier(x1,y1,x2,y2)` 与 `steps(n, pos)` 解析、等价关键字规范化化简、CSS 序列化，以及基于 Newton-Raphson + 二分法兜底的 `sample(t: f64)` 曲线数值求值。

## 核心类型与 API

### 错误集与枚举

- `CssEasingError`：`InvalidEasing` / `InvalidBezierControlPoint` / `InvalidStepCount` / `InvalidStepPosition` / `BufferTooSmall`
- `StepPosition`：`start` / `end` / `jump_none` / `jump_both`（`parse()` 兼容 `jump-start`/`jump-end`）
- `EasingKind`：`linear` / `ease` / `ease_in` / `ease_out` / `ease_in_out` / `cubic_bezier` / `steps`

### `CubicBezier` & `EasingSteps` & `EasingFunction`

- `CubicBezier`：`x1`/`y1`/`x2`/`y2`、`eql(other)`、`sample(t: f64): f64`
- `EasingSteps`：`count`/`position`、`sample(t: f64): f64`（W3C §2.3.1 阶梯算法）
- `EasingFunction`：
  - 构造：`linear()` / `ease()` / `easeIn()` / `easeOut()` / `easeInOut()` / `cubicBezier(x1, y1, x2, y2)` / `makeSteps(count, pos)`
  - 解析与判定：`isIdent(s)`、`parse(input)`、`normalize()`、`isEase()`
  - 采样与输出：`sample(t: f64): f64`、`toCss(buf: []u8): usize ! CssEasingError`

## 示例

```cp
import { EasingFunction } from "cap:css_easing";

const ef = EasingFunction.parse("cubic-bezier(0.25, 0.1, 0.25, 1)")?;
// ef.isEase() == true
// ef.sample(0.5) ≈ 0.8024
```
