[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# nth-check

> 版本：`2.1.1` | 许可证：`BSD-2-Clause` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation CSS :nth-child(An+B) microsyntax parser and index matcher

## 安装与导入

```bash
cap add nth-check
```

```cp
import { parseNth } from "nth-check";
```

## 核心导出 API

- `export fn parseNth(formula: string): NthRule;`
- `export fn nthCheck(formula: string, index: usize): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`nth-check@2.1.1`)（https://github.com/fb55/nth-check），许可证：BSD-2-Clause
