[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @web/rbac

Official declarative Role-Based Access Control (RBAC) with wildcard permissions & route guards for cap:web

| 元信息 | 值 |
| --- | --- |
| 包名 | `@web/rbac` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @web/rbac
```

```cp
import {
    RbacConfig,
    RbacGrant,
    RbacPathRule,
    rbacStrEql,
    rbacStartsWith,
    rbacPermMatches,
    rbacReset,
    rbacGrant,
} from "@web/rbac";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `RbacConfig` | `class` | 字段: `role_header: string, default_role: string` |
| `RbacGrant` | `class` | 字段: `role: string, perm: string` |
| `RbacPathRule` | `class` | 字段: `method: AppMethod, path_prefix: string, required_perm: string` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `rbacStrEql` | `(a: string, b: string): bool` | 调用 `rbacStrEql` 并返回 `bool`。 |
| `rbacStartsWith` | `(s: string, prefix: string): bool` | 调用 `rbacStartsWith` 并返回 `bool`。 |
| `rbacPermMatches` | `(granted: string, needed: string): bool` | 调用 `rbacPermMatches` 并返回 `bool`。 |
| `rbacReset` | `()` | 就地执行 `rbacReset` 操作。 |
| `rbacGrant` | `(role: string, perm: string): bool` | 调用 `rbacGrant` 并返回 `bool`。 |
| `rbacProtect` | `(method: AppMethod, path_prefix: string, required_perm: string): bool` | 调用 `rbacProtect` 并返回 `bool`。 |
| `rbacCan` | `(role: string, needed_perm: string): bool` | 调用 `rbacCan` 并返回 `bool`。 |
| `rbacGuardHook` | `(ctx: &mut Context): bool` | 调用 `rbacGuardHook` 并返回 `bool`。 |
| `createRbacPlugin` | `(cfg: RbacConfig): Plugin` | 创建或初始化实例，返回 `Plugin`。 |
| `registerRbac` | `(app: &mut App, cfg: RbacConfig): &mut App` | 将插件或中间件注册到目标应用实例。 |

## 最小调用示例

```cp
import { rbacStrEql } from "@web/rbac";

pub fn main() {
    let result = rbacStrEql("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
