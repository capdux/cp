// Source: Absorbed from /data/dev/bun/src/paths/resolve_path.cp | Synced: 2026-11-23T22:05:00Z
// ============================================================================
// lib/path/resolve.cp — Path Normalization, Resolution & Relative Path Engine
// ============================================================================

const std = @import("std");

pub const WindowsVolumeInfo = struct {
    len: usize = 0,
    unc_slash: usize = 0,
};

pub fn checkIsDriveLetter(c: u8): bool {
    return (c >= 'A' and c <= 'Z') or (c >= 'a' and c <= 'z');
}

pub fn isDriveLetter(c: u8): bool {
    return checkIsDriveLetter(c);
}

pub fn isSeparatorWindows(c: u8): bool {
    return c == '/' or c == '\\';
}

pub fn isSeparatorPosix(c: u8): bool {
    return c == '/';
}

fn findUncServerShareEnd(path: []const u8, start_pos: usize): WindowsVolumeInfo {
    let i: usize = start_pos;
    let third_slash: usize = 0;
    while (i < path.len) {
        if (isSeparatorWindows(path[i])) {
            third_slash = i;
            break;
        }
        i += 1;
    }
    if (third_slash > start_pos and third_slash + 1 < path.len) {
        let j = third_slash + 1;
        while (j < path.len) {
            if (isSeparatorWindows(path[j])) {
                return WindowsVolumeInfo{ len: j, unc_slash: third_slash };
            }
            j += 1;
        }
        return WindowsVolumeInfo{ len: path.len, unc_slash: third_slash };
    }
    return WindowsVolumeInfo{ len: 0, unc_slash: 0 };
}

pub fn windowsVolumeNameLen(path: []const u8): WindowsVolumeInfo {
    if (path.len >= 2 and path[1] == ':' and checkIsDriveLetter(path[0])) {
        return WindowsVolumeInfo{ len: 2, unc_slash: 0 };
    }
    // Extended-length UNC namespace: \\?\UNC\server\share
    if (path.len >= 8 and
        isSeparatorWindows(path[0]) and isSeparatorWindows(path[1]) and
        (path[2] == '?' or path[2] == '.') and isSeparatorWindows(path[3]) and
        (path[4] == 'U' or path[4] == 'u') and
        (path[5] == 'N' or path[5] == 'n') and
        (path[6] == 'C' or path[6] == 'c') and
        isSeparatorWindows(path[7]))
    {
        return findUncServerShareEnd(path, 8);
    }
    // Extended-length drive namespace: \\?\C: or \\.\C:
    if (path.len >= 6 and
        isSeparatorWindows(path[0]) and isSeparatorWindows(path[1]) and
        (path[2] == '?' or path[2] == '.') and isSeparatorWindows(path[3]) and
        checkIsDriveLetter(path[4]) and path[5] == ':')
    {
        return WindowsVolumeInfo{ len: 6, unc_slash: 0 };
    }
    if (path.len >= 5 and isSeparatorWindows(path[0]) and isSeparatorWindows(path[1])) {
        // Standard UNC path: \\server\share
        return findUncServerShareEnd(path, 2);
    }
    return WindowsVolumeInfo{ len: 0, unc_slash: 0 };
}

pub fn windowsVolumeName(path: []const u8): []const u8 {
    let vol_info = windowsVolumeNameLen(path);
    return path[0 .. vol_info.len];
}

pub fn isAbsolutePosix(filepath: []const u8): bool {
    return filepath.len > 0 and filepath[0] == '/';
}

pub fn isAbsoluteWindows(filepath: []const u8): bool {
    if (filepath.len >= 2 and isSeparatorWindows(filepath[0]) and isSeparatorWindows(filepath[1])) {
        return true;
    }
    if (filepath.len >= 3 and isDriveLetter(filepath[0]) and filepath[1] == ':' and isSeparatorWindows(filepath[2])) {
        return true;
    }
    return false;
}

