// Source: /data/dev/bun/src/paths/resolve_path.cp
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

pub fn windowsVolumeNameLen(path: []const u8): WindowsVolumeInfo {
    if (path.len >= 2 and path[1] == ':' and checkIsDriveLetter(path[0])) {
        return WindowsVolumeInfo{ len: 2, unc_slash: 0 };
    }
    if (path.len >= 5 and isSeparatorWindows(path[0]) and isSeparatorWindows(path[1])) {
        // UNC path: \\server\share
        let i: usize = 2;
        let third_slash: usize = 0;
        while (i < path.len) {
            if (isSeparatorWindows(path[i])) {
                third_slash = i;
                break;
            }
            i += 1;
        }
        if (third_slash > 2 and third_slash + 1 < path.len) {
            let j = third_slash + 1;
            while (j < path.len) {
                if (isSeparatorWindows(path[j])) {
                    return WindowsVolumeInfo{ len: j, unc_slash: third_slash };
                }
                j += 1;
            }
            return WindowsVolumeInfo{ len: path.len, unc_slash: third_slash };
        }
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
    if (filepath.len == 0) {
        buf[0] = '.';
        return buf[0 .. 1];
    }

    const is_abs = isAbsolutePosix(filepath);
    let segments: [128][]const u8 = undefined;
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
            } else if (!is_abs and seg_count < 128) {
                segments[seg_count] = "..";
                seg_count += 1;
            }
        } else if (seg_count < 128) {
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
        if (s_i > 0 or is_abs) {
            if (out_len > 1 or (!is_abs and out_len > 0)) {
                buf[out_len] = '/';
                out_len += 1;
            }
        }
        let seg = segments[s_i];
        @memcpy(buf[out_len .. out_len + seg.len], seg);
        out_len += seg.len;
        s_i += 1;
    }

    return buf[0 .. out_len];
}

pub fn normalizeWindows(filepath: []const u8, buf: []u8): []const u8 {
    if (filepath.len == 0) {
        buf[0] = '.';
        return buf[0 .. 1];
    }

    let vol_info = windowsVolumeNameLen(filepath);
    let vol_len = vol_info.len;
    let vol = filepath[0 .. vol_len];
    let rest = filepath[vol_len ..];

    let is_abs = (vol_len > 0 and rest.len > 0 and isSeparatorWindows(rest[0])) or
                 (vol_len == 0 and rest.len > 0 and isSeparatorWindows(rest[0]));

    let segments: [128][]const u8 = undefined;
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
            } else if (!is_abs and seg_count < 128) {
                segments[seg_count] = "..";
                seg_count += 1;
            }
        } else if (seg_count < 128) {
            segments[seg_count] = seg;
            seg_count += 1;
        }
    }

    let out_len: usize = 0;
    if (vol_len > 0) {
        @memcpy(buf[out_len .. out_len + vol_len], vol);
        out_len += vol_len;
    }

    if (seg_count == 0) {
        if (is_abs) {
            buf[out_len] = '\\';
            out_len += 1;
            return buf[0 .. out_len];
        }
        if (vol_len > 0) {
            return buf[0 .. out_len];
        }
        buf[0] = '.';
        return buf[0 .. 1];
    }

    if (is_abs) {
        buf[out_len] = '\\';
        out_len += 1;
    }

    let s_i: usize = 0;
    while (s_i < seg_count) {
        if (s_i > 0 or is_abs) {
            if (out_len > vol_len + 1 or (!is_abs and out_len > vol_len)) {
                buf[out_len] = '\\';
                out_len += 1;
            }
        }
        let seg = segments[s_i];
        @memcpy(buf[out_len .. out_len + seg.len], seg);
        out_len += seg.len;
        s_i += 1;
    }

    return buf[0 .. out_len];
}

pub fn joinPosix(paths: []const []const u8, buf: []u8): []const u8 {
    let temp: [2048]u8 = undefined;
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
        buf[0] = '.';
        return buf[0 .. 1];
    }

    return normalizePosix(temp[0 .. t_len], buf);
}

pub fn joinWindows(paths: []const []const u8, buf: []u8): []const u8 {
    let temp: [2048]u8 = undefined;
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
        buf[0] = '.';
        return buf[0 .. 1];
    }

    return normalizeWindows(temp[0 .. t_len], buf);
}

