[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# vte

> 版本：`1.0.0` | 许可证：`Apache-2.0` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation ANSI/VT100 terminal escape sequence state machine parser (Paul Williams model) in pure CP

## 安装与导入

```bash
cap add vte
```

```cp
import { vtepZeroParams } from "vte";
```

## 核心导出 API

- `export enum VteActionKind`
- `export enum VteState`
- `export fn vtepZeroParams(): [8]u16;`
- `export fn vtepZeroOsc(): [64]u8;`
- `export fn vtepIsExecute(b: u8): bool;`
- `export fn vtepAddDigit(cur: u16, digit: u8): u16;`
- `export fn vtepParseOscCmd(buf: &[]u8, len: usize): u16;`
- `export fn vtepHasOscCmd(buf: &[]u8, len: usize): bool;`
- `export fn vtepMakeEvent(kind: VteActionKind, ch: u8, final_byte: u8, param_count: usize, params: [8]u16, intermediate: u8): VteEvent;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`vte@1.0.0`)，许可证：Apache-2.0
