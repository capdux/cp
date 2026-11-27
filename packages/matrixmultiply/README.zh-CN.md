[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# matrixmultiply

> 版本：`0.3.10` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap general dense matrix multiplication (sgemm / dgemm / matmulF64) with arbitrary row and column strides

## 安装与导入

```bash
cap add matrixmultiply
```

```cp
import { mmulMin } from "matrixmultiply";
```

## 核心导出 API

- `export fn mmulMin(a: usize, b: usize): usize;`
- `export fn sgemm(m: usize, k: usize, n: usize, alpha: f32, a: &[]f32, rsa: usize, csa: usize, b: &[]f32, rsb: usize, csb: usize, beta: f32, c: []f32, rsc: usize, csc: usize);`
- `export fn dgemm(m: usize, k: usize, n: usize, alpha: f64, a: &[]f64, rsa: usize, csa: usize, b: &[]f64, rsb: usize, csb: usize, beta: f64, c: []f64, rsc: usize, csc: usize);`
- `export fn matmulF64(m: usize, k: usize, n: usize, a: &[]f64, b: &[]f64, c: []f64);`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`matrixmultiply@0.3.10`)（https://github.com/bluss/matrixmultiply），许可证：MIT
