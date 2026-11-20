// Source: cp/lib/paths/security.cp (Path Boundary Guards, Safe Join & Simple Glob)
// Pure CP path traversal guards (`isSubpath`, `safeJoin`) and zero-allocation glob helper.

const std = @import("std");
import {
    PathError,
    PathStyle,
    isSep,
    isDriveLetter,
    eqlIgnoreCase,
    containsNullByte,
    isAbsolute,
    trimTrailingSlashes,
    rootLen,
} from "./types.cp";
import { normalize } from "./ops.cp";
import * as ops from "./ops.cp";

fn segEqual(a: string, b: string, style: PathStyle): bool {
    if (style == .Windows) {
        return eqlIgnoreCase(a, b);
    }
    return std.mem.eql(u8, a, b);
}

fn rootsMatch(a_root: string, b_root: string, style: PathStyle): bool {
    if (style == .Windows) {
        // Normalize '/' to '\' when comparing Windows roots
        if (a_root.len != b_root.len) {
            return false;
        }
        let i: usize = 0;
        while (i < a_root.len) {
            const ca = if (a_root[i] == '/') '\\' else a_root[i];
            const cb = if (b_root[i] == '/') '\\' else b_root[i];
            if (ca != cb) {
                // Case-insensitive drive/UNC comparison
                const la = if (ca >= 'A' && ca <= 'Z') ca + 32 else ca;
                const lb = if (cb >= 'A' && cb <= 'Z') cb + 32 else cb;
                if (la != lb) {
                    return false;
                }
            }
            i += 1;
        }
        return true;
    }
    return std.mem.eql(u8, a_root, b_root);
}

fn splitSegmentsSlice(
    alloc: std.mem.Allocator,
    rest: string,
    style: PathStyle,
    out: *std.ArrayList(string),
): void ! PathError {
    let i: usize = 0;
    while (i < rest.len) {
        while (i < rest.len && isSep(rest[i], style)) {
            i += 1;
        }
        if (i >= rest.len) {
            break;
        }
        const start = i;
        while (i < rest.len && !isSep(rest[i], style)) {
            i += 1;
        }
        out.append(alloc, rest[start..i]) catch return PathError.OutOfMemory;
    }
}

/// Returns true if `target_path` resides inside (or equals) `base_dir` after normalization.
/// Rejects prefix traps such as `/var/www` vs `/var/www2`.
pub fn isSubpath(
    alloc: std.mem.Allocator,
    base_dir: string,
    target_path: string,
    style: PathStyle,
): bool ! PathError {
    if (containsNullByte(base_dir) || containsNullByte(target_path)) {
        return PathError.NullByteFound;
    }

    const norm_base_raw = normalize(alloc, base_dir, style)?;
    defer alloc.free(norm_base_raw);
    const norm_target_raw = normalize(alloc, target_path, style)?;
    defer alloc.free(norm_target_raw);

    const norm_base = trimTrailingSlashes(norm_base_raw, style);
    const norm_target = trimTrailingSlashes(norm_target_raw, style);

    const base_rlen = rootLen(norm_base, style);
    const target_rlen = rootLen(norm_target, style);
    if (!rootsMatch(norm_base[0..base_rlen], norm_target[0..target_rlen], style)) {
        return false;
    }

    let base_segs: std.ArrayList(string) = .empty;
    defer base_segs.deinit(alloc);
    splitSegmentsSlice(alloc, norm_base[base_rlen..norm_base.len], style, &base_segs)?;

    let target_segs: std.ArrayList(string) = .empty;
    defer target_segs.deinit(alloc);
    splitSegmentsSlice(alloc, norm_target[target_rlen..norm_target.len], style, &target_segs)?;

    // Relative base starting with ".." cannot be safely checked against fewer segments
    if (target_segs.items.len < base_segs.items.len) {
        return false;
    }

    let i: usize = 0;
    while (i < base_segs.items.len) {
        if (!segEqual(base_segs.items[i], target_segs.items[i], style)) {
            return false;
        }
        i += 1;
    }

    return true;
}

/// Safely joins `rel_parts` onto `base_dir`, rejecting null bytes, absolute/drive-letter injections,
/// and `../` traversal escapes outside `base_dir`. Caller owns returned slice.
pub fn safeJoin(
    alloc: std.mem.Allocator,
    base_dir: string,
    rel_parts: []const string,
    style: PathStyle,
): string ! PathError {
    if (containsNullByte(base_dir)) {
        return PathError.NullByteFound;
    }

    let i: usize = 0;
    while (i < rel_parts.len) {
        const part = rel_parts[i];
        if (containsNullByte(part)) {
            return PathError.NullByteFound;
        }
        if (isAbsolute(part, style)) {
            return PathError.PathTraversal;
        }
        if (style == .Windows && part.len >= 2 && isDriveLetter(part[0]) && part[1] == ':') {
            return PathError.PathTraversal;
        }
        i += 1;
    }

    const norm_base_raw = normalize(alloc, base_dir, style)?;
    defer alloc.free(norm_base_raw);
    const norm_base = trimTrailingSlashes(norm_base_raw, style);

    let all_parts: std.ArrayList(string) = .empty;
    defer all_parts.deinit(alloc);
    all_parts.append(alloc, norm_base) catch return PathError.OutOfMemory;

    i = 0;
    while (i < rel_parts.len) {
        all_parts.append(alloc, rel_parts[i]) catch return PathError.OutOfMemory;
        i += 1;
    }

    const joined = ops.join(alloc, all_parts.items, style)?;
    errdefer alloc.free(joined);

    const ok = isSubpath(alloc, norm_base, joined, style)?;
    if (!ok) {
        alloc.free(joined);
        return PathError.PathTraversal;
    }

    return joined;
}

/// Zero-allocation simple glob matcher supporting `*` (any chars) and `?` (single char).
pub fn matchGlobSimple(path: string, pattern: string): bool {
    let p_idx: usize = 0;
    let s_idx: usize = 0;
    let star_idx: usize = pattern.len;
    let match_idx: usize = 0;
    let has_star = false;

    while (s_idx < path.len) {
        if (p_idx < pattern.len && (pattern[p_idx] == '?' || pattern[p_idx] == path[s_idx])) {
            p_idx += 1;
            s_idx += 1;
        } else if (p_idx < pattern.len && pattern[p_idx] == '*') {
            star_idx = p_idx;
            match_idx = s_idx;
            has_star = true;
            p_idx += 1;
        } else if (has_star) {
            p_idx = star_idx + 1;
            match_idx += 1;
            s_idx = match_idx;
        } else {
            return false;
        }
    }

    while (p_idx < pattern.len && pattern[p_idx] == '*') {
        p_idx += 1;
    }

    return p_idx == pattern.len;
}
