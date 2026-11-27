[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @web/proxy

Official zero-heap HTTP reverse proxy plugin for cap:web (prefix stripping, traversal guard, HTTP/1.1 formatter, pluggable transport)

| 元信息 | 值 |
| --- | --- |
| 包名 | `@web/proxy` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @web/proxy
```

```cp
import {
    ProxyConfig,
    ProxyRequestPlan,
    pxAsciiLower,
    pxWriteRaw,
    pxWriteByte,
    pxWriteU64,
    pxHasControlChars,
    pxHasTraversal,
} from "@web/proxy";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ProxyConfig` | `class` | 字段: `prefix: string, upstream_host: string, upstream_port: u16, strip_prefix: bool, add_forwarded_headers: bool` |
| `ProxyRequestPlan` | `class` | 字段: `target_path: [256]u8, target_path_len: usize, host_header: [128]u8, host_header_len: usize, forwarded_for: string, forwarded_proto: string` · 方法: `targetPathSlice, hostHeaderSlice` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `pxAsciiLower` | `(c: u8): u8` | 调用 `pxAsciiLower` 并返回 `u8`。 |
| `pxWriteRaw` | `(out_buf: []u8, pos: &mut usize, s: string): bool` | 调用 `pxWriteRaw` 并返回 `bool`。 |
| `pxWriteByte` | `(out_buf: []u8, pos: &mut usize, b: u8): bool` | 调用 `pxWriteByte` 并返回 `bool`。 |
| `pxWriteU64` | `(out_buf: []u8, pos: &mut usize, val: u64): bool` | 调用 `pxWriteU64` 并返回 `bool`。 |
| `pxHasControlChars` | `(s: string): bool` | 调用 `pxHasControlChars` 并返回 `bool`。 |
| `pxHasTraversal` | `(path: string): bool` | 调用 `pxHasTraversal` 并返回 `bool`。 |
| `pxTrimTrailingSlash` | `(s: string): string` | 调用 `pxTrimTrailingSlash` 并返回 `string`。 |
| `pxMatchesPrefix` | `(req_path: string, raw_prefix: string): bool` | 调用 `pxMatchesPrefix` 并返回 `bool`。 |
| `pxDefaultTransport` | `(cfg: &ProxyConfig, ctx: &mut Context, target_path: string): Response` | 调用 `pxDefaultTransport` 并返回 `Response`。 |
| `buildProxyTargetPath` | `(req_path: string, raw_query: string, cfg: ProxyConfig, out_buf: []u8): usize?` | 调用 `buildProxyTargetPath` 并返回 `usize?`。 |
| `buildProxyRequestPlan` | `(req_path: string, raw_query: string, cfg: ProxyConfig, client_ip: string): ProxyRequestPlan?` | 调用 `buildProxyRequestPlan` 并返回 `ProxyRequestPlan?`。 |
| `formatUpstreamHttp1Request` | `(method: AppMethod, target_path: string, body: string, cfg: ProxyConfig, client_ip: string, out_buf: []u8): usize?` | 通过 `formatUpstreamHttp1Request` 编码或格式化输出，返回 `usize?`。 |
| `proxyHook` | `(ctx: &mut Context): bool` | 调用 `proxyHook` 并返回 `bool`。 |
| `createProxyPlugin` | `(cfg: ProxyConfig, transport: ProxyTransportFn?): Plugin` | 创建或初始化实例，返回 `Plugin`。 |
| `registerProxy` | `(app: &mut App, cfg: ProxyConfig): &mut App` | 将插件或中间件注册到目标应用实例。 |
| `registerProxyWithTransport` | `(app: &mut App, cfg: ProxyConfig, transport: ProxyTransportFn): &mut App` | 将插件或中间件注册到目标应用实例。 |

## 最小调用示例

```cp
import { formatUpstreamHttp1Request } from "@web/proxy";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatUpstreamHttp1Request(/* method: AppMethod */, "example", "example", /* cfg: ProxyConfig */, "example", out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
