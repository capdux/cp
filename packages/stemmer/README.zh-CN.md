[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# stemmer

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation Martin Porter (1980) English Stemmer (Steps 1a-5b) in pure CP

## 安装与导入

```bash
cap add stemmer
```

```cp
import { toLowerAscii } from "stemmer";
```

## 核心导出 API

- `export fn toLowerAscii(b: u8): u8;`
- `export fn isConsonant(buf: &[]u8, idx: usize): bool;`
- `export fn measurePrefix(buf: &[]u8, end: usize): usize;`
- `export fn measure(word: &[]u8): usize;`
- `export fn containsVowel(buf: &[]u8, end: usize): bool;`
- `export fn endsDoubleConsonant(buf: &[]u8, end: usize): bool;`
- `export fn endsCvc(buf: &[]u8, end: usize): bool;`
- `export fn endsWith(buf: &[]u8, len: usize, suffix: string): bool;`
- `export fn replaceSuffix(buf: []u8, len: usize, suf_len: usize, rep: string): usize;`
- `export fn tryStep2(buf: []u8, len: usize): usize;`
- `export fn tryStep3(buf: []u8, len: usize): usize;`
- `export fn tryStep4(buf: []u8, len: usize): usize;`
- `export fn stem(word: string, out: []u8): usize;`
- `export fn sameStem(a: string, b: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`stemmer@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`rust-stemmers@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`porter-stemmer@1.0.0`)，许可证：MIT
