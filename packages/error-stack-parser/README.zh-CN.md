[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# error-stack-parser

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation multi-language stack trace parser & cleaner supporting V8/Node/Bun, SpiderMonkey/Safari, and Rust/Zig/CP formats (error-stack-parser / stacktrace-parser / clean-stack parity) for CP

## 安装与导入

```bash
cap add error-stack-parser
```

```cp
import { isInternalLocation } from "error-stack-parser";
```

## 核心导出 API

- `export fn isInternalLocation(file: string, is_native: bool): bool;`
- `export fn parseLocation(raw_loc: string): LocationInfo;`
- `export fn parseFrameLine(raw_line: string): StackFrame?;`
- `export fn parseStack(raw: string): ParsedStack;`
- `export fn pushByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn formatFrameAt(f: StackFrame, out: []u8, start_pos: usize): usize;`
- `export fn formatFrame(f: StackFrame, out: []u8): string;`
- `export fn cleanStack(raw: string, out: []u8): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`error-stack-parser@1.0.0`)，许可证：MIT
