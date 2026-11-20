// Source: cp/lib/rbac/engine.cp (Hierarchical RBAC Engine with Explicit-Deny-Overrides & Audit Trail)
// Pure CP enterprise RBAC / IAM policy evaluation engine supporting multi-layer role
// inheritance with cycle detection, wildcard matching, explicit-deny-overrides-allow
// semantics across subject/role/inherited layers, and structured audit decisions.

const std = @import("std");
import { RbacError, PolicyEffect, PermissionMatcher } from "./permission.cp";

/// Structured access decision returned by `RbacEngine.check` for auditability.
/// `matched_rule` and `matched_role` borrow immutable strings owned by `RbacEngine`
/// and remain valid until `RbacEngine.deinit()`.
pub struct AccessDecision {
    allowed: bool = false,
    effect: PolicyEffect = .Deny,
    matched_rule: string = "",
    matched_role: string = "",
}

struct RoleEntry {
    name: []u8,
    parents: std.ArrayList(usize) = .empty,
    allow_rules: std.ArrayList([]u8) = .empty,
    deny_rules: std.ArrayList([]u8) = .empty,

    fn deinit(*_, alloc: std.mem.Allocator) {
        for (pat of _.allow_rules.items) {
            alloc.free(pat);
        }
        _.allow_rules.deinit(alloc);

        for (pat of _.deny_rules.items) {
            alloc.free(pat);
        }
        _.deny_rules.deinit(alloc);

        _.parents.deinit(alloc);
        alloc.free(_.name);
    }
}

struct SubjectEntry {
    id: []u8,
    roles: std.ArrayList(usize) = .empty,
    deny_rules: std.ArrayList([]u8) = .empty,

    fn deinit(*_, alloc: std.mem.Allocator) {
        for (pat of _.deny_rules.items) {
            alloc.free(pat);
        }
        _.deny_rules.deinit(alloc);

        _.roles.deinit(alloc);
        alloc.free(_.id);
    }
}

fn containsSlice(items: []const []u8, target: string): bool {
    for (item of items) {
        if (std.mem.eql(u8, item, target)) {
            return true;
        }
    }
    return false;
}

/// Hierarchical RBAC & IAM policy engine with explicit-deny-overrides semantics.
pub struct RbacEngine {
    alloc: std.mem.Allocator = undefined,
    roles: std.ArrayList(RoleEntry) = .empty,
    role_index: std.StringHashMapUnmanaged(usize) = .empty,
    subjects: std.ArrayList(SubjectEntry) = .empty,
    subject_index: std.StringHashMapUnmanaged(usize) = .empty,

    /// Initializes an empty `RbacEngine` backed by `alloc`.
    pub static fn init(alloc: std.mem.Allocator): RbacEngine {
        return RbacEngine{
            alloc: alloc,
            roles: .empty,
            role_index: .empty,
            subjects: .empty,
            subject_index: .empty,
        };
    }

    /// Releases all owned role definitions, policy rules, subject bindings, and lookup tables.
    pub fn deinit(*_) {
        for (*r of _.roles.items) {
            r.deinit(_.alloc);
        }
        _.roles.deinit(_.alloc);
        _.role_index.deinit(_.alloc);

        for (*s of _.subjects.items) {
            s.deinit(_.alloc);
        }
        _.subjects.deinit(_.alloc);
        _.subject_index.deinit(_.alloc);
    }

    /// Defines a role identified by `role_name`.
    /// Idempotent: if `role_name` already exists, succeeds without duplicating it.
    pub fn defineRole(*_, role_name: string): void ! RbacError {
        if (_.role_index.get(role_name) == null) {
            const owned_name = _.alloc.dupe(u8, role_name) catch return RbacError.OutOfMemory;
            errdefer _.alloc.free(owned_name);

            const idx = _.roles.items.len;
            const entry = RoleEntry{
                name: owned_name,
                parents: .empty,
                allow_rules: .empty,
                deny_rules: .empty,
            };
            _.roles.append(_.alloc, entry) catch return RbacError.OutOfMemory;
            errdefer * = _.roles.pop();

            _.role_index.put(_.alloc, owned_name, idx) catch return RbacError.OutOfMemory;
        }
    }