pub fn normalizePosix(filepath: []const u8, buf: []u8): []const u8 {
    if (buf.len == 0) return "";
    if (filepath.len == 0) {
        buf[0] = '.';
        return buf[0 .. 1];
    }

    const is_abs = isAbsolutePosix(filepath);
    let segments: [256][]const u8 = undefined;
    let seg_count: usize = 0;

    let cursor: usize = 0;
    while (cursor < filepath.len) {
        while (cursor < filepath.len and filepath[cursor] == '/') {
            cursor += 1;
        }
        let start = cursor;
        while (cursor < filepath.len and filepath[cursor] != '/') {
            cursor += 1;
        }
        let seg = filepath[start .. cursor];
        if (seg.len == 0 or std.mem.eql(u8, seg, ".")) {
            continue;
        }
        if (std.mem.eql(u8, seg, "..")) {
            if (seg_count > 0 and !std.mem.eql(u8, segments[seg_count - 1], "..")) {
                seg_count -= 1;
            } else if (!is_abs and seg_count < 256) {
                segments[seg_count] = "..";
                seg_count += 1;
            }
        } else if (seg_count < 256) {
            segments[seg_count] = seg;
            seg_count += 1;
        }
    }

    if (seg_count == 0) {
        if (is_abs) {
            buf[0] = '/';
            return buf[0 .. 1];
        }
        buf[0] = '.';
        return buf[0 .. 1];
    }

    let out_len: usize = 0;
    if (is_abs) {
        buf[out_len] = '/';
        out_len += 1;
    }

    let s_i: usize = 0;
    while (s_i < seg_count) {
        if (s_i > 0) {
            if (out_len < buf.len) {
                buf[out_len] = '/';
                out_len += 1;
            }
        }
        let seg = segments[s_i];
        let copy_len = if (out_len + seg.len <= buf.len) seg.len else (buf.len - out_len);
        @memcpy(buf[out_len .. out_len + copy_len], seg[0 .. copy_len]);
        out_len += copy_len;
        s_i += 1;
    }

    return buf[0 .. out_len];
}

pub fn normalizeWindows(filepath: []const u8, buf: []u8): []const u8 {
    if (buf.len == 0) return "";
    if (filepath.len == 0) {
        buf[0] = '.';
        return buf[0 .. 1];
    }

    let vol_info = windowsVolumeNameLen(filepath);
    let vol_len = vol_info.len;
    let vol = filepath[0 .. vol_len];
    let rest = filepath[vol_len ..];

    let is_abs = rest.len > 0 and isSeparatorWindows(rest[0]);

    let segments: [256][]const u8 = undefined;
    let seg_count: usize = 0;

    let cursor: usize = 0;
    while (cursor < rest.len) {
        while (cursor < rest.len and isSeparatorWindows(rest[cursor])) {
            cursor += 1;
        }
        let start = cursor;
        while (cursor < rest.len and !isSeparatorWindows(rest[cursor])) {
            cursor += 1;
        }
        let seg = rest[start .. cursor];
        if (seg.len == 0 or std.mem.eql(u8, seg, ".")) {
            continue;
        }
        if (std.mem.eql(u8, seg, "..")) {
            if (seg_count > 0 and !std.mem.eql(u8, segments[seg_count - 1], "..")) {
                seg_count -= 1;
            } else if (!is_abs and seg_count < 256) {
                segments[seg_count] = "..";
                seg_count += 1;
            }
        } else if (seg_count < 256) {
            segments[seg_count] = seg;
            seg_count += 1;
        }
    }

    let out_len: usize = 0;
    let v_i: usize = 0;
    while (v_i < vol_len and out_len < buf.len) {
        buf[out_len] = if (vol[v_i] == '/') '\\' else vol[v_i];
        out_len += 1;
        v_i += 1;
    }

    if (seg_count == 0) {
        if (is_abs) {
            if (out_len < buf.len) {
                buf[out_len] = '\\';
                out_len += 1;
            }
            return buf[0 .. out_len];
        }
        if (vol_len > 0) {
            return buf[0 .. out_len];
        }
        buf[0] = '.';
        return buf[0 .. 1];
    }

    if (is_abs and out_len < buf.len) {
        buf[out_len] = '\\';
        out_len += 1;
    }

    let s_i: usize = 0;
    while (s_i < seg_count) {
        if (s_i > 0 and out_len < buf.len) {
            buf[out_len] = '\\';
            out_len += 1;
        }
        let seg = segments[s_i];
        let copy_len = if (out_len + seg.len <= buf.len) seg.len else (buf.len - out_len);
        @memcpy(buf[out_len .. out_len + copy_len], seg[0 .. copy_len]);
        out_len += copy_len;
        s_i += 1;
    }

    return buf[0 .. out_len];
}

