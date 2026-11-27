[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# on-finished

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap HTTP message/stream completion tracker with once-only finish/error/close lifecycle dispatch (npm on-finished equivalent)

## 安装与导入

```bash
cap add on-finished
```

```cp
import { isFinished } from "on-finished";
```

## 核心导出 API

- `export fn isFinished(lc: StreamLifecycle): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`on-finished@1.0.0`)，许可证：MIT