    /// Grants `perm_pattern` (Allow) to `role_name`.
    pub fn grantPermission(*_, role_name: string, perm_pattern: string): void ! RbacError {
        if (!PermissionMatcher.isValid(perm_pattern)) {
            return RbacError.InvalidPermission;
        }
        const opt_idx = _.role_index.get(role_name);
        if (opt_idx == null) {
            return RbacError.RoleNotFound;
        }
        const idx = opt_idx.?;

        if (!containsSlice(_.roles.items[idx].allow_rules.items, perm_pattern)) {
            const owned_pat = _.alloc.dupe(u8, perm_pattern) catch return RbacError.OutOfMemory;
            errdefer _.alloc.free(owned_pat);

            _.roles.items[idx].allow_rules.append(_.alloc, owned_pat) catch return RbacError.OutOfMemory;
        }
    }

    /// Attaches an explicit `Deny` rule for `perm_pattern` on `role_name`.
    pub fn denyPermission(*_, role_name: string, perm_pattern: string): void ! RbacError {
        if (!PermissionMatcher.isValid(perm_pattern)) {
            return RbacError.InvalidPermission;
        }
        const opt_idx = _.role_index.get(role_name);
        if (opt_idx == null) {
            return RbacError.RoleNotFound;
        }
        const idx = opt_idx.?;

        if (!containsSlice(_.roles.items[idx].deny_rules.items, perm_pattern)) {
            const owned_pat = _.alloc.dupe(u8, perm_pattern) catch return RbacError.OutOfMemory;
            errdefer _.alloc.free(owned_pat);

            _.roles.items[idx].deny_rules.append(_.alloc, owned_pat) catch return RbacError.OutOfMemory;
        }
    }

    /// Declares that `child_role` inherits all permissions and denies from `parent_role`.
    /// Returns `RbacError.CycleDetected` if adding this edge would introduce a cycle.
    pub fn addRoleInheritance(*_, child_role: string, parent_role: string): void ! RbacError {
        const opt_child = _.role_index.get(child_role);
        if (opt_child == null) {
            return RbacError.RoleNotFound;
        }
        const child_idx = opt_child.?;

        const opt_parent = _.role_index.get(parent_role);
        if (opt_parent == null) {
            return RbacError.RoleNotFound;
        }
        const parent_idx = opt_parent.?;

        if (child_idx == parent_idx) {
            return RbacError.CycleDetected;
        }

        let already_linked = false;
        for (existing of _.roles.items[child_idx].parents.items) {
            if (existing == parent_idx) {
                already_linked = true;
                break;
            }
        }

        if (!already_linked) {
            // Adding edge child -> parent creates a cycle iff child is already reachable from parent.
            const n = _.roles.items.len;
            let visited = _.alloc.alloc(bool, n) catch return RbacError.OutOfMemory;
            defer _.alloc.free(visited);
            let i: usize = 0;
            while (i < n) : (i += 1) {
                visited[i] = false;
            }

            let queue: std.ArrayList(usize) = .empty;
            defer queue.deinit(_.alloc);

            visited[parent_idx] = true;
            queue.append(_.alloc, parent_idx) catch return RbacError.OutOfMemory;

            let head: usize = 0;
            while (head < queue.items.len) {
                const curr = queue.items[head];
                head += 1;
                if (curr == child_idx) {
                    return RbacError.CycleDetected;
                }
                for (anc of _.roles.items[curr].parents.items) {
                    if (!visited[anc]) {
                        visited[anc] = true;
                        queue.append(_.alloc, anc) catch return RbacError.OutOfMemory;
                    }
                }
            }

            _.roles.items[child_idx].parents.append(_.alloc, parent_idx) catch return RbacError.OutOfMemory;
        }
    }

