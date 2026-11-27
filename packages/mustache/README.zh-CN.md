[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# mustache

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap Mustache v1.4 logic-less template engine for CP with XSS-safe HTML escaping via @cap/escape-html (clean-room MIT implementation of npm mustache v4).

## 安装与导入

```bash
cap add mustache
```

```cp
import { renderMustache } from "mustache";
```

## 核心导出 API

- `export fn renderMustache(template: string, ctx: MustacheContext, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`mustache@1.0.0`)，许可证：MIT
