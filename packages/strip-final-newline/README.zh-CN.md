[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# strip-final-newline

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-copy final LF/CRLF newline stripper and normalizer matching npm strip-final-newline (250M+/wk) in pure CP

## 安装与导入

```bash
cap add strip-final-newline
```

```cp
import { stripFinalNewline } from "strip-final-newline";
```

## 核心导出 API

- `export fn stripFinalNewline(input: string): string;`
- `export fn stripAllTrailingNewlines(input: string): string;`
- `export fn hasFinalNewline(input: string): bool;`
- `export fn hasFinalCrlf(input: string): bool;`
- `export fn ensureFinalNewline(input: string, use_crlf: bool, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`strip-final-newline@1.0.0`)，许可证：MIT
