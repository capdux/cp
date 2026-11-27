[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# yn

> 版本：`5.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation parser for yes/no boolean strings (y/yes/true/1/on/n/no/false/0/off) with lenient mode

## 安装与导入

```bash
cap add yn
```

```cp
import { yn } from "yn";
```

## 核心导出 API

- `export fn yn(input: string): bool?;`
- `export fn ynLenient(input: string): bool?;`
- `export fn ynDefault(input: string, default_val: bool): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`yn@5.0.0`)（https://github.com/sindresorhus/yn），许可证：MIT
