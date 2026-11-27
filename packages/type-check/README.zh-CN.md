[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# type-check

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation runtime type expression parser and validator supporting primitives, unions (A | B), Maybe T, arrays ([T]), tuples ((A, B)), and objects (clean-room MIT implementation of npm type-check, #241 / 195M wk)

## 安装与导入

```bash
cap add type-check
```

```cp
import { typeCheck } from "type-check";
```

## 核心导出 API

- `export enum DynValueKind`
- `export fn typeCheck(type_expr: string, val: DynValue): bool;`
- `export fn isValidTypeExpr(type_expr: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`type-check@1.0.0`)，许可证：MIT
