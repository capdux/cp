[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# ndarray

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap fixed-capacity 2D/N-D numerical array with stride indexing, matmul, transpose, dot, and reductions (crates.io ndarray equivalent)

## 安装与导入

```bash
cap add ndarray
```

```cp
import { addArrays } from "ndarray";
```

## 核心导出 API

- `export fn addArrays(a: Array2D, b: Array2D): Array2D?;`
- `export fn matmul(a: Array2D, b: Array2D): Array2D?;`
- `export fn dotProduct(a: []f64, b: []f64): f64?;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`ndarray@1.0.0`)，许可证：MIT OR Apache-2.0
