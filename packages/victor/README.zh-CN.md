[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# victor

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation 2D vector math and planar geometry library with built-in pure-CP sqrt/sin/cos for CP.

## 安装与导入

```bash
cap add victor
```

```cp
import { new } from "victor";
```

## 核心导出 API

- `export fn new(x: f64, y: f64): Vec2;`
- `export fn zero(): Vec2;`
- `export fn splat(v: f64): Vec2;`
- `export fn add(a: Vec2, b: Vec2): Vec2;`
- `export fn sub(a: Vec2, b: Vec2): Vec2;`
- `export fn mul(a: Vec2, b: Vec2): Vec2;`
- `export fn scale(v: Vec2, s: f64): Vec2;`
- `export fn neg(v: Vec2): Vec2;`
- `export fn dot(a: Vec2, b: Vec2): f64;`
- `export fn cross(a: Vec2, b: Vec2): f64;`
- `export fn lengthSq(v: Vec2): f64;`
- `export fn length(v: Vec2): f64;`
- `export fn distance(a: Vec2, b: Vec2): f64;`
- `export fn normalize(v: Vec2): Vec2;`
- `export fn lerp(a: Vec2, b: Vec2, t: f64): Vec2;`
- `export fn perp(v: Vec2): Vec2;`
- `export fn reflect(v: Vec2, normal: Vec2): Vec2;`
- `export fn project(a: Vec2, onto: Vec2): Vec2;`
- `export fn rotate(v: Vec2, radians: f64): Vec2;`
- `export fn clampLength(v: Vec2, max_len: f64): Vec2;`
- `export fn approxEq(a: Vec2, b: Vec2, eps: f64): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`victor@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`euclid@1.0.0`)，许可证：MIT
