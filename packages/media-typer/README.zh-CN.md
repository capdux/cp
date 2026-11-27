[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# media-typer

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap RFC 6838 HTTP media type parser, validator, and formatter (clean-room port of npm media-typer)

## 安装与导入

```bash
cap add media-typer
```

```cp
import { mtIsAlphaNum } from "media-typer";
```

## 核心导出 API

- `export fn mtIsAlphaNum(b: u8): bool;`
- `export fn mtIsTypeChar(b: u8): bool;`
- `export fn mtIsSuffixChar(b: u8): bool;`
- `export fn mtIsSubtypeChar(b: u8): bool;`
- `export fn isValidType(s: string): bool;`
- `export fn isValidSubtype(s: string): bool;`
- `export fn isValidSuffix(s: string): bool;`
- `export fn mtPushLower(out: []u8, pos: usize, s: string): usize;`
- `export fn mtStartsWithAscii(s: string, prefix: string): bool;`
- `export fn testMediaType(raw: string): bool;`
- `export fn testType(raw: string): bool;`
- `export fn parseMediaType(raw: string): MediaType;`
- `export fn parse(raw: string): MediaType;`
- `export fn formatMediaType(main_type: string, subtype: string, suffix: string, out: []u8): usize;`
- `export fn format(mt: MediaType, out: []u8): usize;`
- `export fn essence(raw: string, out: []u8): usize;`
- `export fn mediaTypeEq(a: string, b: string): bool;`
- `export fn hasSuffix(raw: string, suffix: string): bool;`
- `export fn isStandardType(t: string): bool;`
- `export fn isVendorSubtype(subtype: string): bool;`
- `export fn isPersonalSubtype(subtype: string): bool;`
- `export fn isUnregisteredSubtype(subtype: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`media-typer@1.0.0`)，许可证：MIT
