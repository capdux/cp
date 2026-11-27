[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @atproto/syntax

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation AT Protocol / Bluesky at:// URI, NSID, and Record Key (rkey) validator, parser, and formatter in pure CP

## 安装与导入

```bash
cap add @atproto/syntax
```

```cp
import { invalidAtUri } from "@atproto/syntax";
```

## 核心导出 API

- `export fn invalidAtUri(): AtUri;`
- `export fn isLowerAlpha(b: u8): bool;`
- `export fn isAlpha(b: u8): bool;`
- `export fn isDigit(b: u8): bool;`
- `export fn isAlphaNum(b: u8): bool;`
- `export fn isValidNsid(nsid: string): bool;`
- `export fn isValidRkey(rkey: string): bool;`
- `export fn isValidDid(s: string): bool;`
- `export fn isValidHandle(s: string): bool;`
- `export fn parseAtUri(uri: string): AtUri;`
- `export fn pushSlice(dst: []u8, pos: usize, src: string): i64;`
- `export fn formatAtUri(authority: string, collection: string, rkey: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`@atproto/syntax@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`atrium-api@1.0.0`)，许可证：MIT
