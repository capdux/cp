[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# pad

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation UTF-8 codepoint-aware string padding (padStart, padEnd, padCenter) and repeatStr in pure CP

## 安装与导入

```bash
cap add pad
```

```cp
import { countChars } from "pad";
```

## 核心导出 API

- `export fn countChars(s: string): usize;`
- `export fn copyBytes(dst: []u8, pos: usize, src: string);`
- `export fn fillByte(dst: []u8, pos: usize, count: usize, ch: u8);`
- `export fn padStart(s: string, target_chars: usize, pad_char: u8, out: []u8): usize;`
- `export fn padEnd(s: string, target_chars: usize, pad_char: u8, out: []u8): usize;`
- `export fn padCenter(s: string, target_chars: usize, pad_char: u8, out: []u8): usize;`
- `export fn repeatStr(s: string, count: usize, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`pad@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`pad@1.0.0`)，许可证：MIT
- **上游生态**：`npm` (`left-pad@1.0.0`)，许可证：MIT
