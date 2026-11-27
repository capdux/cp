[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# iri-string

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation RFC 3986 / RFC 3987 URI/IRI parser, dot-segment remover, and relative reference resolver for CP

## 安装与导入

```bash
cap add iri-string
```

```cp
import { invalidIri } from "iri-string";
```

## 核心导出 API

- `export fn invalidIri(): IriParts;`
- `export fn isAsciiAlpha(c: u8): bool;`
- `export fn isSchemeChar(c: u8): bool;`
- `export fn parseIri(s: string): IriParts;`
- `export fn remLen(path: string, cur: RemCursor): usize;`
- `export fn remAt(path: string, cur: RemCursor, idx: usize): u8;`
- `export fn remAdvance(cur: &mut RemCursor, n: usize);`
- `export fn popLastSegment(out: []u8, out_len: usize): usize;`
- `export fn removeDotSegments(path: string, out: []u8): usize;`
- `export fn obPush(ob: &mut OutBuf, out: []u8, ch: u8);`
- `export fn obWrite(ob: &mut OutBuf, out: []u8, src: string);`
- `export fn resolveReference(base: string, rel: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`iri-string@1.0.0`)，许可证：MIT
