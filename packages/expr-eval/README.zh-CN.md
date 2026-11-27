[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# expr-eval

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation recursive-descent expression evaluation engine with variables, ternary/logical/math operators, built-in functions, and recursion depth guard for CP

## 安装与导入

```bash
cap add expr-eval
```

```cp
import { isIdentStart } from "expr-eval";
```

## 核心导出 API

- `export enum ExprError`
- `export enum TokenKind`
- `export fn isIdentStart(c: u8): bool;`
- `export fn isIdentCont(c: u8): bool;`
- `export fn isTruthy(v: f64): bool;`
- `export fn parseFloatLiteral(slice: string): f64?;`
- `export fn evalExpr(source: string): EvalResult;`
- `export fn evalExprWith(source: string, env: ExprEnv): EvalResult;`
- `export fn evalBool(source: string, env: ExprEnv): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`expr-eval@1.0.0`)，许可证：MIT
