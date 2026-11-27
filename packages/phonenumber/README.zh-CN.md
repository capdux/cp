[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# phonenumber

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap RFC 3966 tel: URI parser, E.164 international phone number normalizer/validator, and URI formatter.

## 安装与导入

```bash
cap add phonenumber
```

```cp
import { isVisualSep } from "phonenumber";
```

## 核心导出 API

- `export fn isVisualSep(c: u8): bool;`
- `export fn isWs(c: u8): bool;`
- `export fn isValidE164(number: string): bool;`
- `export fn normalizeE164(input: string, out: []u8): usize;`
- `export fn parseTelUri(uri: string): TelUri;`
- `export fn appendSlice(out: []u8, pos: usize, s: string): i64;`
- `export fn formatTelUri(e164_number: string, ext: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`phonenumber@1.0.0`)，许可证：Apache-2.0
