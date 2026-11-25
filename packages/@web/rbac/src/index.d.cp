// Type definitions for CP

import { App, AppMethod, Context, Response, Plugin, PluginScope } from "../../../../lib/web/app.cp";
export class RbacConfig {
    role_header: string,
    default_role: string,
}

export class RbacGrant {
    role: string,
    perm: string,
}

export class RbacPathRule {
    method: AppMethod,
    path_prefix: string,
    required_perm: string,
}

export fn rbacStrEql(a: string, b: string): bool;
export fn rbacStartsWith(s: string, prefix: string): bool;
export fn rbacPermMatches(granted: string, needed: string): bool;
export fn rbacReset();
export fn rbacGrant(role: string, perm: string): bool;
export fn rbacProtect(method: AppMethod, path_prefix: string, required_perm: string): bool;
export fn rbacCan(role: string, needed_perm: string): bool;
export fn rbacGuardHook(ctx: &mut Context): bool;
export fn createRbacPlugin(cfg: RbacConfig): Plugin;
export fn registerRbac(app: &mut App, cfg: RbacConfig): &mut App;
