[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @web/multipart

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Official multipart/form-data parser and payload guard plugin for cap:web

## 安装与导入

```bash
cap add @web/multipart
```

```cp
import { mpAsciiLower } from "@web/multipart";
```

## 核心导出 API

- `export fn mpAsciiLower(c: u8): u8;`
- `export fn mpStartsWithIgnoreCase(s: string, prefix: string): bool;`
- `export fn mpEqlIgnoreCase(a: string, b: string): bool;`
- `export fn mpTrimSpaces(s: string): string;`
- `export fn mpParseUsize(s: string): usize?;`
- `export fn mpMatchAt(haystack: string, pos: usize, needle: string): bool;`
- `export fn mpFindBoundary(body: string, start_pos: usize, boundary: string): usize?;`
- `export fn mpFindHeaderEnd(part: string, body_start_out: &mut usize): usize?;`
- `export fn mpExtractParam(header_val: string, key: string): string?;`
- `export fn parseMultipartBody(boundary: string, body: string, cfg: MultipartConfig, out_form: &mut MultipartForm): i32;`
- `export fn multipartGuardHook(ctx: &mut Context): bool;`
- `export fn createMultipartPlugin(cfg: MultipartConfig): Plugin;`
- `export fn registerMultipart(app: &mut App, cfg: MultipartConfig): &mut App;`
