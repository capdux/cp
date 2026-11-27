[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# estree-walker

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation index-pooled ESTree AST arena and non-recursive DFS walker with skip, remove, and replace for CP

## 安装与导入

```bash
cap add estree-walker
```

```cp
import { estwValidId } from "estree-walker";
```

## 核心导出 API

- `export enum EstreeNodeKind`
- `export enum WalkPhase`
- `export fn estwValidId(id: i32, count: usize): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`estree-walker@1.0.0`)，许可证：MIT
