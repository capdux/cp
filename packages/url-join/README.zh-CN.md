[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# url-join

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation smart URL joiner, slash/dot-segment path normalizer, query merger, and trailing-slash utilities (url-join / ufo parity) for CP

## 安装与导入

```bash
cap add url-join
```

```cp
import { pushByte } from "url-join";
```

## 核心导出 API

- `export fn pushByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn isSchemeChar(c: u8): bool;`
- `export fn schemePrefixLen(s: string): usize;`
- `export fn splitPart(p: string): PartSplit;`
- `export fn trimLeadingSlashes(s: string): string;`
- `export fn trimTrailingSlashes(s: string): string;`
- `export fn urlJoin(parts: []string, out: []u8): string;`
- `export fn normalizeUrlPath(path: string, out: []u8): string;`
- `export fn withQuery(base_url: string, key: string, value: string, out: []u8): string;`
- `export fn findQueryOrHash(url: string): usize;`
- `export fn withTrailingSlash(url: string, out: []u8): string;`
- `export fn withoutTrailingSlash(url: string, out: []u8): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`url-join@1.0.0`)，许可证：MIT
