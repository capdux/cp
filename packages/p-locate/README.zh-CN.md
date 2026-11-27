[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# p-locate

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation bounded-concurrency ordered/unordered locator planner (npm p-locate v7 equivalent) for CP, reusing @cap/p-limit.

## 安装与导入

```bash
cap add p-locate
```

```cp
import { plocEmptyCandidate } from "p-locate";
```

## 核心导出 API

- `export fn plocEmptyCandidate(): LocateCandidate;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`p-locate@1.0.0`)，许可证：MIT
