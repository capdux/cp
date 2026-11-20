// Source: bun/src/paths/resolve_path_.zig -> cp/lib/paths/ops.cp (Path Normalization, Join, Resolve & Relative)
// Pure CP cross-platform path normalize, join, resolve, and relative operations.

const std = @import("std");
import {
    PathError,
    PathStyle,
    isSep,
    sepChar,
    isDriveLetter,
    eqlIgnoreCase,
    containsNullByte,
    windowsUncVolumeLen,
    isAbsolute,
    trimTrailingSlashes,
} from "./types.cp";

struct RootSplit {
    prefix: string,
    is_unc: bool,
    unc_server: string,
    unc_share: string,
    is_abs: bool,
    rest: string,
}

fn splitRoot(path: string, style: PathStyle): RootSplit ! PathError {
    if (style == .Posix) {
        if (path.len > 0 && path[0] == '/') {
            return RootSplit{
                prefix: "/",
                is_unc: false,
                unc_server: "",
                unc_share: "",
                is_abs: true,
                rest: path[1..path.len],
            };
        }
        return RootSplit{
            prefix: "",
            is_unc: false,
            unc_server: "",
            unc_share: "",
            is_abs: false,
            rest: path,
        };
    }

    // Windows UNC: \\server\share...
    const unc_vol = windowsUncVolumeLen(path);
    if (unc_vol > 0) {
        let i: usize = 2;
        while (i < unc_vol && !isSep(path[i], .Windows)) {
            i += 1;
        }
        const srv = path[2..i];
        const shr = path[(i + 1)..unc_vol];
        let rest_start = unc_vol;
        if (rest_start < path.len && isSep(path[rest_start], .Windows)) {
            rest_start += 1;
        }
        return RootSplit{
            prefix: "",
            is_unc: true,
            unc_server: srv,
            unc_share: shr,
            is_abs: true,
            rest: path[rest_start..path.len],
        };
    }

    // Windows drive letter: C:\ or C:
    if (path.len >= 2 && path[1] == ':') {
        if (!isDriveLetter(path[0])) {
            return PathError.InvalidDriveLetter;
        }
        if (path.len >= 3 && isSep(path[2], .Windows)) {
            return RootSplit{
                prefix: path[0..2],
                is_unc: false,
                unc_server: "",
                unc_share: "",
                is_abs: true,
                rest: path[3..path.len],
            };
        }
        return RootSplit{
            prefix: path[0..2],
            is_unc: false,
            unc_server: "",
            unc_share: "",
            is_abs: false,
            rest: path[2..path.len],
        };
    }

    // Windows root backslash/slash
    if (path.len > 0 && isSep(path[0], .Windows)) {
        return RootSplit{
            prefix: "\\",
            is_unc: false,
            unc_server: "",
            unc_share: "",
            is_abs: true,
            rest: path[1..path.len],
        };
    }

    return RootSplit{
        prefix: "",
        is_unc: false,
        unc_server: "",
        unc_share: "",
        is_abs: false,
        rest: path,
    };
}

fn collectSegments(
    alloc: std.mem.Allocator,
    rest: string,
    style: PathStyle,
    is_abs: bool,
    out_segs: *std.ArrayList(string),
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
        const seg = rest[start..i];
        if (std.mem.eql(u8, seg, ".")) {
            continue;
        }
        if (std.mem.eql(u8, seg, "..")) {
            if (out_segs.items.len > 0 && !std.mem.eql(u8, out_segs.items[out_segs.items.len - 1], "..")) {
                * = out_segs.pop();
            } else if (!is_abs) {
                out_segs.append(alloc, "..") catch return PathError.OutOfMemory;
            }
            continue;
        }
        out_segs.append(alloc, seg) catch return PathError.OutOfMemory;
    }
}

