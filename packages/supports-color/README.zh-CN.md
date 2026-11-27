[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# supports-color

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation terminal color capability detection (supports-color v9 parity) powered by @cap/has-flag in pure CP

## 安装与导入

```bash
cap add supports-color
```

```cp
import { translateLevel } from "supports-color";
```

## 核心导出 API

- `export fn translateLevel(level: u8): ColorSupport;`
- `export fn detectColorLevel(env: TerminalEnv, argv: &[]string): u8;`
- `export fn detectColorSupport(env: TerminalEnv, argv: &[]string): ColorSupport;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`supports-color@1.0.0`)，许可证：MIT
