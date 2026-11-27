[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# fast-uri

> 版本：`1.0.0` | 许可证：`BSD-3-Clause` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation RFC 3986 URI parser, serializer, relative resolver, and equivalence comparator (clean-room implementation of npm fast-uri, #171 / 229M wk)

## 安装与导入

```bash
cap add fast-uri
```

```cp
import { furiIsAlpha } from "fast-uri";
```

## 核心导出 API

- `export fn furiIsAlpha(b: u8): bool;`
- `export fn furiIsDigit(b: u8): bool;`
- `export fn furiIsHex(b: u8): bool;`
- `export fn furiIsSchemeChar(b: u8): bool;`
- `export fn furiToLower(b: u8): u8;`
- `export fn furiToUpper(b: u8): u8;`
- `export fn furiPush(out: []u8, pos: usize, b: u8): usize;`
- `export fn furiPushSlice(out: []u8, pos: usize, s: string): usize;`
- `export fn furiPushLower(out: []u8, pos: usize, s: string): usize;`
- `export fn furiPushPercentUpper(out: []u8, pos: usize, s: string): usize;`
- `export fn furiPushPort(out: []u8, pos: usize, port: i32): usize;`
- `export fn furiStrEqLower(a: string, b_lower: string): bool;`
- `export fn furiDefaultPort(scheme: string): i32;`
- `export fn furiHasColon(s: string): bool;`
- `export fn furiValidateIpv6Inner(s: string): bool;`
- `export fn furiParsePortStr(s: string): i32;`
- `export fn parseUri(input: string): UriComponents;`
- `export fn serializeUri(c: UriComponents, out: []u8): usize;`
- `export fn furiHasPrefix(s: string, p: string): bool;`
- `export fn furiPopSegment(out: []u8, out_len: usize): usize;`
- `export fn furiRemoveDotSegments(input: string, out: []u8): usize;`
- `export fn resolveUri(base: string, rel: string, out: []u8): usize;`
- `export fn furiNormalizeForCompare(input: string, out: []u8): usize;`
- `export fn equalUri(a: string, b: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`fast-uri@1.0.0`)，许可证：BSD-3-Clause