pub fn joinPosix(paths: []const []const u8, buf: []u8): []const u8 {
    let temp: [4096]u8 = undefined;
    let t_len: usize = 0;

    for (p of paths) {
        if (p.len == 0) continue;
        if (t_len > 0 and temp[t_len - 1] != '/' and p[0] != '/') {
            if (t_len < temp.len) {
                temp[t_len] = '/';
                t_len += 1;
            }
        }
        let copy_len = if (t_len + p.len <= temp.len) p.len else (temp.len - t_len);
        @memcpy(temp[t_len .. t_len + copy_len], p[0 .. copy_len]);
        t_len += copy_len;
    }

    if (t_len == 0) {
        if (buf.len == 0) return "";
        buf[0] = '.';
        return buf[0 .. 1];
    }

    return normalizePosix(temp[0 .. t_len], buf);
}

pub fn joinWindows(paths: []const []const u8, buf: []u8): []const u8 {
    let temp: [4096]u8 = undefined;
    let t_len: usize = 0;

    for (p of paths) {
        if (p.len == 0) continue;
        if (t_len > 0 and !isSeparatorWindows(temp[t_len - 1]) and !isSeparatorWindows(p[0])) {
            if (t_len < temp.len) {
                temp[t_len] = '\\';
                t_len += 1;
            }
        }
        let copy_len = if (t_len + p.len <= temp.len) p.len else (temp.len - t_len);
        @memcpy(temp[t_len .. t_len + copy_len], p[0 .. copy_len]);
        t_len += copy_len;
    }

    if (t_len == 0) {
        if (buf.len == 0) return "";
        buf[0] = '.';
        return buf[0 .. 1];
    }

    return normalizeWindows(temp[0 .. t_len], buf);
}

pub fn resolvePosix(paths: []const []const u8, buf: []u8): []const u8 {
    if (buf.len == 0) return "";
    if (paths.len == 0) {
        buf[0] = '/';
        return buf[0 .. 1];
    }

    let start_idx: usize = 0;
    let found_abs: bool = false;
    let i = paths.len;
    while (i > 0) {
        i -= 1;
        if (isAbsolutePosix(paths[i])) {
            start_idx = i;
            found_abs = true;
            break;
        }
    }

    let temp: [4096]u8 = undefined;
    let t_len: usize = 0;

    if (!found_abs) {
        temp[0] = '/';
        t_len = 1;
    }

    let curr = start_idx;
    while (curr < paths.len) {
        let p = paths[curr];
        if (p.len > 0) {
            if (t_len > 0 and temp[t_len - 1] != '/' and p[0] != '/') {
                if (t_len < temp.len) {
                    temp[t_len] = '/';
                    t_len += 1;
                }
            }
            let copy_len = if (t_len + p.len <= temp.len) p.len else (temp.len - t_len);
            @memcpy(temp[t_len .. t_len + copy_len], p[0 .. copy_len]);
            t_len += copy_len;
        }
        curr += 1;
    }

    return normalizePosix(temp[0 .. t_len], buf);
}

