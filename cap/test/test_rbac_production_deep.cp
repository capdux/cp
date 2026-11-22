// Source: cp/cap/test/test_rbac_production_deep.cp
// Deep Production Test Suite for cap:rbac & cap.rbac
// Covers:
//   1. PermissionMatcher exact, single-segment `*`, trailing multi-segment `**` & invalid segment rejection
//   2. Multi-layer role inheritance (superadmin -> admin -> editor -> viewer), effectiveRoles & CycleDetected
//   3. Explicit-Deny-Overrides-Allow across role-level, inheritance-chain & subject-level layers + Default-Deny
//   4. Dynamic assignRole / revokeRole mutations, ownership buffer mutation safety & AccessDecision audit fields
//   5. Dynamic lazy namespace mounting via `import cap from "cap"; cap.rbac`

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    RbacError,
    PolicyEffect,
    PermissionMatcher,
    AccessDecision,
    RbacEngine,
} from "cap:auth";
import cap from "cap";

fn strEq(a: string, b: string): bool {
    return std.mem.eql(u8, a, b);
}

fn containsStr(haystack: []const string, needle: string): bool {
    for (item of haystack) {
        if (strEq(item, needle)) return true;
    }
    return false;
}

fn testPermissionMatcher(): !void {
    // Valid patterns & concrete permissions
    expect_eq(PermissionMatcher.isValid("repo:issues:read"), true);
    expect_eq(PermissionMatcher.isValid("tool:*"), true);
    expect_eq(PermissionMatcher.isValid("*:*"), true);
    expect_eq(PermissionMatcher.isValid("repo:**"), true);
    expect_eq(PermissionMatcher.isValid("**"), true);
    expect_eq(PermissionMatcher.isValid("*"), true);

    // Invalid empty segments, whitespace, partial wildcards & non-trailing `**`
    expect_eq(PermissionMatcher.isValid(""), false);
    expect_eq(PermissionMatcher.isValid(":"), false);
    expect_eq(PermissionMatcher.isValid(":read"), false);
    expect_eq(PermissionMatcher.isValid("repo:"), false);
    expect_eq(PermissionMatcher.isValid("repo::read"), false);
    expect_eq(PermissionMatcher.isValid("repo:read*"), false);
    expect_eq(PermissionMatcher.isValid("repo:*read"), false);
    expect_eq(PermissionMatcher.isValid("repo:***"), false);
    expect_eq(PermissionMatcher.isValid("repo:**:read"), false);
    expect_eq(PermissionMatcher.isValid("repo:issues read"), false);

    // Concrete permission validation
    expect_eq(PermissionMatcher.isValidConcrete("repo:issues:read"), true);
    expect_eq(PermissionMatcher.isValidConcrete("tool:*"), false);
    expect_eq(PermissionMatcher.isValidConcrete("repo:**"), false);

    // Exact matches
    expect_eq(PermissionMatcher.matches("repo:issues:read", "repo:issues:read"), true);
    expect_eq(PermissionMatcher.matches("repo:issues:read", "repo:issues:write"), false);
    expect_eq(PermissionMatcher.matches("repo:issues:read", "repo:issues"), false);

    // Single-segment `*`
    expect_eq(PermissionMatcher.matches("tool:*", "tool:search"), true);
    expect_eq(PermissionMatcher.matches("tool:*", "tool:fs:read"), false);
    expect_eq(PermissionMatcher.matches("tool:*", "tool"), false);
    expect_eq(PermissionMatcher.matches("repo:*:read", "repo:issues:read"), true);
    expect_eq(PermissionMatcher.matches("repo:*:read", "repo:pulls:read"), true);
    expect_eq(PermissionMatcher.matches("repo:*:read", "repo:issues:sub:read"), false);
    expect_eq(PermissionMatcher.matches("*:*", "tool:exec"), true);
    expect_eq(PermissionMatcher.matches("*:*", "repo:issues:read"), false);

    // Trailing multi-segment `**`
    expect_eq(PermissionMatcher.matches("repo:**", "repo:issues"), true);
    expect_eq(PermissionMatcher.matches("repo:**", "repo:issues:read"), true);
    expect_eq(PermissionMatcher.matches("repo:**", "repo:issues:comments:edit"), true);
    expect_eq(PermissionMatcher.matches("repo:**", "repo"), false);
    expect_eq(PermissionMatcher.matches("repo:**", "other:issues:read"), false);
    expect_eq(PermissionMatcher.matches("**", "a"), true);
    expect_eq(PermissionMatcher.matches("**", "a:b:c"), true);

    // Asymmetric wildcard safety (requested cannot escalate using `*`)
    expect_eq(PermissionMatcher.matches("repo:read", "repo:*"), false);
    expect_eq(PermissionMatcher.matches("repo:**", "repo:*"), false);
    expect_eq(PermissionMatcher.matches("repo:**:read", "repo:issues:read"), false);
    expect_eq(PermissionMatcher.matches("repo:issues:read", "repo::read"), false);
}

