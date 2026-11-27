[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @web/compress

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Official HTTP response compression plugin for cap:web with pure-CP zero-heap RFC 1952 Gzip encoder

## 安装与导入

```bash
cap add @web/compress
```

```cp
import { cmpAsciiLower } from "@web/compress";
```

## 核心导出 API

- `export enum EncodingKind`
- `export fn cmpAsciiLower(c: u8): u8;`
- `export fn cmpEqlIgnoreCase(a: string, b: string): bool;`
- `export fn cmpTrimSpaces(s: string): string;`
- `export fn cmpParseQValue(raw: string): i32;`
- `export fn negotiateEncoding(accept_encoding: string, prefer_gzip: bool): EncodingKind;`
- `export fn cmpCrc32(data: string): u32;`
- `export fn cmpWriteU16Le(out_buf: []u8, pos: usize, val: u16);`
- `export fn cmpWriteU32Le(out_buf: []u8, pos: usize, val: u32);`
- `export fn encodeGzipStored(src: string, out_buf: []u8): usize?;`
- `export fn compressResponseHook(ctx: &mut Context, resp: &mut Response);`
- `export fn createCompressPlugin(cfg: CompressConfig): Plugin;`
- `export fn registerCompress(app: &mut App, cfg: CompressConfig): &mut App;`