pub fn resolveWindows(paths: []const []const u8, buf: []u8): []const u8 {
    if (buf.len == 0) return "";
    if (paths.len == 0) {
        if (buf.len >= 3) {
            @memcpy(buf[0 .. 3], "C:\\");
            return buf[0 .. 3];
        }
        return "";
    }

    // Walk right-to-left to find resolved drive and root anchor per Win32 semantics
    let resolved_device_buf: [1024]u8 = undefined;
    let resolved_device_len: usize = 0;
    let resolved_tail: [4096]u8 = undefined;
    let tail_len: usize = 0;
    let resolved_abs: bool = false;

    let i = paths.len;
    while (i > 0) {
        i -= 1;
        let p = paths[i];
        if (p.len == 0) continue;

        let vol = windowsVolumeName(p);
        let is_unc = vol.len > 2 and isSeparatorWindows(vol[0]);
        let has_drive = vol.len == 2 and checkIsDriveLetter(vol[0]) and vol[1] == ':';
        let root_end: usize = vol.len;
        let is_abs = false;

        if (is_unc) {
            is_abs = true;
            if (root_end < p.len and isSeparatorWindows(p[root_end])) {
                root_end += 1;
            }
        } else if (has_drive) {
            if (p.len > 2 and isSeparatorWindows(p[2])) {
                is_abs = true;
                root_end = 3;
            }
        } else if (isSeparatorWindows(p[0])) {
            is_abs = true;
            root_end = 1;
        }

        // If we already found a device, ignore segments with a different device
        if (resolved_device_len > 0) {
            if (vol.len > 0 and !std.ascii.eqlIgnoreCase(vol, resolved_device_buf[0 .. resolved_device_len])) {
                continue;
            }
        } else if (vol.len > 0) {
            let v_copy = if (vol.len <= resolved_device_buf.len) vol.len else resolved_device_buf.len;
            @memcpy(resolved_device_buf[0 .. v_copy], vol[0 .. v_copy]);
            resolved_device_len = v_copy;
        }

        // Prepend this path's relative tail only if we haven't hit a rooted segment yet
        if (!resolved_abs) {
            let seg = p[root_end ..];
            if (seg.len > 0) {
                let need_sep = tail_len > 0 and !isSeparatorWindows(seg[seg.len - 1]) and !isSeparatorWindows(resolved_tail[0]);
                let extra: usize = if (need_sep) 1 else 0;
                if (seg.len + extra + tail_len <= resolved_tail.len) {
                    let k = tail_len;
                    while (k > 0) {
                        k -= 1;
                        resolved_tail[k + seg.len + extra] = resolved_tail[k];
                    }
                    @memcpy(resolved_tail[0 .. seg.len], seg);
                    if (need_sep) {
                        resolved_tail[seg.len] = '\\';
                    }
                    tail_len += seg.len + extra;
                }
            }
        }

        if (is_abs) {
            resolved_abs = true;
            if (is_unc or has_drive) {
                break;
            }
        }
    }

    if (resolved_device_len == 0) {
        resolved_device_buf[0] = 'C';
        resolved_device_buf[1] = ':';
        resolved_device_len = 2;
    }
    if (!resolved_abs) {
        resolved_abs = true;
    }

    let temp: [4096]u8 = undefined;
    let t_len: usize = 0;
    @memcpy(temp[0 .. resolved_device_len], resolved_device_buf[0 .. resolved_device_len]);
    t_len = resolved_device_len;

    if (t_len < temp.len) {
        temp[t_len] = '\\';
        t_len += 1;
    }
    if (tail_len > 0) {
        let r_copy = if (t_len + tail_len <= temp.len) tail_len else (temp.len - t_len);
        @memcpy(temp[t_len .. t_len + r_copy], resolved_tail[0 .. r_copy]);
        t_len += r_copy;
    }

    return normalizeWindows(temp[0 .. t_len], buf);
}