pub fn resolvePosix(paths: []const []const u8, buf: []u8): []const u8 {
    let start_idx: usize = 0;
    let i = paths.len;
    while (i > 0) {
        i -= 1;
        if (isAbsolutePosix(paths[i])) {
            start_idx = i;
            break;
        }
    }

    let temp: [2048]u8 = undefined;
    let t_len: usize = 0;

    if (!isAbsolutePosix(paths[start_idx])) {
        temp[0] = '/';
        t_len = 1;
    }

    let curr = start_idx;
    while (curr < paths.len) {
        let p = paths[curr];
        if (p.len > 0) {
            if (t_len > 0 and temp[t_len - 1] != '/' and p[0] != '/') {
                temp[t_len] = '/';
                t_len += 1;
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
    let start_idx: usize = 0;
    let i = paths.len;
    while (i > 0) {
        i -= 1;
        if (isAbsoluteWindows(paths[i])) {
            start_idx = i;
            break;
        }
    }

    let temp: [2048]u8 = undefined;
    let t_len: usize = 0;

    if (!isAbsoluteWindows(paths[start_idx])) {
        @memcpy(temp[0 .. 3], "C:\\");
        t_len = 3;
    }

    let curr = start_idx;
    while (curr < paths.len) {
        let p = paths[curr];
        if (p.len > 0) {
            if (t_len > 0 and !isSeparatorWindows(temp[t_len - 1]) and !isSeparatorWindows(p[0])) {
                temp[t_len] = '\\';
                t_len += 1;
            }
            let copy_len = if (t_len + p.len <= temp.len) p.len else (temp.len - t_len);
            @memcpy(temp[t_len .. t_len + copy_len], p[0 .. copy_len]);
            t_len += copy_len;
        }
        curr += 1;
    }

    return normalizeWindows(temp[0 .. t_len], buf);
}

pub fn relativePosix(from_path: []const u8, to_path: []const u8, buf: []u8): []const u8 {
    let from_buf: [1024]u8 = undefined;
    let to_buf: [1024]u8 = undefined;
    let n_from = normalizePosix(from_path, &from_buf);
    let n_to = normalizePosix(to_path, &to_buf);

    if (std.mem.eql(u8, n_from, n_to)) {
        return "";
    }

    let from_segs: [64][]const u8 = undefined;
    let from_count: usize = 0;
    let cursor: usize = 0;
    while (cursor < n_from.len) {
        while (cursor < n_from.len and n_from[cursor] == '/') cursor += 1;
        let start = cursor;
        while (cursor < n_from.len and n_from[cursor] != '/') cursor += 1;
        if (cursor > start and from_count < 64) {
            from_segs[from_count] = n_from[start .. cursor];
            from_count += 1;
        }
    }

    let to_segs: [64][]const u8 = undefined;
    let to_count: usize = 0;
    cursor = 0;
    while (cursor < n_to.len) {
        while (cursor < n_to.len and n_to[cursor] == '/') cursor += 1;
        let start = cursor;
        while (cursor < n_to.len and n_to[cursor] != '/') cursor += 1;
        if (cursor > start and to_count < 64) {
            to_segs[to_count] = n_to[start .. cursor];
            to_count += 1;
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
        if (out_len > 0) {
            buf[out_len] = '/';
            out_len += 1;
        }
        buf[out_len] = '.';
        buf[out_len + 1] = '.';
        out_len += 2;
        up += 1;
    }

    let down = common;
    while (down < to_count) {
        if (out_len > 0) {
            buf[out_len] = '/';
            out_len += 1;
        }
        let seg = to_segs[down];
        @memcpy(buf[out_len .. out_len + seg.len], seg);
        out_len += seg.len;
        down += 1;
    }

    return buf[0 .. out_len];
}

pub fn relativeWindows(from_path: []const u8, to_path: []const u8, buf: []u8): []const u8 {
    let from_buf: [1024]u8 = undefined;
    let to_buf: [1024]u8 = undefined;
    let n_from = normalizeWindows(from_path, &from_buf);
    let n_to = normalizeWindows(to_path, &to_buf);

    if (std.ascii.eqlIgnoreCase(n_from, n_to)) {
        return "";
    }

    let from_vol = windowsVolumeName(n_from);
    let to_vol = windowsVolumeName(n_to);
    if (from_vol.len > 0 and to_vol.len > 0 and !std.ascii.eqlIgnoreCase(from_vol, to_vol)) {
        // Different drives, cannot compute relative path
        let copy_len = if (n_to.len <= buf.len) n_to.len else buf.len;
        @memcpy(buf[0 .. copy_len], n_to[0 .. copy_len]);
        return buf[0 .. copy_len];
    }

    let from_segs: [64][]const u8 = undefined;
    let from_count: usize = 0;
    let cursor: usize = from_vol.len;
    while (cursor < n_from.len) {
        while (cursor < n_from.len and isSeparatorWindows(n_from[cursor])) cursor += 1;
        let start = cursor;
        while (cursor < n_from.len and !isSeparatorWindows(n_from[cursor])) cursor += 1;
        if (cursor > start and from_count < 64) {
            from_segs[from_count] = n_from[start .. cursor];
            from_count += 1;
        }
    }

    let to_segs: [64][]const u8 = undefined;
    let to_count: usize = 0;
    cursor = to_vol.len;
    while (cursor < n_to.len) {
        while (cursor < n_to.len and isSeparatorWindows(n_to[cursor])) cursor += 1;
        let start = cursor;
        while (cursor < n_to.len and !isSeparatorWindows(n_to[cursor])) cursor += 1;
        if (cursor > start and to_count < 64) {
            to_segs[to_count] = n_to[start .. cursor];
            to_count += 1;
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
        if (out_len > 0) {
            buf[out_len] = '\\';
            out_len += 1;
        }
        buf[out_len] = '.';
        buf[out_len + 1] = '.';
        out_len += 2;
        up += 1;
    }

    let down = common;
    while (down < to_count) {
        if (out_len > 0) {
            buf[out_len] = '\\';
            out_len += 1;
        }
        let seg = to_segs[down];
        @memcpy(buf[out_len .. out_len + seg.len], seg);
        out_len += seg.len;
        down += 1;
    }

    return buf[0 .. out_len];
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
    return paths[0][0 .. last_common_sep];
}
