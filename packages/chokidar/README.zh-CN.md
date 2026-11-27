[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# chokidar

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap pure CP file watcher state machine, snapshot differ, debounce engine, and glob ignore filter (clean-room MIT implementation of npm chokidar v4 core)

## 安装与导入

```bash
cap add chokidar
```

```cp
import { chkStrEq } from "chokidar";
```

## 核心导出 API

- `export fn chkStrEq(a: string, b: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`chokidar@1.0.0`)，许可证：MIT
