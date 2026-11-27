[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# language-tags

> 版本：`1.0.9` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap BCP 47 / RFC 5646 language tag parser, canonical formatter, and RFC 4647 range matcher in pure CP

## 安装与导入

```bash
cap add language-tags
```

```cp
import { ltgIsAlpha } from "language-tags";
```

## 核心导出 API

- `export fn ltgIsAlpha(c: u8): bool;`
- `export fn ltgIsDigit(c: u8): bool;`
- `export fn ltgIsAlnum(c: u8): bool;`
- `export fn ltgIsSep(c: u8): bool;`
- `export fn ltgToLower(c: u8): u8;`
- `export fn ltgToUpper(c: u8): u8;`
- `export fn ltgAllAlpha(s: string): bool;`
- `export fn ltgAllDigit(s: string): bool;`
- `export fn ltgAllAlnum(s: string): bool;`
- `export fn ltgInvalidTag(): Bcp47Tag;`
- `export fn parseLanguageTag(tag: string): Bcp47Tag;`
- `export fn ltgPushByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn formatCanonicalTag(t: Bcp47Tag, out: []u8): usize;`
- `export fn matchesLanguageRange(tag: string, range: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`language-tags@1.0.9`)（https://github.com/mattcg/language-tags），许可证：MIT
