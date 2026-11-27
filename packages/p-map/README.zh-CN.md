[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# p-map

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap bounded-concurrency batch planner, chunk partitioner, and token-bucket rate limiter (clean-room port of npm p-map / p-all)

## 安装与导入

```bash
cap add p-map
```

```cp
import { batchCount } from "p-map";
```

## 核心导出 API

- `export fn batchCount(total_items: usize, concurrency: usize): usize;`
- `export fn batchRange(total_items: usize, batch_size: usize, batch_idx: usize): BatchSlice?;`
- `export fn workerPartition(total_items: usize, worker_count: usize, worker_idx: usize): BatchSlice?;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`p-map@1.0.0`)，许可证：MIT
