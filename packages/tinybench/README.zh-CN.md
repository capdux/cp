[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# tinybench

> 版本：`3.2.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap micro-benchmark statistics engine (mean, ops/sec, min/max, p50/p75/p95/p99, stddev, RME)

## 安装与导入

```bash
cap add tinybench
```

```cp
import { computeStats } from "tinybench";
```

## 核心导出 API

- `export fn computeStats(samples_ns: &[]f64): BenchStats;`
- `export fn speedupRatio(baseline: BenchStats, candidate: BenchStats): f64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`tinybench@3.2.0`)（https://github.com/tinylibs/tinybench），许可证：MIT
