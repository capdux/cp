[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# rot13-cipher

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap ROT13, ROT47, arbitrary Caesar shift, and Atbash alphabet substitution utilities.

## 安装与导入

```bash
cap add rot13-cipher
```

```cp
import { shiftLetter } from "rot13-cipher";
```

## 核心导出 API

- `export fn shiftLetter(c: u8, norm_shift: u8): u8;`
- `export fn rot13(input: string, out: []u8): usize;`
- `export fn rot13InPlace(buf: []u8);`
- `export fn rot47(input: string, out: []u8): usize;`
- `export fn caesar(input: string, shift: i32, out: []u8): usize;`
- `export fn atbash(input: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`rot13-cipher@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`rot13@1.0.0`)，许可证：MIT
