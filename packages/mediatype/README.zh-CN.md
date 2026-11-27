[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# mediatype

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation RFC 9110 / RFC 6838 HTTP Media Type (MIME) parser, structured suffix extractor, and parameter matcher for CP.

## 安装与导入

```bash
cap add mediatype
```

```cp
import { isValidToken } from "mediatype";
```

## 核心导出 API

- `export fn isValidToken(s: string): bool;`
- `export fn parseMediaType(input: string, out: &mut MediaType): bool;`
- `export fn getParam(mt: MediaType, name: string, out: []u8): usize;`
- `export fn formatMediaType(mt: MediaType, out: []u8): usize;`
- `export fn isJsonLike(mt: MediaType): bool;`
- `export fn isTextLike(mt: MediaType): bool;`
- `export fn essenceEq(a: string, b: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`mediatype@1.0.0`)，许可证：MIT
