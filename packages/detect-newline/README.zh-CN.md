[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# detect-newline

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation dominant newline detector and cross-platform LF/CRLF normalizer for CP

## 安装与导入

```bash
cap add detect-newline
```

```cp
import { analyzeNewlines } from "detect-newline";
```

## 核心导出 API

- `export fn analyzeNewlines(input: string): NewlineStats;`
- `export fn detectNewline(input: string): string;`
- `export fn detectNewlineGraceful(input: string): string;`
- `export fn normalizeToLf(input: string, out: []u8): usize;`
- `export fn normalizeToCrlf(input: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`detect-newline@1.0.0`)，许可证：MIT