fn testHierarchyAndCycleDetection(): !void {
    const alloc = std.heap.page_allocator;
    let engine = RbacEngine.init(alloc);
    defer engine.deinit();

    engine.defineRole("viewer")?;
    engine.defineRole("editor")?;
    engine.defineRole("admin")?;
    engine.defineRole("superadmin")?;
    // Idempotent defineRole
    engine.defineRole("viewer")?;

    // Hierarchy: superadmin -> admin -> editor -> viewer
    engine.addRoleInheritance("editor", "viewer")?;
    engine.addRoleInheritance("admin", "editor")?;
    engine.addRoleInheritance("superadmin", "admin")?;
    // Idempotent inheritance edge
    engine.addRoleInheritance("admin", "editor")?;

    engine.grantPermission("viewer", "repo:issues:read")?;
    engine.grantPermission("editor", "repo:issues:write")?;
    engine.grantPermission("admin", "repo:settings:*")?;
    engine.grantPermission("superadmin", "cluster:**")?;

    engine.assignRole("root-agent", "superadmin")?;
    engine.assignRole("ci-bot", "editor")?;

    // Transitive permission inheritance check
    const d_super_read = engine.check("root-agent", "repo:issues:read");
    expect_eq(d_super_read.allowed, true);
    expect_eq(d_super_read.effect, PolicyEffect.Allow);
    expect_eq(strEq(d_super_read.matched_role, "viewer"), true);
    expect_eq(strEq(d_super_read.matched_rule, "repo:issues:read"), true);

    const d_super_write = engine.check("root-agent", "repo:issues:write");
    expect_eq(d_super_write.allowed, true);
    expect_eq(strEq(d_super_write.matched_role, "editor"), true);

    const d_super_cluster = engine.check("root-agent", "cluster:nodes:drain:force");
    expect_eq(d_super_cluster.allowed, true);
    expect_eq(strEq(d_super_cluster.matched_role, "superadmin"), true);

    // Editor inherits viewer, but NOT admin/superadmin
    expect_eq(engine.check("ci-bot", "repo:issues:read").allowed, true);
    expect_eq(engine.check("ci-bot", "repo:issues:write").allowed, true);
    expect_eq(engine.check("ci-bot", "repo:settings:update").allowed, false);
    expect_eq(engine.check("ci-bot", "cluster:nodes:list").allowed, false);

    // effectiveRoles deduplicated BFS expansion
    const root_roles = engine.effectiveRoles(alloc, "root-agent")?;
    defer alloc.free(root_roles);
    expect_eq(root_roles.len, 4);
    expect_eq(strEq(root_roles[0], "superadmin"), true);
    expect_eq(strEq(root_roles[1], "admin"), true);
    expect_eq(strEq(root_roles[2], "editor"), true);
    expect_eq(strEq(root_roles[3], "viewer"), true);

    const unknown_roles = engine.effectiveRoles(alloc, "nobody")?;
    defer alloc.free(unknown_roles);
    expect_eq(unknown_roles.len, 0);

    // Cycle detection: self-loop & ancestor-to-descendant cycle
    let self_cycle = false;
    engine.addRoleInheritance("viewer", "viewer") catch e => {
        if (e == RbacError.CycleDetected) self_cycle = true;
    };
    expect_eq(self_cycle, true);

    let indirect_cycle = false;
    engine.addRoleInheritance("viewer", "superadmin") catch e => {
        if (e == RbacError.CycleDetected) indirect_cycle = true;
    };
    expect_eq(indirect_cycle, true);

    // Missing role guard
    let missing_role = false;
    engine.addRoleInheritance("ghost", "viewer") catch e => {
        if (e == RbacError.RoleNotFound) missing_role = true;
    };
    expect_eq(missing_role, true);

    // Invalid permission guard
    let invalid_perm = false;
    engine.grantPermission("viewer", "repo::read") catch e => {
        if (e == RbacError.InvalidPermission) invalid_perm = true;
    };
    expect_eq(invalid_perm, true);
}

