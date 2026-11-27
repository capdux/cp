[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# mathjs

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation mathematical expression parser & evaluator (npm mathjs parity) composing libm with scoped variables, constants, 14 built-in functions, and strict error reporting for CP

## 安装与导入

```bash
cap add mathjs
```

```cp
import { evalExprWithScope } from "mathjs";
```

## 核心导出 API

- `export fn evalExprWithScope(expr: string, scope: MathScope): EvalResult;`
- `export fn evalExpr(expr: string): EvalResult;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`mathjs@1.0.0`)，许可证：MIT
