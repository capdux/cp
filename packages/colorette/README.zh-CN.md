[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# colorette

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap pure CP terminal ANSI color formatter with nested close-code recovery (clean-room MIT implementation of npm colorette v2)

## 安装与导入

```bash
cap add colorette
```

```cp
import { isColorSupported } from "colorette";
```

## 核心导出 API

- `export fn isColorSupported(no_color: string, force_color: string, term: string, is_tty: bool): bool;`
- `export fn createColors(use_color: bool): Colorette;`
- `export fn createColorsWithOptions(opts: ColoretteOptions): Colorette;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`colorette@1.0.0`)，许可证：MIT
