[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# tdigest

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation Merging T-Digest streaming quantile (P50/P95/P99/P99.9) and CDF estimator with distributed merge for CP

## 安装与导入

```bash
cap add tdigest
```

```cp
import { emptyCentroid } from "tdigest";
```

## 核心导出 API

- `export fn emptyCentroid(): Centroid;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`tdigest@1.0.0`)，许可证：MIT
