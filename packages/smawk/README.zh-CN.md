[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# smawk

> 版本：`0.3.2` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap SMAWK / monotone matrix row-minima algorithm and total monotonicity checker in pure CP

## 安装与导入

```bash
cap add smawk
```

```cp
import { smwkRowMinInRange } from "smawk";
```

## 核心导出 API

- `export fn smwkRowMinInRange(matrix: &[]f64, cols: usize, row: usize, c_lo: usize, c_hi: usize): usize;`
- `export fn rowMinima(matrix: &[]f64, rows: usize, cols: usize, out_min_cols: []usize): usize;`
- `export fn isTotallyMonotone(matrix: &[]f64, rows: usize, cols: usize): bool;`
- `export fn smwkDivideConquer(matrix: &[]f64, cols: usize, r_lo: usize, r_hi: usize, c_lo: usize, c_hi: usize, out_min_cols: []usize);`
- `export fn mongeRowMinima(matrix: &[]f64, rows: usize, cols: usize, out_min_cols: []usize): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`smawk@0.3.2`)（https://github.com/mgeisler/smawk），许可证：MIT
