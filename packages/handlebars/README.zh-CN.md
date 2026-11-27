[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# handlebars

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation Handlebars template engine (npm handlebars v4 parity) composing escape-html for XSS-safe rendering in CP

## 安装与导入

```bash
cap add handlebars
```

```cp
import { renderHandlebars } from "handlebars";
```

## 核心导出 API

- `export fn renderHandlebars(tpl: string, ctx: HbsContext, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`handlebars@1.0.0`)，许可证：MIT
