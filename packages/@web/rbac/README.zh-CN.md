[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @web/rbac

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Official declarative Role-Based Access Control (RBAC) with wildcard permissions & route guards for cap:web

## 安装与导入

```bash
cap add @web/rbac
```

```cp
import { rbacStrEql } from "@web/rbac";
```

## 核心导出 API

- `export fn rbacStrEql(a: string, b: string): bool;`
- `export fn rbacStartsWith(s: string, prefix: string): bool;`
- `export fn rbacPermMatches(granted: string, needed: string): bool;`
- `export fn rbacReset();`
- `export fn rbacGrant(role: string, perm: string): bool;`
- `export fn rbacProtect(method: AppMethod, path_prefix: string, required_perm: string): bool;`
- `export fn rbacCan(role: string, needed_perm: string): bool;`
- `export fn rbacGuardHook(ctx: &mut Context): bool;`
- `export fn createRbacPlugin(cfg: RbacConfig): Plugin;`
- `export fn registerRbac(app: &mut App, cfg: RbacConfig): &mut App;`