fn testExplicitDenyOverridesThreeLayers(): !void {
    const alloc = std.heap.page_allocator;
    let engine = RbacEngine.init(alloc);
    defer engine.deinit();

    engine.defineRole("base_user")?;
    engine.defineRole("tool_operator")?;
    engine.defineRole("auditor_guard")?;
    engine.addRoleInheritance("tool_operator", "base_user")?;

    // Allow broad tool access on child role, but deny shell exec on inherited parent role
    engine.grantPermission("tool_operator", "tool:**")?;
    engine.denyPermission("base_user", "tool:shell:exec")?;

    // Layer 1 (Role-level explicit deny on same role): allow repo:* + deny repo:secrets:*
    engine.grantPermission("tool_operator", "repo:*:read")?;
    engine.denyPermission("tool_operator", "repo:secrets:read")?;

    // Multi-role conflicting policies: auditor_guard denies tool:fs:delete even if tool_operator allows tool:**
    engine.denyPermission("auditor_guard", "tool:fs:delete")?;

    engine.assignRole("agent-alpha", "tool_operator")?;
    engine.assignRole("agent-alpha", "auditor_guard")?;

    // Layer 3 (Subject-level explicit deny): overrides all role allows
    engine.denySubjectPermission("agent-alpha", "tool:http:post")?;

    // Allowed action
    const d_ok = engine.check("agent-alpha", "tool:fs:read");
    expect_eq(d_ok.allowed, true);
    expect_eq(d_ok.effect, PolicyEffect.Allow);
    expect_eq(strEq(d_ok.matched_rule, "tool:**"), true);
    expect_eq(strEq(d_ok.matched_role, "tool_operator"), true);

    // 1. Same-role explicit deny overrides same-role wildcard allow
    const d_role_deny = engine.check("agent-alpha", "repo:secrets:read");
    expect_eq(d_role_deny.allowed, false);
    expect_eq(d_role_deny.effect, PolicyEffect.Deny);
    expect_eq(strEq(d_role_deny.matched_rule, "repo:secrets:read"), true);
    expect_eq(strEq(d_role_deny.matched_role, "tool_operator"), true);

    // 2. Inherited parent explicit deny overrides child wildcard allow
    const d_inherited_deny = engine.check("agent-alpha", "tool:shell:exec");
    expect_eq(d_inherited_deny.allowed, false);
    expect_eq(d_inherited_deny.effect, PolicyEffect.Deny);
    expect_eq(strEq(d_inherited_deny.matched_rule, "tool:shell:exec"), true);
    expect_eq(strEq(d_inherited_deny.matched_role, "base_user"), true);

    // 3. Sibling assigned role explicit deny overrides another assigned role allow
    const d_sibling_deny = engine.check("agent-alpha", "tool:fs:delete");
    expect_eq(d_sibling_deny.allowed, false);
    expect_eq(d_sibling_deny.effect, PolicyEffect.Deny);
    expect_eq(strEq(d_sibling_deny.matched_rule, "tool:fs:delete"), true);
    expect_eq(strEq(d_sibling_deny.matched_role, "auditor_guard"), true);

    // 4. Subject-level explicit deny overrides all role allows
    const d_subj_deny = engine.check("agent-alpha", "tool:http:post");
    expect_eq(d_subj_deny.allowed, false);
    expect_eq(d_subj_deny.effect, PolicyEffect.Deny);
    expect_eq(strEq(d_subj_deny.matched_rule, "tool:http:post"), true);
    expect_eq(strEq(d_subj_deny.matched_role, ""), true);

    // 5. Default-Deny when no rule matches or unknown subject
    const d_default = engine.check("agent-alpha", "billing:invoice:pay");
    expect_eq(d_default.allowed, false);
    expect_eq(d_default.effect, PolicyEffect.Deny);
    expect_eq(strEq(d_default.matched_rule, ""), true);
    expect_eq(strEq(d_default.matched_role, ""), true);

    const d_unknown = engine.check("stranger", "tool:fs:read");
    expect_eq(d_unknown.allowed, false);
    expect_eq(d_unknown.effect, PolicyEffect.Deny);
    expect_eq(strEq(d_unknown.matched_rule, ""), true);
}

