[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# picocolors

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation ultra-fast terminal ANSI color & style formatter with nested close-code recovery and NO_COLOR/FORCE_COLOR detection for CP (clean-room parity with npm picocolors)

## 安装与导入

```bash
cap add picocolors
```

```cp
import { detectColorSupport } from "picocolors";
```

## 核心导出 API

- `export fn detectColorSupport(no_color: string, force_color: string, is_tty: bool): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`picocolors@1.0.0`)，许可证：ISC
