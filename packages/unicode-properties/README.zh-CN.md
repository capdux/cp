[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# unicode-properties

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation Unicode character properties, UAX #31 identifier validation (XID_Start/XID_Continue), White_Space, Default_Ignorable, and UTS #51 Emoji classification in pure CP.

## 安装与导入

```bash
cap add unicode-properties
```

```cp
import { decodeUtf8Strict } from "unicode-properties";
```

## 核心导出 API

- `export fn decodeUtf8Strict(s: string, pos: usize, next_pos: &mut usize): u32;`
- `export fn isXidStart(cp: u32): bool;`
- `export fn isXidContinue(cp: u32): bool;`
- `export fn isUnicodeWhitespace(cp: u32): bool;`
- `export fn isEmojiPresentation(cp: u32): bool;`
- `export fn isEmoji(cp: u32): bool;`
- `export fn isDefaultIgnorable(cp: u32): bool;`
- `export fn isValidIdentifier(s: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`unicode-properties@1.0.0`)，许可证：MIT
