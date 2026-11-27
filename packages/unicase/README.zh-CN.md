[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# unicase

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap Unicode & ASCII case-insensitive comparison, ordering, matching, and hashing (clean-room port of Rust unicase)

## 安装与导入

```bash
cap add unicase
```

```cp
import { ucDecodeUtf8 } from "unicase";
```

## 核心导出 API

- `export fn ucDecodeUtf8(s: string, pos: usize, step_out: []usize): u32;`
- `export fn ucEncodeUtf8(cp: u32, out: []u8, pos: usize): usize;`
- `export fn foldAsciiByte(b: u8): u8;`
- `export fn foldCodepoint(cp: u32, folded_out: []u32): usize;`
- `export fn caseFold(src: string, out: []u8): usize;`
- `export fn eqAscii(a: string, b: string): bool;`
- `export fn cmpAscii(a: string, b: string): i32;`
- `export fn cmpUnicode(a: string, b: string): i32;`
- `export fn eqUnicode(a: string, b: string): bool;`
- `export fn hashAscii(s: string): u64;`
- `export fn hashUnicode(s: string): u64;`
- `export fn startsWithIgnoreCase(haystack: string, prefix: string): bool;`
- `export fn endsWithIgnoreCase(haystack: string, suffix: string): bool;`
- `export fn containsIgnoreCase(haystack: string, needle: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`unicase@1.0.0`)，许可证：MIT