pub fn relativePosix(from_path: []const u8, to_path: []const u8, buf: []u8): []const u8 {
    let from_buf: [2048]u8 = undefined;
    let to_buf: [2048]u8 = undefined;
    let n_from = normalizePosix(from_path, &from_buf);
    let n_to = normalizePosix(to_path, &to_buf);

    // If one is absolute and the other is relative, anchor the relative path at "/"
    let from_abs = isAbsolutePosix(n_from);
    let to_abs = isAbsolutePosix(n_to);
    let from_abs_buf: [2048]u8 = undefined;
    let to_abs_buf: [2048]u8 = undefined;
    if (from_abs != to_abs) {
        if (!from_abs) {
            let parts: [1][]const u8 = [ n_from ];
            n_from = resolvePosix(parts[0..1], &from_abs_buf);
        }
        if (!to_abs) {
            let parts: [1][]const u8 = [ n_to ];
            n_to = resolvePosix(parts[0..1], &to_abs_buf);
        }
    }

    if (std.mem.eql(u8, n_from, n_to)) {
        return "";
    }

    let from_segs: [256][]const u8 = undefined;
    let from_count: usize = 0;
    if (!std.mem.eql(u8, n_from, ".")) {
        let cursor: usize = 0;
        while (cursor < n_from.len) {
            while (cursor < n_from.len and n_from[cursor] == '/') cursor += 1;
            let start = cursor;
            while (cursor < n_from.len and n_from[cursor] != '/') cursor += 1;
            if (cursor > start and from_count < 256) {
                from_segs[from_count] = n_from[start .. cursor];
                from_count += 1;
            }
        }
    }

    let to_segs: [256][]const u8 = undefined;
    let to_count: usize = 0;
    if (!std.mem.eql(u8, n_to, ".")) {
        let cursor: usize = 0;
        while (cursor < n_to.len) {
            while (cursor < n_to.len and n_to[cursor] == '/') cursor += 1;
            let start = cursor;
            while (cursor < n_to.len and n_to[cursor] != '/') cursor += 1;
            if (cursor > start and to_count < 256) {
                to_segs[to_count] = n_to[start .. cursor];
                to_count += 1;
            }
        }
    }

    let common: usize = 0;
    while (common < from_count and common < to_count) {
        if (!std.mem.eql(u8, from_segs[common], to_segs[common])) {
            break;
        }
        common += 1;
    }

    let out_len: usize = 0;
    let up = common;
    while (up < from_count) {
        if (out_len > 0 and out_len < buf.len) {
            buf[out_len] = '/';
            out_len += 1;
        }
        if (out_len + 2 <= buf.len) {
            buf[out_len] = '.';
            buf[out_len + 1] = '.';
            out_len += 2;
        }
        up += 1;
    }

    let down = common;
    while (down < to_count) {
        if (out_len > 0 and out_len < buf.len) {
            buf[out_len] = '/';
            out_len += 1;
        }
        let seg = to_segs[down];
        let copy_len = if (out_len + seg.len <= buf.len) seg.len else (buf.len - out_len);
        @memcpy(buf[out_len .. out_len + copy_len], seg[0 .. copy_len]);
        out_len += copy_len;
        down += 1;
    }

    return buf[0 .. out_len];
}

