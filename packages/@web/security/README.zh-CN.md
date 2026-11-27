[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @web/security

Official Helmet security headers (HSTS/CSP/X-Frame-Options/Referrer-Policy) and Double-Submit CSRF plugin for cap:web

| 元信息 | 值 |
| --- | --- |
| 包名 | `@web/security` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @web/security
```

```cp
import {
    SecurityConfig,
    constantTimeEql,
    isSafeHttpMethod,
    securityOnRequest,
    createSecurityPlugin,
    registerSecurity,
} from "@web/security";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `SecurityConfig` | `class` | 字段: `hsts: string, csp: string, x_frame_options: string, x_content_type_options: string, referrer_policy: string, permissions_policy: string, +4 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `constantTimeEql` | `(a: string, b: string): bool` | 调用 `constantTimeEql` 并返回 `bool`。 |
| `isSafeHttpMethod` | `(m: AppMethod): bool` | 执行 `isSafeHttpMethod` 条件判定，返回 `bool`。 |
| `securityOnRequest` | `(ctx: &mut Context): bool` | 调用 `securityOnRequest` 并返回 `bool`。 |
| `createSecurityPlugin` | `(cfg: SecurityConfig): Plugin` | 创建或初始化实例，返回 `Plugin`。 |
| `registerSecurity` | `(app: &mut App, cfg: SecurityConfig): &mut App` | 将插件或中间件注册到目标应用实例。 |

## 最小调用示例

```cp
import { constantTimeEql } from "@web/security";

pub fn main() {
    let result = constantTimeEql("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
