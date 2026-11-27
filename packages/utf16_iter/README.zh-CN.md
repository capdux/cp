[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# utf16_iter

> 版本：`1.0.0` | 许可证：`Apache-2.0 OR MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation bidirectional UTF-16 code point iterator, Latin-1 to UTF-16 expander, and UTF-16 to UTF-8 transcoder in CP

## 安装与导入

```bash
cap add utf16_iter
```

```cp
import { u16iIsHighSurrogate } from "utf16_iter";
```

## 核心导出 API

- `export fn u16iIsHighSurrogate(unit: u16): bool;`
- `export fn u16iIsLowSurrogate(unit: u16): bool;`
- `export fn u16iCombineSurrogates(high: u16, low: u16): u32;`
- `export fn u16iEncodeUtf8(cp: u32, out: []u8, offset: usize): usize;`
- `export fn countUtf16Chars(slice: &[]u16): usize;`
- `export fn utf16ToUtf8(slice: &[]u16, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`utf16_iter@1.0.0`)，许可证：Apache-2.0 OR MIT
