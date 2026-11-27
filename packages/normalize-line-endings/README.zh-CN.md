[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# normalize-line-endings

> 版本：`0.3.0` | 许可证：`Apache-2.0` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap line ending detector and normalizer (CRLF/CR <-> LF)

## 安装与导入

```bash
cap add normalize-line-endings
```

```cp
import { detectLineEnding } from "normalize-line-endings";
```

## 核心导出 API

- `export enum LineEnding`
- `export fn detectLineEnding(input: string): LineEnding;`
- `export fn normalizeToLf(input: string, out: []u8): usize;`
- `export fn normalizeToCrlf(input: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`normalize-line-endings@0.3.0`)（https://github.com/derekdreery/normalize-line-endings），许可证：Apache-2.0
