[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# wordwrap

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap column-width text word wrapper with indentation, soft/hard modes, and newline preservation in pure CP

## 安装与导入

```bash
cap add wordwrap
```

```cp
import { wwrpIsInlineWs } from "wordwrap";
```

## 核心导出 API

- `export fn wwrpIsInlineWs(c: u8): bool;`
- `export fn wwrpPushByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn wwrpPushSpaces(out: []u8, pos: usize, count: usize): usize;`
- `export fn wwrpPushSlice(out: []u8, pos: usize, s: string): usize;`
- `export fn wwrpEmitWord(word: string, start_col: usize, max_w: usize, hard_mode: bool, out: []u8, start_pos: usize, cur_line_len: usize): WwrpEmitRes;`
- `export fn wrapText(input: string, start_col: usize, stop_col: usize, hard_mode: bool, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`wordwrap@1.0.0`)（https://github.com/substack/node-wordwrap），许可证：MIT