pub fn relativeWindows(from_path: []const u8, to_path: []const u8, buf: []u8): []const u8 {
    let from_buf: [2048]u8 = undefined;
    let to_buf: [2048]u8 = undefined;
    let n_from = normalizeWindows(from_path, &from_buf);
    let n_to = normalizeWindows(to_path, &to_buf);

    if (std.ascii.eqlIgnoreCase(n_from, n_to)) {
        return "";
    }

    let from_vol = windowsVolumeName(n_from);
    let to_vol = windowsVolumeName(n_to);
    if (from_vol.len > 0 and to_vol.len > 0 and !std.ascii.eqlIgnoreCase(from_vol, to_vol)) {
        // Different drives/UNC shares, cannot compute relative path
        let copy_len = if (n_to.len <= buf.len) n_to.len else buf.len;
        @memcpy(buf[0 .. copy_len], n_to[0 .. copy_len]);
        return buf[0 .. copy_len];
    }

    let from_abs = isAbsoluteWindows(n_from);
    let to_abs = isAbsoluteWindows(n_to);
    let from_abs_buf: [2048]u8 = undefined;
    let to_abs_buf: [2048]u8 = undefined;
    if (from_abs != to_abs) {
        if (!from_abs) {
            let parts: [1][]const u8 = [ n_from ];
            n_from = resolveWindows(parts[0..1], &from_abs_buf);
            from_vol = windowsVolumeName(n_from);
        }
        if (!to_abs) {
            let parts: [1][]const u8 = [ n_to ];
            n_to = resolveWindows(parts[0..1], &to_abs_buf);
            to_vol = windowsVolumeName(n_to);
        }
        if (from_vol.len > 0 and to_vol.len > 0 and !std.ascii.eqlIgnoreCase(from_vol, to_vol)) {
            let copy_len = if (n_to.len <= buf.len) n_to.len else buf.len;
            @memcpy(buf[0 .. copy_len], n_to[0 .. copy_len]);
            return buf[0 .. copy_len];
        }
    }

    let from_segs: [256][]const u8 = undefined;
    let from_count: usize = 0;
    if (!std.mem.eql(u8, n_from, ".")) {
        let cursor: usize = from_vol.len;
        while (cursor < n_from.len) {
            while (cursor < n_from.len and isSeparatorWindows(n_from[cursor])) cursor += 1;
            let start = cursor;
            while (cursor < n_from.len and !isSeparatorWindows(n_from[cursor])) cursor += 1;
            if (cursor > start and from_count < 256) {
                from_segs[from_count] = n_from[start .. cursor];
                from_count += 1;
            }
        }
    }

    let to_segs: [256][]const u8 = undefined;
    let to_count: usize = 0;
    if (!std.mem.eql(u8, n_to, ".")) {
        let cursor: usize = to_vol.len;
        while (cursor < n_to.len) {
            while (cursor < n_to.len and isSeparatorWindows(n_to[cursor])) cursor += 1;
            let start = cursor;
            while (cursor < n_to.len and !isSeparatorWindows(n_to[cursor])) cursor += 1;
            if (cursor > start and to_count < 256) {
                to_segs[to_count] = n_to[start .. cursor];
                to_count += 1;
            }
        }
    }

    let common: usize = 0;
    while (common < from_count and common < to_count) {
        if (!std.ascii.eqlIgnoreCase(from_segs[common], to_segs[common])) {
            break;
        }
        common += 1;
    }

    let out_len: usize = 0;
    let up = common;
    while (up < from_count) {
        if (out_len > 0 and out_len < buf.len) {
            buf[out_len] = '\\';
            out_len += 1;
        }
        if (out_len + 2 <= buf.len) {
            buf[out_len] = '.';
            buf[out_len + 1] = '.';
            out_len += 2;
        }
        up += 1;
    }

    let down = common;
    while (down < to_count) {
        if (out_len > 0 and out_len < buf.len) {
            buf[out_len] = '\\';
            out_len += 1;
        }
        let seg = to_segs[down];
        let copy_len = if (out_len + seg.len <= buf.len) seg.len else (buf.len - out_len);
        @memcpy(buf[out_len .. out_len + copy_len], seg[0 .. copy_len]);
        out_len += copy_len;
        down += 1;
    }

    return buf[0 .. out_len];
}

pub fn toNamespacedPathPosix(filepath: []const u8, buf: []u8): []const u8 {
    if (filepath.len == 0) return "";
    let copy_len = if (filepath.len <= buf.len) filepath.len else buf.len;
    @memcpy(buf[0 .. copy_len], filepath[0 .. copy_len]);
    return buf[0 .. copy_len];
}

