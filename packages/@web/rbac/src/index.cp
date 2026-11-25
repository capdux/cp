// Official @web/rbac Plugin for `cap:web`
// Zero-heap Role-Based Access Control (RBAC) with wildcard permissions (`*`, `orders:*`)
// and route-level policy guards (inspired by Casbin / axum-rbac).

import {
    App,
    AppMethod,
    Context,
    Response,
    Plugin,
    PluginScope,
} from "../../../../lib/web/app.cp";

pub const RBAC_MAX_GRANTS: usize = 64;
pub const RBAC_MAX_RULES: usize = 32;

pub struct RbacConfig {
    role_header: string = "X-Role";
    default_role: string = "guest";
}

struct RbacGrant {
    role: string = "";
    perm: string = "";
}

struct RbacPathRule {
    method: AppMethod = AppMethod.GET;
    path_prefix: string = "";
    required_perm: string = "";
}

let g_rbac_cfg: RbacConfig = RbacConfig{};
let g_rbac_grants: [RBAC_MAX_GRANTS]RbacGrant = [RbacGrant{}; RBAC_MAX_GRANTS];
let g_rbac_grant_len: usize = 0;
let g_rbac_rules: [RBAC_MAX_RULES]RbacPathRule = [RbacPathRule{}; RBAC_MAX_RULES];
let g_rbac_rule_len: usize = 0;

fn rbacStrEql(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn rbacStartsWith(s: string, prefix: string): bool {
    if (s.len < prefix.len) return false;
    let i: usize = 0;
    while (i < prefix.len) : (i += 1) {
        if (s[i] != prefix[i]) return false;
    }
    return true;
}

fn rbacPermMatches(granted: string, needed: string): bool {
    if (rbacStrEql(granted, "*") || rbacStrEql(granted, needed)) return true;
    // Wildcard suffix like `orders:*` matches `orders:read` or `orders:write`
    if (granted.len >= 2 && granted[granted.len - 1] == '*') {
        return rbacStartsWith(needed, granted[0 .. granted.len - 1]);
    }
    return false;
}

pub fn rbacReset() {
    g_rbac_grant_len = 0;
    g_rbac_rule_len = 0;
}

pub fn rbacGrant(role: string, perm: string): bool {
    if (g_rbac_grant_len >= RBAC_MAX_GRANTS) return false;
    g_rbac_grants[g_rbac_grant_len] = RbacGrant{ role: role, perm: perm };
    g_rbac_grant_len += 1;
    return true;
}

pub fn rbacProtect(method: AppMethod, path_prefix: string, required_perm: string): bool {
    if (g_rbac_rule_len >= RBAC_MAX_RULES) return false;
    g_rbac_rules[g_rbac_rule_len] = RbacPathRule{
        method: method,
        path_prefix: path_prefix,
        required_perm: required_perm,
    };
    g_rbac_rule_len += 1;
    return true;
}

/// Check whether `role` has `needed_perm` (supports `*` and `resource:*` wildcards).
pub fn rbacCan(role: string, needed_perm: string): bool {
    let i: usize = 0;
    while (i < g_rbac_grant_len) : (i += 1) {
        if (rbacStrEql(g_rbac_grants[i].role, role) && rbacPermMatches(g_rbac_grants[i].perm, needed_perm)) {
            return true;
        }
    }
    return false;
}

pub fn rbacGuardHook(ctx: *Context): bool {
    const role = ctx.derived("jwt_role") ?? (ctx.header(g_rbac_cfg.role_header) ?? g_rbac_cfg.default_role);
    * = ctx.setDerived("rbac_role", role);

    let i: usize = 0;
    while (i < g_rbac_rule_len) : (i += 1) {
        const rule = g_rbac_rules[i];
        if (rule.method == ctx.method && rbacStartsWith(ctx.path, rule.path_prefix)) {
            if (!rbacCan(role, rule.required_perm)) {
                ctx.abort(Response.json(403, "{\"error\":\"forbidden_insufficient_permission\"}"));
                return false;
            }
        }
    }
    return true;
}

pub fn createRbacPlugin(cfg: RbacConfig): Plugin {
    g_rbac_cfg = cfg;
    let p = Plugin.init("@web/rbac");
    * = p.withScope(PluginScope.Global).onBeforeHandle(rbacGuardHook);
    return p;
}

pub fn registerRbac(app: *App, cfg: RbacConfig): *App {
    const p = createRbacPlugin(cfg);
    return app.usePlugin(&p);
}
