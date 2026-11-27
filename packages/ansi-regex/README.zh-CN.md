[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# ansi-regex

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap ANSI escape sequence matcher & locator reusing anstyle-parse, matching npm ansi-regex (400M+/wk) in pure CP

## 安装与导入

```bash
cap add ansi-regex
```

```cp
import { testAnsi } from "ansi-regex";
```

## 核心导出 API

- `export fn testAnsi(input: string): bool;`
- `export fn findFirstAnsi(input: string): AnsiSpan;`
- `export fn findAllAnsi(input: string, out: []AnsiSpan): usize;`
- `export fn countAnsiSequences(input: string): usize;`
- `export fn stripAllAnsi(input: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`ansi-regex@1.0.0`)，许可证：MIT
