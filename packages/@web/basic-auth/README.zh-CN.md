[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @web/basic-auth

Official HTTP Basic Auth (RFC 7617) and X-API-Key constant-time authentication plugin for cap:web

| 元信息 | 值 |
| --- | --- |
| 包名 | `@web/basic-auth` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @web/basic-auth
```

```cp
import {
    BasicAuthConfig,
    baConstTimeEql,
    baDecodeVal,
    basicAuthDecodeBase64,
    baStartsWithIgnoreCase,
    basicAuthHook,
    createBasicAuthPlugin,
    registerBasicAuth,
} from "@web/basic-auth";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `BasicAuthConfig` | `class` | 字段: `username: string, password: string, api_key: string, api_key_header: string, realm: string, optional_auth: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `baConstTimeEql` | `(a: string, b: string): bool` | 调用 `baConstTimeEql` 并返回 `bool`。 |
| `baDecodeVal` | `(c: u8): u8?` | 调用 `baDecodeVal` 并返回 `u8?`。 |
| `basicAuthDecodeBase64` | `(input: string, out_buf: []u8): usize?` | 调用 `basicAuthDecodeBase64` 并返回 `usize?`。 |
| `baStartsWithIgnoreCase` | `(s: string, prefix: string): bool` | 调用 `baStartsWithIgnoreCase` 并返回 `bool`。 |
| `basicAuthHook` | `(ctx: &mut Context): bool` | 调用 `basicAuthHook` 并返回 `bool`。 |
| `createBasicAuthPlugin` | `(cfg: BasicAuthConfig): Plugin` | 创建或初始化实例，返回 `Plugin`。 |
| `registerBasicAuth` | `(app: &mut App, cfg: BasicAuthConfig): &mut App` | 将插件或中间件注册到目标应用实例。 |

## 最小调用示例

```cp
import { baConstTimeEql } from "@web/basic-auth";

pub fn main() {
    let result = baConstTimeEql("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
