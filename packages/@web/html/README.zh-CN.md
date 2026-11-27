[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @web/html

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Official HTML SSR, zero-heap XSS escaping, and HTMX helpers plugin for cap:web

## 安装与导入

```bash
cap add @web/html
```

```cp
import { htmAsciiLower } from "@web/html";
```

## 核心导出 API

- `export fn htmAsciiLower(c: u8): u8;`
- `export fn htmAsciiEqlIgnoreCase(a: string, b: string): bool;`
- `export fn htmWriteByte(out_buf: []u8, pos: &mut usize, b: u8): bool;`
- `export fn htmWriteRaw(out_buf: []u8, pos: &mut usize, s: string): bool;`
- `export fn htmEscapeInto(out_buf: []u8, pos: &mut usize, input: string): bool;`
- `export fn htmlEscape(input: string, out_buf: []u8): usize?;`
- `export fn htmlIsHtmx(ctx: &Context): bool;`
- `export fn htmlHxRedirect(url: string): Response;`
- `export fn htmlHxTrigger(body_html: string, event_name: string): Response;`
- `export fn htmAfterHook(ctx: &mut Context, resp: &mut Response);`
- `export fn registerHtml(app: &mut App, cfg: HtmlConfig): &mut App;`
