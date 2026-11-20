// Source: cp/lib/rbac/permission.cp (Wildcard Permission Matcher & Policy Effect)
// Pure CP hierarchical colon-separated wildcard permission validator and matcher
// supporting exact segments, single-segment `*`, and trailing multi-segment `**`.

const std = @import("std");

/// Error set returned by RBAC policy configuration and role hierarchy operations.
pub err RbacError {
    InvalidPermission,
    RoleNotFound,
    CycleDetected,
    OutOfMemory,
}

/// Access decision effect produced by policy evaluation.
pub enum PolicyEffect: u8 {
    Allow = 0,
    Deny = 1,
}

/// Pure static validator and matcher for colon-separated hierarchical permissions
/// such as `"repo:issues:read"`, `"tool:*"`, `"*:*"`, and `"agent:tools:**"`.
pub struct PermissionMatcher {
    /// Validates a permission pattern or concrete permission string.
    /// Rules:
    /// - Must be non-empty and contain only printable non-whitespace ASCII (`0x21..=0x7e`).
    /// - Colon-separated segments (`':'`) must all be non-empty (no leading, trailing, or `"::"`).
    /// - Segments containing `'*'` must be either `"*"` (single-segment wildcard) or `"**"`
    ///   (multi-segment wildcard, allowed only as the final segment).
    pub static fn isValid(perm: string): bool {
        if (perm.len == 0) {
            return false;
        }

        let pos: usize = 0;
        while (true) {
            let end: usize = pos;
            while (end < perm.len && perm[end] != ':') : (end += 1) {
                const c = perm[end];
                if (c <= 0x20 || c >= 0x7f) {
                    return false;
                }
            }

            const seg = perm[pos..end];
            if (seg.len == 0) {
                return false;
            }

            let has_star = false;
            for (i of 0..seg.len) {
                if (seg[i] == '*') {
                    has_star = true;
                    break;
                }
            }

            if (has_star) {
                const is_single = (seg.len == 1 && seg[0] == '*');
                const is_double = (seg.len == 2 && seg[0] == '*' && seg[1] == '*');
                if (!is_single && !is_double) {
                    return false;
                }
                if (is_double && end < perm.len) {
                    // `**` is only permitted as the trailing segment.
                    return false;
                }
            }

            if (end == perm.len) {
                break;
            }
            pos = end + 1;
        }

        return true;
    }

    /// Validates a concrete requested permission (valid format and no `'*'` wildcards).
    pub static fn isValidConcrete(perm: string): bool {
        if (!PermissionMatcher.isValid(perm)) {
            return false;
        }
        for (i of 0..perm.len) {
            if (perm[i] == '*') {
                return false;
            }
        }
        return true;
    }

    /// Returns `true` if `pattern` matches `requested`.
    /// - `pattern` must satisfy `isValid(pattern)`.
    /// - `requested` must satisfy `isValidConcrete(requested)`.
    /// - `"*"` matches exactly one segment.
    /// - Trailing `"**"` matches one or more remaining segments.
    pub static fn matches(pattern: string, requested: string): bool {
        if (!PermissionMatcher.isValid(pattern)) {
            return false;
        }
        if (!PermissionMatcher.isValidConcrete(requested)) {
            return false;
        }

        let p_pos: usize = 0;
        let r_pos: usize = 0;
        let p_done = false;
        let r_done = false;

        while (!p_done) {
            let p_end: usize = p_pos;
            while (p_end < pattern.len && pattern[p_end] != ':') : (p_end += 1) {}
            const p_seg = pattern[p_pos..p_end];
            if (p_end == pattern.len) {
                p_done = true;
            } else {
                p_pos = p_end + 1;
            }

            if (p_seg.len == 2 && p_seg[0] == '*' && p_seg[1] == '*') {
                // Trailing `**` matches 1 or more remaining segments in `requested`.
                return !r_done;
            }

            if (r_done) {
                return false;
            }

            let r_end: usize = r_pos;
            while (r_end < requested.len && requested[r_end] != ':') : (r_end += 1) {}
            const r_seg = requested[r_pos..r_end];
            if (r_end == requested.len) {
                r_done = true;
            } else {
                r_pos = r_end + 1;
            }

            if (!(p_seg.len == 1 && p_seg[0] == '*')) {
                if (!std.mem.eql(u8, p_seg, r_seg)) {
                    return false;
                }
            }
        }

        return r_done;
    }
}