pub fn toNamespacedPathWindows(filepath: []const u8, buf: []u8): []const u8 {
    if (filepath.len == 0) return "";

    let res_buf: [4096]u8 = undefined;
    let parts: [1][]const u8 = [ filepath ];
    let resolved = resolveWindows(parts[0..1], &res_buf);
    if (resolved.len == 0) return "";

    // Already in \\?\ or \\.\ device namespace
    if (resolved.len >= 4 and
        resolved[0] == '\\' and resolved[1] == '\\' and
        (resolved[2] == '?' or resolved[2] == '.') and resolved[3] == '\\')
    {
        let copy_len = if (resolved.len <= buf.len) resolved.len else buf.len;
        @memcpy(buf[0 .. copy_len], resolved[0 .. copy_len]);
        return buf[0 .. copy_len];
    }

    // UNC path: \\server\share\... -> \\?\UNC\server\share\...
    if (resolved.len >= 3 and resolved[0] == '\\' and resolved[1] == '\\') {
        let prefix = "\\\\?\\UNC\\";
        let rest = resolved[2 ..];
        let out_len: usize = 0;
        let p_len = if (prefix.len <= buf.len) prefix.len else buf.len;
        @memcpy(buf[0 .. p_len], prefix[0 .. p_len]);
        out_len += p_len;
        let r_len = if (out_len + rest.len <= buf.len) rest.len else (buf.len - out_len);
        @memcpy(buf[out_len .. out_len + r_len], rest[0 .. r_len]);
        out_len += r_len;
        return buf[0 .. out_len];
    }

    // Drive path: C:\... -> \\?\C:\...
    if (resolved.len >= 3 and checkIsDriveLetter(resolved[0]) and resolved[1] == ':' and resolved[2] == '\\') {
        let prefix = "\\\\?\\";
        let out_len: usize = 0;
        let p_len = if (prefix.len <= buf.len) prefix.len else buf.len;
        @memcpy(buf[0 .. p_len], prefix[0 .. p_len]);
        out_len += p_len;
        let r_len = if (out_len + resolved.len <= buf.len) resolved.len else (buf.len - out_len);
        @memcpy(buf[out_len .. out_len + r_len], resolved[0 .. r_len]);
        out_len += r_len;
        return buf[0 .. out_len];
    }

    let copy_len = if (resolved.len <= buf.len) resolved.len else buf.len;
    @memcpy(buf[0 .. copy_len], resolved[0 .. copy_len]);
    return buf[0 .. copy_len];
}

pub fn longestCommonPathPosix(paths: []const []const u8): []const u8 {
    if (paths.len == 0) return "";
    if (paths.len == 1) return paths[0];

    let min_len = paths[0].len;
    for (p of paths) {
        if (p.len < min_len) min_len = p.len;
    }

    let last_common_sep: usize = 0;
    let has_sep: bool = false;
    let idx: usize = 0;
    while (idx < min_len) {
        let c = paths[0][idx];
        let all_match = true;
        for (p of paths) {
            if (p[idx] != c) {
                all_match = false;
                break;
            }
        }
        if (!all_match) break;
        if (c == '/') {
            last_common_sep = idx;
            has_sep = true;
        }
        idx += 1;
    }

    if (!has_sep) {
        if (paths[0].len > 0 and paths[0][0] == '/') return "/";
        return "";
    }
    if (last_common_sep == 0) return "/";
    return paths[0][0 .. last_common_sep];
}

pub fn longestCommonPathWindows(paths: []const []const u8): []const u8 {
    if (paths.len == 0) return "";
    if (paths.len == 1) return paths[0];

    let first_vol = windowsVolumeName(paths[0]);
    for (p of paths) {
        let v = windowsVolumeName(p);
        if (!std.ascii.eqlIgnoreCase(first_vol, v)) {
            return "";
        }
    }

    let min_len = paths[0].len;
    for (p of paths) {
        if (p.len < min_len) min_len = p.len;
    }

    let last_common_sep: usize = 0;
    let has_sep: bool = false;
    let idx: usize = first_vol.len;
    while (idx < min_len) {
        let c = std.ascii.toLower(paths[0][idx]);
        let all_match = true;
        for (p of paths) {
            let ch = std.ascii.toLower(p[idx]);
            if (isSeparatorWindows(c) and isSeparatorWindows(ch)) {
                // both are separators
            } else if (c != ch) {
                all_match = false;
                break;
            }
        }
        if (!all_match) break;
        if (isSeparatorWindows(c)) {
            last_common_sep = idx;
            has_sep = true;
        }
        idx += 1;
    }

    if (!has_sep) {
        if (first_vol.len > 0) return paths[0][0 .. first_vol.len];
        return "";
    }
    if (last_common_sep == first_vol.len and first_vol.len > 0) {
        return paths[0][0 .. first_vol.len + 1];
    }
    return paths[0][0 .. last_common_sep];
}
