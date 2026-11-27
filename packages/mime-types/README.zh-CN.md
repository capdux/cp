[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# mime-types

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap MIME type, file extension, charset, Content-Type, and compressibility resolver in pure CP (clean-room MIT counterpart to npm mime-types v2.1).

## 安装与导入

```bash
cap add mime-types
```

```cp
import { mtypToLower } from "mime-types";
```

## 核心导出 API

- `export fn mtypToLower(c: u8): u8;`
- `export fn mtypIsSpace(c: u8): bool;`
- `export fn mtypIsTokenChar(c: u8): bool;`
- `export fn mtypEqIgnoreCase(a: string, b: string): bool;`
- `export fn mtypEndsWithIgnoreCase(s: string, suffix: string): bool;`
- `export fn mtypContainsSlash(s: string): bool;`
- `export fn mtypTrimBounds(s: string, out_start: &mut usize, out_end: &mut usize);`
- `export fn mtypIsValidToken(s: string): bool;`
- `export fn mtypWriteStr(out: []u8, pos: &mut usize, s: string);`
- `export fn mtypExtractExt(path_or_ext: string): string?;`
- `export fn mtypLookupExt(ext: string): string?;`
- `export fn mtypExtensionForEssence(essence: string): string?;`
- `export fn parseMimeType(raw: string): MimeParts;`
- `export fn lookup(path_or_ext: string): string?;`
- `export fn extension(mime_type: string): string?;`
- `export fn charset(mime_type: string): string?;`
- `export fn mtypIsTopLevelType(t: string): bool;`
- `export fn contentType(type_or_ext: string, out: []u8): usize;`
- `export fn isCompressible(mime_type: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`mime-types@1.0.0`)，许可证：MIT
