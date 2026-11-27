[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# clean-stack

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation stack trace cleaner removing internal runtime frames and normalizing base paths (clean-room MIT implementation of npm clean-stack v5)

## 安装与导入

```bash
cap add clean-stack
```

```cp
import { isInternalStackFrame } from "clean-stack";
```

## 核心导出 API

- `export fn isInternalStackFrame(line: string): bool;`
- `export fn cleanStackWithOptions(stack: string, opts: CleanStackOptions, out: []u8): usize;`
- `export fn cleanStack(stack: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`clean-stack@1.0.0`)，许可证：MIT
