[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# redent

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation strip-and-reindent multiline string utility (npm redent parity) composing strip-indent and indent-string for CP

## 安装与导入

```bash
cap add redent
```

```cp
import { redentWithIndent } from "redent";
```

## 核心导出 API

- `export fn redentWithIndent(s: string, count: usize, indent: string, include_empty_lines: bool, out: []u8): usize;`
- `export fn redent(s: string, count: usize, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`redent@1.0.0`)，许可证：MIT
