[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# strsim

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap string similarity & edit-distance metrics: Hamming, Levenshtein, OSA Damerau-Levenshtein, Jaro-Winkler, and Sorensen-Dice (clean-room port of Rust strsim)

## 安装与导入

```bash
cap add strsim
```

```cp
import { ssmMin3 } from "strsim";
```

## 核心导出 API

- `export fn ssmMin3(a: usize, b: usize, c: usize): usize;`
- `export fn hamming(a: string, b: string): usize?;`
- `export fn levenshtein(a: string, b: string): usize;`
- `export fn osaDistance(a: string, b: string): usize;`
- `export fn normalizedLevenshteinBps(a: string, b: string): u32;`
- `export fn jaroBps(a: string, b: string): u32;`
- `export fn jaroWinklerBps(a: string, b: string): u32;`
- `export fn sorensenDiceBps(a: string, b: string): u32;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`strsim@1.0.0`)，许可证：MIT
