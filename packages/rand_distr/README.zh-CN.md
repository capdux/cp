[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# rand_distr

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap probability distribution samplers (Uniform, Normal Box-Muller, Exponential, Bernoulli, WeightedIndex) built on rand_xoshiro (clean-room port of Rust rand_distr)

## 安装与导入

```bash
cap add rand_distr
```

```cp
import { rdstAbs } from "rand_distr";
```

## 核心导出 API

- `export fn rdstAbs(x: f64): f64;`
- `export fn rdstSqrt(x: f64): f64;`
- `export fn rdstLn(x: f64): f64;`
- `export fn rdstCos(angle: f64): f64;`
- `export fn rdstOpen01(rng: &mut Xoshiro256): f64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`rand_distr@1.0.0`)，许可证：MIT OR Apache-2.0
