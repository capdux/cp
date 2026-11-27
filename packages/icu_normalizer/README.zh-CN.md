[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# icu_normalizer

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation UAX #15 Unicode NFC / NFD / NFKC / NFKD normalization, canonical combining class ordering, Hangul algorithmic (de)composition, and Latin/compatibility mappings (ICU4X icu_normalizer parity) in pure CP.

## 安装与导入

```bash
cap add icu_normalizer
```

```cp
import { inormDecodeUtf8 } from "icu_normalizer";
```

## 核心导出 API

- `export fn inormDecodeUtf8(s: string, pos: usize): InormUtf8Dec;`
- `export fn inormUtf8Len(cp: u32): usize;`
- `export fn inormEncodeUtf8(cp: u32, out: []u8, pos: usize): usize;`
- `export fn canonicalCombiningClass(cp: u32): u8;`
- `export fn hangulDecompose(cp: u32, out: []u32): usize;`
- `export fn hangulCompose(l: u32, v: u32, t: u32): u32;`
- `export fn inormHangulComposePair(first: u32, second: u32): u32;`
- `export fn inormWrite2(out: []u32, a: u32, b: u32): usize;`
- `export fn latinCanonicalDecompose(cp: u32, out: []u32): usize;`
- `export fn latinCanonicalCompose(base: u32, comb: u32): u32;`
- `export fn compatibilityDecompose(cp: u32, out: []u32): usize;`
- `export fn canonicalOrder(cps: []u32);`
- `export fn inormComposeInPlace(cps: []u32, len: usize): usize;`
- `export fn inormNormalizeCore(input: string, compat: bool, compose: bool, out: []u8): usize;`
- `export fn normalizeNfd(input: string, out: []u8): usize;`
- `export fn normalizeNfc(input: string, out: []u8): usize;`
- `export fn normalizeNfkd(input: string, out: []u8): usize;`
- `export fn normalizeNfkc(input: string, out: []u8): usize;`
- `export fn isNfc(input: string): bool;`
- `export fn isNfd(input: string): bool;`
- `export fn eqNfc(a: string, b: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`icu_normalizer@1.0.0`)，许可证：Unicode-3.0
