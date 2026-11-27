[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# optionator

> 版本：`0.9.4` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap CLI option specification and auto-aligned --help documentation generator in pure CP

## 安装与导入

```bash
cap add optionator
```

```cp
import { optnLeftColLen } from "optionator";
```

## 核心导出 API

- `export fn optnLeftColLen(spec: OptionSpec): usize;`
- `export fn formatOptionRow(spec: OptionSpec, col_width: usize, out: []u8): usize;`
- `export fn generateHelp(program: string, summary: string, specs: &[]OptionSpec, out: []u8): usize;`
- `export fn findOptionSpec(specs: &[]OptionSpec, flag: string): OptionSpec?;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`optionator@0.9.4`)（https://github.com/gkz/optionator），许可证：MIT
