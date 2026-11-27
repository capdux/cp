[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# utf8parse

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation RFC 3629 strict UTF-8 validator, WHATWG lossy repair (U+FFFD), ASCII fast-path, and UTF-16 code-unit length calculator for CP

## 安装与导入

```bash
cap add utf8parse
```

```cp
import { isCont } from "utf8parse";
```

## 核心导出 API

- `export fn isCont(b: u8): bool;`
- `export fn checkSeqAt(data: string, pos: usize): StepResult;`
- `export fn isAscii(data: string): bool;`
- `export fn validateUtf8(data: string): Utf8CheckResult;`
- `export fn isValidUtf8(data: string): bool;`
- `export fn appendByte(out: []u8, pos: usize, c: u8): usize;`
- `export fn toLossyUtf8(data: string, out: []u8): string;`
- `export fn utf16Len(data: string): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`utf8parse@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`utf8_iter@1.0.0`)，许可证：MIT
