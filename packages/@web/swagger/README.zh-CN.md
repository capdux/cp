[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @web/swagger

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Official OpenAPI 3.1.0 & interactive Scalar/Swagger UI documentation plugin for cap:web

## 安装与导入

```bash
cap add @web/swagger
```

```cp
import { swAsciiLower } from "@web/swagger";
```

## 核心导出 API

- `export fn swAsciiLower(c: u8): u8;`
- `export fn swAsciiEqlIgnoreCase(a: string, b: string): bool;`
- `export fn swWriteByte(out_buf: []u8, pos: &mut usize, b: u8): bool;`
- `export fn swWriteRaw(out_buf: []u8, pos: &mut usize, s: string): bool;`
- `export fn swWriteHtmlEscaped(out_buf: []u8, pos: &mut usize, s: string): bool;`
- `export fn renderDocsHtml(cfg: SwaggerConfig, out_buf: []u8): usize?;`
- `export fn refreshSpec(app: &mut App): usize?;`
- `export fn cachedSpec(): string;`
- `export fn cachedDocsHtml(): string;`
- `export fn swOpenApiSpecHandler(ctx: &mut Context): Response;`
- `export fn swDocsUiHandler(ctx: &mut Context): Response;`
- `export fn registerSwagger(app: &mut App, cfg: SwaggerConfig): &mut App;`
