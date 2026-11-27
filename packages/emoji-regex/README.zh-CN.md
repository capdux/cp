[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# emoji-regex

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation Unicode RGI Emoji UTF-8 scanner and extractor (npm emoji-regex parity) in pure CP

## 安装与导入

```bash
cap add emoji-regex
```

```cp
import { isEmojiCodePoint } from "emoji-regex";
```

## 核心导出 API

- `export fn isEmojiCodePoint(cp: u32): bool;`
- `export fn findNextEmoji(s: string, from_pos: usize): EmojiSpan?;`
- `export fn countEmojis(s: string): usize;`
- `export fn containsEmoji(s: string): bool;`
- `export fn stripEmojis(s: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`emoji-regex@1.0.0`)，许可证：MIT
