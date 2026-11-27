[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @web/s3

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Official S3 object key sanitizer and pure-CP AWS SigV4 presigned URL endpoint plugin for cap:web

## 安装与导入

```bash
cap add @web/s3
```

```cp
import { s3wRotr32 } from "@web/s3";
```

## 核心导出 API

- `export fn s3wRotr32(x: u32, n: u5): u32;`
- `export fn s3wSha256Compress(state: &mut [8]u32, block: &[64]u8);`
- `export fn s3wSha256(data: &[]u8, out: &mut [32]u8);`
- `export fn s3wHmacSha256(key: &[]u8, msg: &[]u8, out: &mut [32]u8);`
- `export fn s3wBytesToHex(bytes: &[32]u8, out_hex: &mut [64]u8);`
- `export fn s3wWriteByte(out_buf: []u8, pos: &mut usize, b: u8): bool;`
- `export fn s3wWriteRaw(out_buf: []u8, pos: &mut usize, s: string): bool;`
- `export fn s3wWriteU64(out_buf: []u8, pos: &mut usize, val: u64): bool;`
- `export fn s3wExtractHost(endpoint: string): string;`
- `export fn s3wTrimTrailingSlash(s: string): string;`
- `export fn s3wWriteJsonEscaped(out_buf: []u8, pos: &mut usize, s: string): bool;`
- `export fn s3NormalizeKey(raw_key: string, out_buf: []u8): usize?;`
- `export fn s3BuildPresignedUrl(cfg: S3PluginConfig, method: string, object_key: string, expires_secs: u64, amz_date: string, out_buf: []u8): usize?;`
- `export fn s3wPresignHandler(ctx: &mut Context): Response;`
- `export fn createS3Plugin(cfg: S3PluginConfig): Plugin;`
- `export fn registerS3(app: &mut App, cfg: S3PluginConfig): &mut App;`