    fn ensureSubject(*_, subject_id: string): usize ! RbacError {
        if (existing := _.subject_index.get(subject_id)) {
            return existing;
        }

        const owned_id = _.alloc.dupe(u8, subject_id) catch return RbacError.OutOfMemory;
        errdefer _.alloc.free(owned_id);

        const idx = _.subjects.items.len;
        const entry = SubjectEntry{
            id: owned_id,
            roles: .empty,
            deny_rules: .empty,
        };
        _.subjects.append(_.alloc, entry) catch return RbacError.OutOfMemory;
        errdefer * = _.subjects.pop();

        _.subject_index.put(_.alloc, owned_id, idx) catch return RbacError.OutOfMemory;
        return idx;
    }

    /// Assigns `role_name` to `subject_id`. Idempotent if already assigned.
    pub fn assignRole(*_, subject_id: string, role_name: string): void ! RbacError {
        const opt_role = _.role_index.get(role_name);
        if (opt_role == null) {
            return RbacError.RoleNotFound;
        }
        const role_idx = opt_role.?;
        const subj_idx = _.ensureSubject(subject_id)?;

        let already_assigned = false;
        for (existing of _.subjects.items[subj_idx].roles.items) {
            if (existing == role_idx) {
                already_assigned = true;
                break;
            }
        }

        if (!already_assigned) {
            _.subjects.items[subj_idx].roles.append(_.alloc, role_idx) catch return RbacError.OutOfMemory;
        }
    }

    /// Revokes `role_name` from `subject_id`.
    /// Returns `RbacError.RoleNotFound` if `role_name` is undefined.
    /// No-op if `subject_id` does not currently hold `role_name`.
    pub fn revokeRole(*_, subject_id: string, role_name: string): void ! RbacError {
        const opt_role = _.role_index.get(role_name);
        if (opt_role == null) {
            return RbacError.RoleNotFound;
        }
        const role_idx = opt_role.?;

        if (subj_idx := _.subject_index.get(subject_id)) {
            let pos: usize = 0;
            while (pos < _.subjects.items[subj_idx].roles.items.len) : (pos += 1) {
                if (_.subjects.items[subj_idx].roles.items[pos] == role_idx) {
                    * = _.subjects.items[subj_idx].roles.orderedRemove(pos);
                    break;
                }
            }
        }
    }

    /// Attaches a subject-level explicit `Deny` rule for `perm_pattern` directly on `subject_id`.
    pub fn denySubjectPermission(*_, subject_id: string, perm_pattern: string): void ! RbacError {
        if (!PermissionMatcher.isValid(perm_pattern)) {
            return RbacError.InvalidPermission;
        }
        const subj_idx = _.ensureSubject(subject_id)?;

        if (!containsSlice(_.subjects.items[subj_idx].deny_rules.items, perm_pattern)) {
            const owned_pat = _.alloc.dupe(u8, perm_pattern) catch return RbacError.OutOfMemory;
            errdefer _.alloc.free(owned_pat);

            _.subjects.items[subj_idx].deny_rules.append(_.alloc, owned_pat) catch return RbacError.OutOfMemory;
        }
    }

    fn collectEffectiveRoleIndices(alloc: std.mem.Allocator, subj_idx: usize): ([]usize) ! RbacError {
        const n = _.roles.items.len;
        if (n == 0 || _.subjects.items[subj_idx].roles.items.len == 0) {
            return alloc.alloc(usize, 0) catch return RbacError.OutOfMemory;
        }

        let visited = alloc.alloc(bool, n) catch return RbacError.OutOfMemory;
        defer alloc.free(visited);
        let i: usize = 0;
        while (i < n) : (i += 1) {
            visited[i] = false;
        }

        let queue: std.ArrayList(usize) = .empty;
        errdefer queue.deinit(alloc);

        for (direct of _.subjects.items[subj_idx].roles.items) {
            if (!visited[direct]) {
                visited[direct] = true;
                queue.append(alloc, direct) catch return RbacError.OutOfMemory;
            }
        }

        let head: usize = 0;
        while (head < queue.items.len) {
            const curr = queue.items[head];
            head += 1;
            for (parent_idx of _.roles.items[curr].parents.items) {
                if (!visited[parent_idx]) {
                    visited[parent_idx] = true;
                    queue.append(alloc, parent_idx) catch return RbacError.OutOfMemory;
                }
            }
        }

        return queue.toOwnedSlice(alloc) catch return RbacError.OutOfMemory;
    }

