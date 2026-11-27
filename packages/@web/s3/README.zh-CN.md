[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @web/s3

Official S3 object key sanitizer and pure-CP AWS SigV4 presigned URL endpoint plugin for cap:web

| 元信息 | 值 |
| --- | --- |
| 包名 | `@web/s3` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @web/s3
```

```cp
import {
    S3PluginConfig,
    S3wSha256Hasher,
    s3wRotr32,
    s3wSha256Compress,
    s3wSha256,
    s3wHmacSha256,
    s3wBytesToHex,
    s3wWriteByte,
} from "@web/s3";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `S3PluginConfig` | `class` | 字段: `endpoint: string, bucket: string, region: string, access_key: string, secret_key: string, presign_path: string, +1 项` |
| `S3wSha256Hasher` | `class` | 字段: `state: [8]u32, buf: [64]u8, buf_len: usize, total_len: u64` · 方法: `init, update, finalize` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `s3wRotr32` | `(x: u32, n: u5): u32` | 调用 `s3wRotr32` 并返回 `u32`。 |
| `s3wSha256Compress` | `(state: &mut [8]u32, block: &[64]u8)` | 就地执行 `s3wSha256Compress` 操作。 |
| `s3wSha256` | `(data: &[]u8, out: &mut [32]u8)` | 就地执行 `s3wSha256` 操作。 |
| `s3wHmacSha256` | `(key: &[]u8, msg: &[]u8, out: &mut [32]u8)` | 就地执行 `s3wHmacSha256` 操作。 |
| `s3wBytesToHex` | `(bytes: &[32]u8, out_hex: &mut [64]u8)` | 就地执行 `s3wBytesToHex` 操作。 |
| `s3wWriteByte` | `(out_buf: []u8, pos: &mut usize, b: u8): bool` | 调用 `s3wWriteByte` 并返回 `bool`。 |
| `s3wWriteRaw` | `(out_buf: []u8, pos: &mut usize, s: string): bool` | 调用 `s3wWriteRaw` 并返回 `bool`。 |
| `s3wWriteU64` | `(out_buf: []u8, pos: &mut usize, val: u64): bool` | 调用 `s3wWriteU64` 并返回 `bool`。 |
| `s3wExtractHost` | `(endpoint: string): string` | 调用 `s3wExtractHost` 并返回 `string`。 |
| `s3wTrimTrailingSlash` | `(s: string): string` | 调用 `s3wTrimTrailingSlash` 并返回 `string`。 |
| `s3wWriteJsonEscaped` | `(out_buf: []u8, pos: &mut usize, s: string): bool` | 调用 `s3wWriteJsonEscaped` 并返回 `bool`。 |
| `s3NormalizeKey` | `(raw_key: string, out_buf: []u8): usize?` | 调用 `s3NormalizeKey` 并返回 `usize?`。 |
| `s3BuildPresignedUrl` | `(cfg: S3PluginConfig, method: string, object_key: string, expires_secs: u64, amz_date: string, out_buf: []u8): usize?` | 调用 `s3BuildPresignedUrl` 并返回 `usize?`。 |
| `s3wPresignHandler` | `(ctx: &mut Context): Response` | 调用 `s3wPresignHandler` 并返回 `Response`。 |
| `createS3Plugin` | `(cfg: S3PluginConfig): Plugin` | 创建或初始化实例，返回 `Plugin`。 |
| `registerS3` | `(app: &mut App, cfg: S3PluginConfig): &mut App` | 将插件或中间件注册到目标应用实例。 |

## 最小调用示例

```cp
import { s3wWriteRaw } from "@web/s3";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = s3wWriteRaw(out_buf[0..256], /* pos: &mut usize */, "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
