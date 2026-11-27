[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# prepend-http

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation utility to prepend https:// or http:// to humanized URLs (npm prepend-http v4 parity) for CP

## 安装与导入

```bash
cap add prepend-http
```

```cp
import { phtIsSpace } from "prepend-http";
```

## 核心导出 API

- `export fn phtIsSpace(c: u8): bool;`
- `export fn phtTrim(s: string): string;`
- `export fn phtToLowerAscii(c: u8): u8;`
- `export fn phtIsAlpha(c: u8): bool;`
- `export fn phtIsSchemeChar(c: u8): bool;`
- `export fn phtStartsWith(s: string, prefix: string): bool;`
- `export fn phtStartsWithIgnoreCase(s: string, prefix_lower: string): bool;`
- `export fn phtCopyInto(src: string, out: []u8, offset: usize): usize;`
- `export fn isProtocolRelative(url: string): bool;`
- `export fn hasScheme(url: string): bool;`
- `export fn prependHttp(url: string, https: bool, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`prepend-http@1.0.0`)，许可证：MIT
