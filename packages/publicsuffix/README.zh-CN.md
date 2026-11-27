[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# publicsuffix

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap pure-CP Public Suffix List (PSL) parser for eTLD and registrable domain extraction with custom wildcard/exception rules (crates.io publicsuffix equivalent)

## 安装与导入

```bash
cap add publicsuffix
```

```cp
import { pslToLower } from "publicsuffix";
```

## 核心导出 API

- `export fn pslToLower(b: u8): u8;`
- `export fn pslIsLabelChar(b: u8): bool;`
- `export fn pslBufEqLowerStr(buf: [256]u8, start: usize, len: usize, s: string): bool;`
- `export fn pslCountPatternLabels(pat: string): usize;`
- `export fn pslMatchSuffixPattern(norm: [256]u8, norm_len: usize, label_starts: [128]usize, label_count: usize, pat: string): usize;`
- `export fn pslMatchBuiltin3(norm: [256]u8, norm_len: usize, label_starts: [128]usize, label_count: usize): bool;`
- `export fn pslMatchBuiltin2(norm: [256]u8, norm_len: usize, label_starts: [128]usize, label_count: usize): bool;`
- `export fn pslMatchBuiltin1(norm: [256]u8, norm_len: usize, label_starts: [128]usize, label_count: usize): bool;`
- `export fn parseDomain(host: string): DomainInfo?;`
- `export fn suffix(host: string): string?;`
- `export fn registrableDomain(host: string): string?;`
- `export fn isPublicSuffix(host: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`publicsuffix@1.0.0`)，许可证：MIT OR Apache-2.0