fn testDynamicRoleMutationAndAudit(): !void {
    const alloc = std.heap.page_allocator;
    let engine = RbacEngine.init(alloc);
    defer engine.deinit();

    // Verify ownership safety when caller mutates input slices after API calls
    let role_buf: [16]u8 = undefined;
    @memcpy(role_buf[0..9], "deploy_op");
    engine.defineRole(role_buf[0..9])?;
    @memcpy(role_buf[0..9], "XXXXXXXXX");

    let perm_buf: [16]u8 = undefined;
    @memcpy(perm_buf[0..11], "deploy:prod");
    engine.grantPermission("deploy_op", perm_buf[0..11])?;
    @memcpy(perm_buf[0..11], "YYYYYYYYYYY");

    let subj_buf: [16]u8 = undefined;
    @memcpy(subj_buf[0..8], "alice-01");
    engine.assignRole(subj_buf[0..8], "deploy_op")?;
    @memcpy(subj_buf[0..8], "ZZZZZZZZ");

    engine.defineRole("read_only")?;
    engine.grantPermission("read_only", "deploy:status")?;
    engine.assignRole("alice-01", "read_only")?;

    // Before revoke: both deploy:prod and deploy:status allowed
    const before_prod = engine.check("alice-01", "deploy:prod");
    expect_eq(before_prod.allowed, true);
    expect_eq(strEq(before_prod.matched_role, "deploy_op"), true);
    expect_eq(strEq(before_prod.matched_rule, "deploy:prod"), true);
    expect_eq(engine.check("alice-01", "deploy:status").allowed, true);

    // Revoke deploy_op dynamically
    engine.revokeRole("alice-01", "deploy_op")?;

    // After revoke: deploy:prod is denied (default-deny), deploy:status still allowed
    const after_prod = engine.check("alice-01", "deploy:prod");
    expect_eq(after_prod.allowed, false);
    expect_eq(after_prod.effect, PolicyEffect.Deny);
    expect_eq(strEq(after_prod.matched_rule, ""), true);
    expect_eq(strEq(after_prod.matched_role, ""), true);

    const after_status = engine.check("alice-01", "deploy:status");
    expect_eq(after_status.allowed, true);
    expect_eq(strEq(after_status.matched_role, "read_only"), true);

    // Revoking again or revoking from unknown subject is a safe no-op
    engine.revokeRole("alice-01", "deploy_op")?;
    engine.revokeRole("unknown-user", "read_only")?;

    // Effective roles reflects revocation accurately
    const eff = engine.effectiveRoles(alloc, "alice-01")?;
    defer alloc.free(eff);
    expect_eq(eff.len, 1);
    expect_eq(containsStr(eff, "read_only"), true);
    expect_eq(containsStr(eff, "deploy_op"), false);
}

fn testCapRbacDynamicMount(): !void {
    const alloc = std.heap.page_allocator;

    // Verify PermissionMatcher and RbacEngine via `cap.rbac` lazy namespace mounting
    expect_eq(cap.auth.PermissionMatcher.isValid("agent:tool:read"), true);
    expect_eq(cap.auth.PermissionMatcher.matches("agent:**", "agent:tool:search:web"), true);

    let engine = cap.auth.RbacEngine.init(alloc);
    defer engine.deinit();

    engine.defineRole("tenant_admin")?;
    engine.defineRole("tenant_member")?;
    engine.addRoleInheritance("tenant_admin", "tenant_member")?;
    engine.grantPermission("tenant_member", "workspace:docs:read")?;
    engine.grantPermission("tenant_admin", "workspace:*:write")?;
    engine.denyPermission("tenant_admin", "workspace:audit:write")?;

    engine.assignRole("bot-77", "tenant_admin")?;

    const d1 = engine.check("bot-77", "workspace:docs:read");
    expect_eq(d1.allowed, true);
    expect_eq(d1.effect, cap.auth.PolicyEffect.Allow);
    expect_eq(strEq(d1.matched_role, "tenant_member"), true);

    const d2 = engine.check("bot-77", "workspace:docs:write");
    expect_eq(d2.allowed, true);
    expect_eq(strEq(d2.matched_role, "tenant_admin"), true);

    const d3 = engine.check("bot-77", "workspace:audit:write");
    expect_eq(d3.allowed, false);
    expect_eq(d3.effect, cap.auth.PolicyEffect.Deny);
    expect_eq(strEq(d3.matched_rule, "workspace:audit:write"), true);
}

fn main() {
    test("1. PermissionMatcher Exact, Single `*`, Multi-Level `**` & Invalid Segment Rejection", () => {
        testPermissionMatcher()?;
    });

    test("2. Multi-Layer Role Inheritance, effectiveRoles & CycleDetected Guard", () => {
        testHierarchyAndCycleDetection()?;
    });

    test("3. Explicit-Deny-Overrides-Allow (Role, Inherited & Subject) & Default-Deny", () => {
        testExplicitDenyOverridesThreeLayers()?;
    });

    test("4. Dynamic assignRole/revokeRole, Ownership Safety & AccessDecision Audit Trail", () => {
        testDynamicRoleMutationAndAudit()?;
    });

    test("5. Dynamic Lazy Namespace Mounting via `import cap from \"cap\"; cap.rbac`", () => {
        testCapRbacDynamicMount()?;
    });
}
