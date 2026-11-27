[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# url-template

> 版本：`0.1.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation RFC 6570 URI Template (Levels 1-4) expander and validator for CP.

## 安装与导入

```bash
cap add url-template
```

```cp
import { isValid } from "url-template";
```

## 核心导出 API

- `export fn isValid(tmpl: string): bool;`
- `export fn countVars(tmpl: string): usize;`
- `export fn extractVarName(tmpl: string, index: usize, out: []u8): usize;`
- `export fn pctEncode(input: string, allow_reserved: bool, out: []u8): usize;`
- `export fn expand(tmpl: string, vars: &[]TemplateVar, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`url-template@0.1.0`)，许可证：MIT