/// Normalizes `path`, collapsing `.`, `..`, and redundant separators while guarding root boundaries.
/// Caller owns returned slice.
pub fn normalize(alloc: std.mem.Allocator, path: string, style: PathStyle): string ! PathError {
    if (containsNullByte(path)) {
        return PathError.NullByteFound;
    }
    if (path.len == 0) {
        return alloc.dupe(u8, ".") catch return PathError.OutOfMemory;
    }

    const split = splitRoot(path, style)?;
    let segs: std.ArrayList(string) = .empty;
    defer segs.deinit(alloc);

    collectSegments(alloc, split.rest, style, split.is_abs, &segs)?;

    const sep = sepChar(style);
    const trailing = path.len > 0 && isSep(path[path.len - 1], style);

    let buf: std.ArrayList(u8) = .empty;
    errdefer buf.deinit(alloc);

    if (split.is_unc) {
        buf.appendSlice(alloc, "\\\\") catch return PathError.OutOfMemory;
        buf.appendSlice(alloc, split.unc_server) catch return PathError.OutOfMemory;
        buf.append(alloc, '\\') catch return PathError.OutOfMemory;
        buf.appendSlice(alloc, split.unc_share) catch return PathError.OutOfMemory;
        buf.append(alloc, '\\') catch return PathError.OutOfMemory;
    } else if (split.prefix.len > 0) {
        buf.appendSlice(alloc, split.prefix) catch return PathError.OutOfMemory;
        if (style == .Windows && split.is_abs && !std.mem.eql(u8, split.prefix, "\\")) {
            buf.append(alloc, '\\') catch return PathError.OutOfMemory;
        }
    }

    if (segs.items.len == 0) {
        if (!split.is_abs && split.prefix.len == 0) {
            buf.append(alloc, '.') catch return PathError.OutOfMemory;
            if (trailing) {
                buf.append(alloc, sep) catch return PathError.OutOfMemory;
            }
        } else if (!split.is_abs && split.prefix.len > 0) {
            // Windows drive-relative "C:" or "C:."
            buf.append(alloc, '.') catch return PathError.OutOfMemory;
        }
        return buf.toOwnedSlice(alloc) catch return PathError.OutOfMemory;
    }

    let idx: usize = 0;
    while (idx < segs.items.len) {
        if (idx > 0) {
            buf.append(alloc, sep) catch return PathError.OutOfMemory;
        }
        buf.appendSlice(alloc, segs.items[idx]) catch return PathError.OutOfMemory;
        idx += 1;
    }

    if (trailing) {
        buf.append(alloc, sep) catch return PathError.OutOfMemory;
    }

    return buf.toOwnedSlice(alloc) catch return PathError.OutOfMemory;
}

/// Joins `parts` with the style separator and normalizes the resulting path.
/// Caller owns returned slice.
pub fn join(alloc: std.mem.Allocator, parts: []const string, style: PathStyle): string ! PathError {
    let i: usize = 0;
    while (i < parts.len) {
        if (containsNullByte(parts[i])) {
            return PathError.NullByteFound;
        }
        i += 1;
    }

    const sep = sepChar(style);
    let buf: std.ArrayList(u8) = .empty;
    defer buf.deinit(alloc);

    let first = true;
    i = 0;
    while (i < parts.len) {
        const part = parts[i];
        i += 1;
        if (part.len == 0) {
            continue;
        }
        if (!first) {
            buf.append(alloc, sep) catch return PathError.OutOfMemory;
        }
        buf.appendSlice(alloc, part) catch return PathError.OutOfMemory;
        first = false;
    }

    if (buf.items.len == 0) {
        return alloc.dupe(u8, ".") catch return PathError.OutOfMemory;
    }

    return normalize(alloc, buf.items, style);
}

/// Resolves a sequence of paths from right to left into an absolute normalized path.
/// If no part is absolute, anchors at the style root (`/` or `\`).
/// Caller owns returned slice.
pub fn resolve(alloc: std.mem.Allocator, parts: []const string, style: PathStyle): string ! PathError {
    let i: usize = 0;
    while (i < parts.len) {
        if (containsNullByte(parts[i])) {
            return PathError.NullByteFound;
        }
        i += 1;
    }

    // Find rightmost absolute part index
    let start_idx: usize = 0;
    let found_abs = false;
    let k: usize = parts.len;
    while (k > 0) {
        k -= 1;
        if (parts[k].len > 0 && isAbsolute(parts[k], style)) {
            start_idx = k;
            found_abs = true;
            break;
        }
    }

    const sep = sepChar(style);
    let buf: std.ArrayList(u8) = .empty;
    defer buf.deinit(alloc);

    if (!found_abs) {
        buf.append(alloc, sep) catch return PathError.OutOfMemory;
    }

    let idx: usize = start_idx;
    while (idx < parts.len) {
        const p = parts[idx];
        idx += 1;
        if (p.len == 0) {
            continue;
        }
        if (buf.items.len > 0 && !isSep(buf.items[buf.items.len - 1], style)) {
            buf.append(alloc, sep) catch return PathError.OutOfMemory;
        }
        buf.appendSlice(alloc, p) catch return PathError.OutOfMemory;
    }

    const raw_norm = normalize(alloc, buf.items, style)?;
    const trimmed = trimTrailingSlashes(raw_norm, style);
    if (trimmed.len == raw_norm.len) {
        return raw_norm;
    }
    const res = alloc.dupe(u8, trimmed) catch {
        alloc.free(raw_norm);
        return PathError.OutOfMemory;
    };
    alloc.free(raw_norm);
    return res;
}

