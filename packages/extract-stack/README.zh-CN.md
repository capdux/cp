[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# extract-stack

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation Error.stack frame extractor (npm extract-stack v2 parity) for CP

## 安装与导入

```bash
cap add extract-stack
```

```cp
import { exsIsHspace } from "extract-stack";
```

## 核心导出 API

- `export fn exsIsHspace(c: u8): bool;`
- `export fn exsIsFrameLine(line: string): bool;`
- `export fn exsFindFirstFrameOffset(stack: string): usize;`
- `export fn exsTrimLine(line: string): string;`
- `export fn extractStack(stack: string): string;`
- `export fn extractErrorHeader(stack: string): string;`
- `export fn extractStackLines(stack: string, out: []string): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`extract-stack@1.0.0`)，许可证：MIT
