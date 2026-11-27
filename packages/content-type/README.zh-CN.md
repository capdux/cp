[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# content-type

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap RFC 7231 HTTP Content-Type header parser and formatter with quoted-string unescaping (clean-room port of content-type)

## 安装与导入

```bash
cap add content-type
```

```cp
import { ctypIsOws } from "content-type";
```

## 核心导出 API

- `export fn ctypIsOws(c: u8): bool;`
- `export fn ctypIsTokenChar(c: u8): bool;`
- `export fn ctypIsValidToken(s: string): bool;`
- `export fn ctypIsQuotedAllowed(c: u8): bool;`
- `export fn ctypWriteByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn ctypWriteSlice(out: []u8, pos: usize, s: string): usize;`
- `export fn isValidMediaType(s: string): bool;`
- `export fn unescapeQuotedString(raw: string, out: []u8): usize;`
- `export fn parseContentType(header: string): ContentType;`
- `export fn formatContentType(media_type: string, params: &[]ContentTypeParam, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`content-type@1.0.0`)，许可证：MIT
