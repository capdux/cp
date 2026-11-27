[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# unicode-bidi

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation Unicode Bidirectional Algorithm (UAX #9) classifier, base direction resolver, visual run splitter, and bracket mirroring for CP

## 安装与导入

```bash
cap add unicode-bidi
```

```cp
import { decodeUtf8At } from "unicode-bidi";
```

## 核心导出 API

- `export fn decodeUtf8At(text: string, pos: usize): Utf8Step;`
- `export fn bidiClass(cp: u32): u8;`
- `export fn baseDirection(text: string): u8;`
- `export fn hasRtl(text: string): bool;`
- `export fn isLtrOnly(text: string): bool;`
- `export fn resolveCharLevel(cls: u8, base_level: u8, prev_level: u8): u8;`
- `export fn computeVisualRuns(text: string, out_runs: []BidiRun): usize;`
- `export fn mirrorAsciiBracket(ch: u8): u8;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`unicode-bidi@1.0.0`)，许可证：MIT
- **上游生态**：`npm` (`bidi-js@1.0.0`)，许可证：MIT