fn rootsEqual(a: RootSplit, b: RootSplit, style: PathStyle): bool {
    if (a.is_abs != b.is_abs) {
        return false;
    }
    if (a.is_unc != b.is_unc) {
        return false;
    }
    if (a.is_unc) {
        return eqlIgnoreCase(a.unc_server, b.unc_server) && eqlIgnoreCase(a.unc_share, b.unc_share);
    }
    if (style == .Windows) {
        return eqlIgnoreCase(a.prefix, b.prefix);
    }
    return std.mem.eql(u8, a.prefix, b.prefix);
}

fn segEqual(a: string, b: string, style: PathStyle): bool {
    if (style == .Windows) {
        return eqlIgnoreCase(a, b);
    }
    return std.mem.eql(u8, a, b);
}

/// Computes the relative path from `from_path` to `to_path` under `style`.
/// Caller owns returned slice.
pub fn relative(alloc: std.mem.Allocator, from_path: string, to_path: string, style: PathStyle): string ! PathError {
    if (containsNullByte(from_path) || containsNullByte(to_path)) {
        return PathError.NullByteFound;
    }

    const norm_from_raw = normalize(alloc, from_path, style)?;
    defer alloc.free(norm_from_raw);
    const norm_to_raw = normalize(alloc, to_path, style)?;
    defer alloc.free(norm_to_raw);

    const norm_from = trimTrailingSlashes(norm_from_raw, style);
    const norm_to = trimTrailingSlashes(norm_to_raw, style);

    const from_split = splitRoot(norm_from, style)?;
    const to_split = splitRoot(norm_to, style)?;

    if (!rootsEqual(from_split, to_split, style)) {
        return alloc.dupe(u8, norm_to) catch return PathError.OutOfMemory;
    }

    let from_segs: std.ArrayList(string) = .empty;
    defer from_segs.deinit(alloc);
    collectSegments(alloc, from_split.rest, style, from_split.is_abs, &from_segs)?;

    let to_segs: std.ArrayList(string) = .empty;
    defer to_segs.deinit(alloc);
    collectSegments(alloc, to_split.rest, style, to_split.is_abs, &to_segs)?;

    let common: usize = 0;
    const min_len = if (from_segs.items.len < to_segs.items.len) from_segs.items.len else to_segs.items.len;
    while (common < min_len && segEqual(from_segs.items[common], to_segs.items[common], style)) {
        common += 1;
    }

    const up_count = from_segs.items.len - common;
    if (up_count == 0 && common == to_segs.items.len) {
        return alloc.dupe(u8, "") catch return PathError.OutOfMemory;
    }

    const sep = sepChar(style);
    let out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(alloc);

    let first = true;
    let u: usize = 0;
    while (u < up_count) {
        if (!first) {
            out.append(alloc, sep) catch return PathError.OutOfMemory;
        }
        out.appendSlice(alloc, "..") catch return PathError.OutOfMemory;
        first = false;
        u += 1;
    }

    let t: usize = common;
    while (t < to_segs.items.len) {
        if (!first) {
            out.append(alloc, sep) catch return PathError.OutOfMemory;
        }
        out.appendSlice(alloc, to_segs.items[t]) catch return PathError.OutOfMemory;
        first = false;
        t += 1;
    }

    return out.toOwnedSlice(alloc) catch return PathError.OutOfMemory;
}
