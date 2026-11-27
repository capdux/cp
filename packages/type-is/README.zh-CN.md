[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# type-is

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap HTTP Content-Type matcher supporting shorthands, wildcards (text/*), and structured suffixes (+json) (clean-room port of npm type-is)

## 安装与导入

```bash
cap add type-is
```

```cp
import { normalizeTypePattern } from "type-is";
```

## 核心导出 API

- `export fn normalizeTypePattern(pattern: string): string;`
- `export fn typeMatches(content_type: string, pattern: string): bool;`
- `export fn typeIs(content_type: string, candidates_csv: string): string?;`
- `export fn hasBody(transfer_encoding: string, content_length: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`type-is@1.0.0`)，许可证：MIT
