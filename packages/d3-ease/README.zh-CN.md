[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# d3-ease

> 版本：`1.0.0` | 许可证：`ISC` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation D3 easing functions (linear, quad, cubic, poly, sin, exp, circle, bounce, back, elastic) in pure CP

## 安装与导入

```bash
cap add d3-ease
```

```cp
import { deasTpmt } from "d3-ease";
```

## 核心导出 API

- `export fn deasTpmt(x: f64): f64;`
- `export fn deasClampAmp(a: f64): f64;`
- `export fn easeLinear(t: f64): f64;`
- `export fn easeQuadIn(t: f64): f64;`
- `export fn easeQuadOut(t: f64): f64;`
- `export fn easeQuadInOut(t: f64): f64;`
- `export fn easeCubicIn(t: f64): f64;`
- `export fn easeCubicOut(t: f64): f64;`
- `export fn easeCubicInOut(t: f64): f64;`
- `export fn easePolyInExp(t: f64, e: f64): f64;`
- `export fn easePolyOutExp(t: f64, e: f64): f64;`
- `export fn easePolyInOutExp(t: f64, e: f64): f64;`
- `export fn easePolyIn(t: f64): f64;`
- `export fn easePolyOut(t: f64): f64;`
- `export fn easePolyInOut(t: f64): f64;`
- `export fn easeSinIn(t: f64): f64;`
- `export fn easeSinOut(t: f64): f64;`
- `export fn easeSinInOut(t: f64): f64;`
- `export fn easeExpIn(t: f64): f64;`
- `export fn easeExpOut(t: f64): f64;`
- `export fn easeExpInOut(t: f64): f64;`
- `export fn easeCircleIn(t: f64): f64;`
- `export fn easeCircleOut(t: f64): f64;`
- `export fn easeCircleInOut(t: f64): f64;`
- `export fn easeBounceOut(t: f64): f64;`
- `export fn easeBounceIn(t: f64): f64;`
- `export fn easeBounceInOut(t: f64): f64;`
- `export fn easeBackInOvershoot(t: f64, s: f64): f64;`
- `export fn easeBackOutOvershoot(t: f64, s: f64): f64;`
- `export fn easeBackInOutOvershoot(t: f64, s: f64): f64;`
- `export fn easeBackIn(t: f64): f64;`
- `export fn easeBackOut(t: f64): f64;`
- `export fn easeBackInOut(t: f64): f64;`
- `export fn easeElasticInParams(t: f64, a: f64, p: f64): f64;`
- `export fn easeElasticOutParams(t: f64, a: f64, p: f64): f64;`
- `export fn easeElasticInOutParams(t: f64, a: f64, p: f64): f64;`
- `export fn easeElasticIn(t: f64): f64;`
- `export fn easeElasticOut(t: f64): f64;`
- `export fn easeElasticInOut(t: f64): f64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`d3-ease@1.0.0`)，许可证：ISC
