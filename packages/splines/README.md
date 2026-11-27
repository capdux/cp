[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# splines

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:splines` ([https://github.com/phaazon/splines](https://github.com/phaazon/splines))（原库名 `splines` → CP 包名 `splines`）

Zero-heap Natural Cubic Spline (Thomas O(N)), Catmull-Rom spline, quadratic/cubic Bezier curves, smoothstep, and linear interpolation in pure CP.

## Features

- `CubicSpline`: Natural cubic spline up to `MAX_KNOTS = 64` knots, O(N) Thomas tridiagonal solver, O(log N) binary search segment lookup, Horner evaluation (`eval`) & first derivative (`derivative`)
- `bezierQuad` & `bezierCubic` on `Vec2`
- `catmullRom`, `smoothstep`, `lerp`

## Usage

```cp
import { CubicSpline, bezierCubic, catmullRom, Vec2 } from "splines";

const xs: [4]f64 = [0.0, 1.0, 2.0, 3.0];
const ys: [4]f64 = [0.0, 2.0, 1.0, 3.0];
const sp = CubicSpline.build(xs[0..4], ys[0..4]);
const y = sp.eval(1.5);
const dy = sp.derivative(1.5);
```
