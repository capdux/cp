[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# accept-language-parser

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap RFC 9110 / BCP-47 Accept-Language quality weight parser and language negotiation engine.

## 安装与导入

```bash
cap add accept-language-parser
```

```cp
import { isWs } from "accept-language-parser";
```

## 核心导出 API

- `export fn isWs(c: u8): bool;`
- `export fn toLowerAscii(c: u8): u8;`
- `export fn eqIgnoreCase(a: string, b: string): bool;`
- `export fn isValidTag(tag: string): bool;`
- `export fn parseQuality(s: string): i32;`
- `export fn parseAcceptLanguage(header: string, out: []LangRange): usize;`
- `export fn matchesTag(range_tag: string, supported_tag: string): bool;`
- `export fn isExcluded(ranges: &[]LangRange, count: usize, candidate: string): bool;`
- `export fn pickLanguage(header: string, supported: &[]string): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`accept-language-parser@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`accept-language@1.0.0`)，许可证：MIT
