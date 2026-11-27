[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# commander

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation CLI command framework (npm commander v12 parity) composing yargs-parser and cliui for CP

## 安装与导入

```bash
cap add commander
```

```cp
import { parseOptionFlags } from "commander";
```

## 核心导出 API

- `export fn parseOptionFlags(flags: string): FlagSpec;`
- `export fn parseArgSpec(spec: string, desc: string): ArgDef;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`commander@1.0.0`)，许可证：MIT
