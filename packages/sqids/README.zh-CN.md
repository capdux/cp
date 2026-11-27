[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# sqids

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation YouTube-style reversible short ID generator (Sqids specification) with deterministic shuffle, min-length padding, and profanity blocklist for CP

## 安装与导入

```bash
cap add sqids
```

```cp
import { toLowerAscii } from "sqids";
```

## 核心导出 API

- `export enum SqidsError`
- `export fn toLowerAscii(c: u8): u8;`
- `export fn hasDigit(s: string): bool;`
- `export fn containsSlice(haystack: &[]u8, needle: &[]u8): bool;`
- `export fn startsWithSlice(haystack: &[]u8, prefix: &[]u8): bool;`
- `export fn endsWithSlice(haystack: &[]u8, suffix: &[]u8): bool;`
- `export fn isBlockedId(id: &[]u8): bool;`
- `export fn shuffleAlphabet(buf: []u8);`
- `export fn reverseBytes(buf: []u8);`
- `export fn indexOfByte(haystack: &[]u8, needle: u8): usize?;`
- `export fn appendToId(num: u64, alphabet: &[]u8, dst: []u8, pos: usize): usize?;`
- `export fn chunkToNumber(chunk: &[]u8, alphabet: &[]u8): u64?;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`sqids@1.0.0`)，许可证：MIT
