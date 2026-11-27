[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# unicode-script

> 版本：`1.0.0` | 许可证：`MIT OR Apache-2.0` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap UAX #24 Unicode Script property classifier and dominant script detector in pure CP (modeled after Rust unicode-script)

## 安装与导入

```bash
cap add unicode-script
```

```cp
import { uscScriptIndex } from "unicode-script";
```

## 核心导出 API

- `export enum Script`
- `export fn uscScriptIndex(s: Script): usize;`
- `export fn uscIndexToScript(idx: usize): Script;`
- `export fn scriptOf(cp: u32): Script;`
- `export fn scriptName(s: Script): string;`
- `export fn isLatin(cp: u32): bool;`
- `export fn isHan(cp: u32): bool;`
- `export fn isCjkScript(s: Script): bool;`
- `export fn uscDecodeUtf8(text: string, pos: usize): UscDecoded;`
- `export fn dominantScript(text: string): Script;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`unicode-script@1.0.0`)，许可证：MIT OR Apache-2.0
