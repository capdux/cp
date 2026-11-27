[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# normalize-url

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation URL normalization utility (npm normalize-url parity) composing @cap/prepend-http for CP

## 安装与导入

```bash
cap add normalize-url
```

```cp
import { defaultNormalizeUrlOptions } from "normalize-url";
```

## 核心导出 API

- `export fn defaultNormalizeUrlOptions(): NormalizeUrlOptions;`
- `export fn nurlWriteByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn nurlWriteLower(out: []u8, pos: usize, s: string): usize;`
- `export fn nurlFindSchemeSep(s: string): usize;`
- `export fn nurlIsDefaultPort(scheme: string, port: string): bool;`
- `export fn nurlNormalizePath(raw_path: string, remove_trailing_slash: bool, has_query_or_hash: bool, out: []u8, start_pos: usize): usize;`
- `export fn normalizeUrlWithOptions(url: string, opts: NormalizeUrlOptions, out: []u8): usize;`
- `export fn normalizeUrl(url: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`normalize-url@1.0.0`)，许可证：MIT
