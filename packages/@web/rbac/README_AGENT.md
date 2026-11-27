# @web/rbac@1.0.0 (Agent Quickref)

> Official declarative Role-Based Access Control (RBAC) with wildcard permissions & route guards for cap:web | License: MIT
> Import: `import { ... } from "@web/rbac";` | Full types: `src/index.d.cp`

## Functions
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
