[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/vec2

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:victor` ([https://github.com/maxkueng/victor](https://github.com/maxkueng/victor)) / `crates.io:euclid` ([https://github.com/servo/euclid](https://github.com/servo/euclid))（原库名 `victor` → CP 包名 `@cap/vec2`）

Zero-allocation 2D vector math and planar geometry library for CP with built-in Newton-Raphson `sqrt` and Taylor-series `sin`/`cos` (zero external dependencies).

## Features

- **Zero heap allocation**: value-type `Vec2 { x: f64, y: f64 }`.
- **Constructors & arithmetic**: `new`, `zero`, `splat`, `add`, `sub`, `mul`, `scale`, `neg`.
- **Planar geometry**: `dot`, `cross`, `lengthSq`, `length`, `distance`, `normalize`, `lerp`, `perp`, `reflect`, `project`, `rotate`, `clampLength`, `approxEq`.

## Usage

```cp
import { new, rotate, normalize, dot } from "@cap/vec2";

const v = new(3.0 as f64, 4.0 as f64);
const u = normalize(v); // (0.6, 0.8)
```
