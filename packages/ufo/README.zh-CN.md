[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# ufo

> 版本：`1.5.4` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap URL protocol detection, slash normalization, and safe URL joining utilities

## 安装与导入

```bash
cap add ufo
```

```cp
import { ufoIsAlpha } from "ufo";
```

## 核心导出 API

- `export fn ufoIsAlpha(c: u8): bool;`
- `export fn ufoIsSchemeChar(c: u8): bool;`
- `export fn ufoProtocolEnd(url: string): usize;`
- `export fn ufoCopyBytes(src: string, out: []u8, offset: usize): usize;`
- `export fn hasProtocol(url: string): bool;`
- `export fn isRelative(url: string): bool;`
- `export fn withoutLeadingSlash(s: string): string;`
- `export fn withoutTrailingSlash(s: string): string;`
- `export fn withLeadingSlash(s: string, out: []u8): usize;`
- `export fn withTrailingSlash(s: string, out: []u8): usize;`
- `export fn cleanDoubleSlashes(url: string, out: []u8): usize;`
- `export fn joinUrl(base: string, segment: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`ufo@1.5.4`)（https://github.com/unjs/ufo），许可证：MIT
