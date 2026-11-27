[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# indent-string

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap multi-line string indenter matching npm indent-string (180M+/wk) in pure CP

## 安装与导入

```bash
cap add indent-string
```

```cp
import { writeRepeat } from "indent-string";
```

## 核心导出 API

- `export fn writeRepeat(out: []u8, pos: usize, unit: string, count: usize): usize;`
- `export fn isLineBlank(input: string, start: usize, end: usize): bool;`
- `export fn indentStringWith(input: string, count: usize, indent_unit: string, include_empty_lines: bool, out: []u8): usize;`
- `export fn indentString(input: string, count: usize, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`indent-string@1.0.0`)，许可证：MIT
