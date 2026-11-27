[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# num-complex

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation Complex64 arithmetic, Smith overflow-safe division, polar coordinates, Euler exp, principal sqrt, and formatting (Rust num-complex parity) for CP

## 安装与导入

```bash
cap add num-complex
```

```cp
import { fabs } from "num-complex";
```

## 核心导出 API

- `export fn fabs(x: f64): f64;`
- `export fn pushByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn pushU64(out: []u8, pos: usize, val: u64): usize;`
- `export fn pushAbsFloat(out: []u8, pos: usize, v: f64): usize;`
- `export fn formatComplex(z: Complex64, out: []u8): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`num-complex@1.0.0`)，许可证：MIT
