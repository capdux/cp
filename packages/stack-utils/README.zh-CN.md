[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# stack-utils

> 版本：`2.0.6` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap V8 stack trace line parser extracting function name, file path, line, column, and native flag

## 安装与导入

```bash
cap add stack-utils
```

```cp
import { stkuIsSpace } from "stack-utils";
```

## 核心导出 API

- `export fn stkuIsSpace(ch: u8): bool;`
- `export fn stkuTrim(s: string): string;`
- `export fn stkuEq(a: string, b: string): bool;`
- `export fn stkuIsDigits(s: string): bool;`
- `export fn stkuParseU32(s: string): u32;`
- `export fn stkuLastColon(s: string): usize;`
- `export fn stkuInvalid(): StackFrame;`
- `export fn stkuParseLoc(fn_name: string, loc: string): StackFrame;`
- `export fn parseStackLine(line: string): StackFrame;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`stack-utils@2.0.6`)（https://github.com/tapjs/stack-utils），许可证：MIT
