[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# escalade

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap ancestor directory path walker (clean-room CP implementation of npm escalade)

## 安装与导入

```bash
cap add escalade
```

```cp
import { joinCandidate } from "escalade";
```

## 核心导出 API

- `export fn joinCandidate(dir: string, name: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`escalade@1.0.0`)，许可证：MIT
