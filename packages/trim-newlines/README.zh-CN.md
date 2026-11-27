[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# trim-newlines

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation trimming of leading and trailing CR/LF newlines from strings (clean-room MIT implementation of npm trim-newlines v5)

## 安装与导入

```bash
cap add trim-newlines
```

```cp
import { trimNewlinesStart } from "trim-newlines";
```

## 核心导出 API

- `export fn trimNewlinesStart(input: string): string;`
- `export fn trimNewlinesEnd(input: string): string;`
- `export fn trimNewlines(input: string): string;`
- `export fn countLeadingNewlines(input: string): usize;`
- `export fn countTrailingNewlines(input: string): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`trim-newlines@1.0.0`)，许可证：MIT
