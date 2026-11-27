[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @web/cors

Official CORS middleware plugin for cap:web (preflight OPTIONS, origin allowlist, credentials, exposed headers)

| 元信息 | 值 |
| --- | --- |
| 包名 | `@web/cors` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @web/cors
```

```cp
import {
    CorsConfig,
    corsHasCtlOrCrlf,
    corsBytesEql,
    isOriginAllowed,
    applyCorsHeaders,
    corsOnRequest,
    createCorsPlugin,
    registerCors,
} from "@web/cors";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `CorsConfig` | `class` | 字段: `allow_origin: string, allow_methods: string, allow_headers: string, expose_headers: string, allow_credentials: bool, max_age: string` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `corsHasCtlOrCrlf` | `(s: string): bool` | 调用 `corsHasCtlOrCrlf` 并返回 `bool`。 |
| `corsBytesEql` | `(a: string, b: string): bool` | 调用 `corsBytesEql` 并返回 `bool`。 |
| `isOriginAllowed` | `(allow_origin: string, origin: string): bool` | 执行 `isOriginAllowed` 条件判定，返回 `bool`。 |
| `applyCorsHeaders` | `(ctx: &mut Context, origin: string)` | 就地执行 `applyCorsHeaders` 操作。 |
| `corsOnRequest` | `(ctx: &mut Context): bool` | 调用 `corsOnRequest` 并返回 `bool`。 |
| `createCorsPlugin` | `(cfg: CorsConfig): Plugin` | 创建或初始化实例，返回 `Plugin`。 |
| `registerCors` | `(app: &mut App, cfg: CorsConfig): &mut App` | 将插件或中间件注册到目标应用实例。 |

## 最小调用示例

```cp
import { isOriginAllowed } from "@web/cors";

pub fn main() {
    let result = isOriginAllowed("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
