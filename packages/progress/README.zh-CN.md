[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# progress

> 版本：`2.0.3` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap terminal progress bar renderer supporting :bar, :percent, :current, :total, :eta, and :rate tokens

## 安装与导入

```bash
cap add progress
```

```cp
import { newProgressBar } from "progress";
```

## 核心导出 API

- `export fn newProgressBar(total: u64, width: usize): ProgressBar;`
- `export fn renderProgress(bar: ProgressBar, template: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`progress@2.0.3`)（https://github.com/visionmedia/node-progress），许可证：MIT
