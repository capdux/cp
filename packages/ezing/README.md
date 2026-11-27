[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# ezing

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:ezing` ([https://github.com/NyxCode/ezing](https://github.com/NyxCode/ezing))（原库名 `ezing` → CP 包名 `ezing`）

Zero-allocation Robert Penner animation easing function family and CSS `cubic-bezier(x1, y1, x2, y2)` solver for CP — clean-room MIT parity with Rust `ezing` / `keyframe` and npm `eases`.

纯 CP 零堆分配动画缓动函数库，完整实现 Robert Penner 缓动家族（Linear / Quad / Cubic / Quart / Quint / Sine / Circ / Back / Bounce）与牛顿+二分双保险 CSS `cubic-bezier` 求解器。

## Install / 安装

```bash
cap add ezing
```

## API

| Family | Functions |
| --- | --- |
| Linear | `linear(t)` |
| Polynomial | `quadIn/Out/InOut`, `cubicIn/Out/InOut`, `quartIn/Out/InOut`, `quintIn/Out/InOut` |
| Trigonometric / Circular | `sineIn/Out/InOut`, `circIn/Out/InOut` |
| Overshoot / Bounce | `backIn/Out/InOut`, `bounceIn/Out/InOut` |
| CSS Bezier | `cubicBezier(x1, y1, x2, y2, x) -> f64` |

## Usage / 使用示例

```cp
import { cubicInOut, bounceOut, cubicBezier } from "ezing";

const a = cubicInOut(0.5);                        // 0.5
const b = bounceOut(0.75);
const c = cubicBezier(0.25, 0.1, 0.25, 1.0, 0.5); // CSS ease at x=0.5 (~0.8024)
```
