[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @web/compress

Official HTTP response compression plugin for cap:web with pure-CP zero-heap RFC 1952 Gzip encoder

| 元信息 | 值 |
| --- | --- |
| 包名 | `@web/compress` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @web/compress
```

```cp
import {
    EncodingKind,
    CompressConfig,
    cmpAsciiLower,
    cmpEqlIgnoreCase,
    cmpTrimSpaces,
    cmpParseQValue,
    negotiateEncoding,
    cmpCrc32,
} from "@web/compress";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `EncodingKind` | `None, Gzip, Deflate` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `CompressConfig` | `class` | 字段: `min_bytes: usize, prefer_gzip: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `cmpAsciiLower` | `(c: u8): u8` | 调用 `cmpAsciiLower` 并返回 `u8`。 |
| `cmpEqlIgnoreCase` | `(a: string, b: string): bool` | 调用 `cmpEqlIgnoreCase` 并返回 `bool`。 |
| `cmpTrimSpaces` | `(s: string): string` | 调用 `cmpTrimSpaces` 并返回 `string`。 |
| `cmpParseQValue` | `(raw: string): i32` | 调用 `cmpParseQValue` 并返回 `i32`。 |
| `negotiateEncoding` | `(accept_encoding: string, prefer_gzip: bool): EncodingKind` | 调用 `negotiateEncoding` 并返回 `EncodingKind`。 |
| `cmpCrc32` | `(data: string): u32` | 调用 `cmpCrc32` 并返回 `u32`。 |
| `cmpWriteU16Le` | `(out_buf: []u8, pos: usize, val: u16)` | 就地执行 `cmpWriteU16Le` 操作。 |
| `cmpWriteU32Le` | `(out_buf: []u8, pos: usize, val: u32)` | 就地执行 `cmpWriteU32Le` 操作。 |
| `encodeGzipStored` | `(src: string, out_buf: []u8): usize?` | 通过 `encodeGzipStored` 编码或格式化输出，返回 `usize?`。 |
| `compressResponseHook` | `(ctx: &mut Context, resp: &mut Response)` | 就地执行 `compressResponseHook` 操作。 |
| `createCompressPlugin` | `(cfg: CompressConfig): Plugin` | 创建或初始化实例，返回 `Plugin`。 |
| `registerCompress` | `(app: &mut App, cfg: CompressConfig): &mut App` | 将插件或中间件注册到目标应用实例。 |

## 最小调用示例

```cp
import { encodeGzipStored } from "@web/compress";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = encodeGzipStored("example", out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
