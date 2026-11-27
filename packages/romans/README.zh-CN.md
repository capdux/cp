[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# romans

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation strict Roman numeral (1..3999) bidirectional parser, validator, and formatter in pure CP

## 安装与导入

```bash
cap add romans
```

```cp
import { romanCharVal } from "romans";
```

## 核心导出 API

- `export fn romanCharVal(b: u8): i32;`
- `export fn toRoman(num: u16, out: []u8): usize;`
- `export fn parseRoman(input: string): i32;`
- `export fn isValidRoman(input: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`romans@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`roman@1.0.0`)，许可证：MIT
