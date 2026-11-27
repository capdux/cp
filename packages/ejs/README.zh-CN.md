[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# ejs

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap EJS template engine supporting <%= escaped %>, <%- raw %>, <%# comments %>, <% if/else %>, and <% repeat %> tags (clean-room port of npm ejs)

## 安装与导入

```bash
cap add ejs
```

```cp
import { ejsWriteStr } from "ejs";
```

## 核心导出 API

- `export fn ejsWriteStr(s: string, out: []u8, pos: usize): usize;`
- `export fn ejsLookupStr(ctx: EjsContext, key: string): string;`
- `export fn ejsLookupBool(ctx: EjsContext, key: string): bool;`
- `export fn ejsLookupCount(ctx: EjsContext, key: string): usize;`
- `export fn ejsFindTagOpen(tpl: string, start_pos: usize): usize;`
- `export fn ejsFindTagClose(tpl: string, start_pos: usize): usize;`
- `export fn ejsFindBlockBounds(tpl: string, body_start: usize, out_pos: []usize): bool;`
- `export fn ejsRenderSlice(tpl: string, ctx: EjsContext, out: []u8, start_out: usize, depth: usize): usize;`
- `export fn renderEjs(tpl: string, ctx: EjsContext, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`ejs@1.0.0`)，许可证：Apache-2.0
