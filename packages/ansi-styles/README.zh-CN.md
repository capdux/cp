[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# ansi-styles

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

ANSI escape codes for styling strings in the terminal (clean-room CP implementation of npm ansi-styles)

## 安装与导入

```bash
cap add ansi-styles
```

```cp
import { asPushStr } from "ansi-styles";
```

## 核心导出 API

- `export fn asPushStr(out: []u8, pos: usize, s: string): usize;`
- `export fn asPushU8(out: []u8, pos: usize, v: u8): usize;`
- `export fn wrapStyle(style: StylePair, text: string, out: []u8): usize;`
- `export fn formatAnsi256(code: u8, is_bg: bool, out: []u8): usize;`
- `export fn formatAnsi16m(r: u8, g: u8, b: u8, is_bg: bool, out: []u8): usize;`
- `export fn formatRgbToAnsi16(r: u8, g: u8, b: u8, is_bg: bool, out: []u8): usize;`
- `export fn formatRgbToAnsi256(r: u8, g: u8, b: u8, is_bg: bool, out: []u8): usize;`
- `export fn formatHexToAnsi16m(hex: string, is_bg: bool, out: []u8): usize;`
- `export fn formatKeywordToAnsi16m(name: string, is_bg: bool, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`ansi-styles@1.0.0`)，许可证：MIT