    /// Returns all deduplicated effective role names for `subject_id` in BFS inheritance
    /// expansion order (direct roles first, then transitive ancestors).
    /// Returned string elements borrow from the engine; caller must free the outer slice
    /// with `alloc.free(roles)`.
    pub fn effectiveRoles(alloc: std.mem.Allocator, subject_id: string): ([]string) ! RbacError {
        const opt_subj = _.subject_index.get(subject_id);
        if (opt_subj == null) {
            return alloc.alloc(string, 0) catch return RbacError.OutOfMemory;
        }
        const subj_idx = opt_subj.?;

        const indices = _.collectEffectiveRoleIndices(alloc, subj_idx)?;
        defer alloc.free(indices);

        let out = alloc.alloc(string, indices.len) catch return RbacError.OutOfMemory;
        let i: usize = 0;
        while (i < indices.len) : (i += 1) {
            out[i] = _.roles.items[indices[i]].name;
        }
        return out;
    }

    /// Evaluates whether `subject_id` is allowed to perform `requested_perm`.
    /// Follows strict AWS IAM / Casbin evaluation order:
    /// 1. Invalid `requested_perm` or unknown `subject_id` -> Default Deny.
    /// 2. Subject-level explicit Deny -> Deny (overrides everything).
    /// 3. Role-level explicit Deny across all direct & inherited roles -> Deny.
    /// 4. Role-level Allow across all direct & inherited roles -> Allow.
    /// 5. Otherwise -> Default Deny.
    pub fn check(subject_id: string, requested_perm: string): AccessDecision {
        if (!PermissionMatcher.isValidConcrete(requested_perm)) {
            return AccessDecision{
                allowed: false,
                effect: .Deny,
                matched_rule: "",
                matched_role: "",
            };
        }

        const opt_subj = _.subject_index.get(subject_id);
        if (opt_subj == null) {
            return AccessDecision{
                allowed: false,
                effect: .Deny,
                matched_rule: "",
                matched_role: "",
            };
        }
        const subj_idx = opt_subj.?;

        // 1. Subject-level explicit deny (highest precedence).
        for (pat of _.subjects.items[subj_idx].deny_rules.items) {
            if (PermissionMatcher.matches(pat, requested_perm)) {
                return AccessDecision{
                    allowed: false,
                    effect: .Deny,
                    matched_rule: pat,
                    matched_role: "",
                };
            }
        }

        const eff_indices = _.collectEffectiveRoleIndices(_.alloc, subj_idx) catch {
            return AccessDecision{
                allowed: false,
                effect: .Deny,
                matched_rule: "",
                matched_role: "",
            };
        };
        defer _.alloc.free(eff_indices);

        // 2. Role-level explicit deny across all effective (direct + inherited) roles.
        for (r_idx of eff_indices) {
            for (pat of _.roles.items[r_idx].deny_rules.items) {
                if (PermissionMatcher.matches(pat, requested_perm)) {
                    return AccessDecision{
                        allowed: false,
                        effect: .Deny,
                        matched_rule: pat,
                        matched_role: _.roles.items[r_idx].name,
                    };
                }
            }
        }

        // 3. Role-level allow across all effective roles.
        for (r_idx of eff_indices) {
            for (pat of _.roles.items[r_idx].allow_rules.items) {
                if (PermissionMatcher.matches(pat, requested_perm)) {
                    return AccessDecision{
                        allowed: true,
                        effect: .Allow,
                        matched_rule: pat,
                        matched_role: _.roles.items[r_idx].name,
                    };
                }
            }
        }

        // 4. Default deny.
        return AccessDecision{
            allowed: false,
            effect: .Deny,
            matched_rule: "",
            matched_role: "",
        };
    }
}
