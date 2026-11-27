# @web/rbac — AI Agent Quick Ref

Official declarative Role-Based Access Control (RBAC) with wildcard permissions & route guards for cap:web (`v1.0.0`, `MIT`)

## Import

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

## Core Types & Constants

```cp
class RbacConfig { role_header: string, default_role: string }
class RbacGrant { role: string, perm: string }
class RbacPathRule { method: AppMethod, path_prefix: string, required_perm: string }
```

## API Signatures

```cp
fn rbacStrEql(a: string, b: string): bool;
fn rbacStartsWith(s: string, prefix: string): bool;
fn rbacPermMatches(granted: string, needed: string): bool;
fn rbacReset();
fn rbacGrant(role: string, perm: string): bool;
fn rbacProtect(method: AppMethod, path_prefix: string, required_perm: string): bool;
fn rbacCan(role: string, needed_perm: string): bool;
fn rbacGuardHook(ctx: &mut Context): bool;
fn createRbacPlugin(cfg: RbacConfig): Plugin;
fn registerRbac(app: &mut App, cfg: RbacConfig): &mut App;
```

## Minimal Usage

```cp
import { rbacStrEql } from "@web/rbac";

pub fn main() {
    let result = rbacStrEql("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@web/rbac"`; install via `cap add @web/rbac`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
