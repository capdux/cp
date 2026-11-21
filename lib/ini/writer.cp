// Source: Absorbed from /data/dev/bun/src/ini.zig | Synced: 2026-11-21T15:56:07Z (INI Config Safe Writer & Escaper)

fn isHWhitespace(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn writeByte(out: []u8, pos: *usize, b: u8): bool {
    if (pos.* >= out.len) {
        return false;
    }
    out[pos.*] = b;
    pos.* += 1;
    return true;
}

fn writeSlice(out: []u8, pos: *usize, s: []const u8): bool {
    if (pos.* > out.len || s.len > out.len - pos.*) {
        return false;
    }
    let i: usize = 0;
    while (i < s.len) {
        out[pos.* + i] = s[i];
        i += 1;
    }
    pos.* += s.len;
    return true;
}

fn writeQuotedEscaped(out: []u8, pos: *usize, s: []const u8): bool {
    if (!writeByte(out, pos, '"')) {
        return false;
    }
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c == '"') {
            if (!writeSlice(out, pos, "\\\"")) return false;
        } else if (c == '\\') {
            if (!writeSlice(out, pos, "\\\\")) return false;
        } else if (c == '\n') {
            if (!writeSlice(out, pos, "\\n")) return false;
        } else if (c == '\r') {
            if (!writeSlice(out, pos, "\\r")) return false;
        } else if (c == '\t') {
            if (!writeSlice(out, pos, "\\t")) return false;
        } else {
            if (!writeByte(out, pos, c)) return false;
        }
        i += 1;
    }
    return writeByte(out, pos, '"');
}

fn isValidIniSection(section: []const u8): bool {
    if (section.len == 0) {
        return false;
    }
    if (isHWhitespace(section[0]) || isHWhitespace(section[section.len - 1])) {
        return false;
    }
    let i: usize = 0;
    while (i < section.len) {
        const c = section[i];
        if (c == '[' || c == ']' || c == '\r' || c == '\n' || c == '"' || c == ';' || c == '#' || c == 0) {
            return false;
        }
        i += 1;
    }
    return true;
}

pub fn isValidIniKey(key: []const u8): bool {
    if (key.len == 0) {
        return false;
    }
    if (isHWhitespace(key[0]) || isHWhitespace(key[key.len - 1])) {
        return false;
    }
    let i: usize = 0;
    while (i < key.len) {
        const c = key[i];
        if (c == '=' || c == '\r' || c == '\n' || c == '[' || c == ']' || c == ';' || c == '#' || c == 0) {
            return false;
        }
        i += 1;
    }
    return true;
}

pub fn needsIniQuotes(val: []const u8): bool {
    if (val.len == 0) {
        return false;
    }
    if (isHWhitespace(val[0]) || isHWhitespace(val[val.len - 1])) {
        return true;
    }
    let i: usize = 0;
    while (i < val.len) {
        const c = val[i];
        if (c == ';' || c == '#' || c == '"' || c == '\\' || c == '\r' || c == '\n' || c == '\t') {
            return true;
        }
        i += 1;
    }
    return false;
}

pub fn appendIniSection(out: []u8, pos: *usize, section: []const u8): bool {
    if (!isValidIniSection(section)) {
        return false;
    }
    const saved = pos.*;
    if (!writeByte(out, pos, '[')) {
        pos.* = saved;
        return false;
    }
    if (!writeSlice(out, pos, section)) {
        pos.* = saved;
        return false;
    }
    if (!writeSlice(out, pos, "]\n")) {
        pos.* = saved;
        return false;
    }
    return true;
}

pub fn appendIniSubsection(out: []u8, pos: *usize, section: []const u8, subsection: []const u8): bool {
    if (!isValidIniSection(section)) {
        return false;
    }
    let k: usize = 0;
    while (k < subsection.len) {
        const c = subsection[k];
        if (c == '\r' || c == '\n' || c == 0) {
            return false;
        }
        k += 1;
    }

    const saved = pos.*;
    if (!writeByte(out, pos, '[')) {
        pos.* = saved;
        return false;
    }
    if (!writeSlice(out, pos, section)) {
        pos.* = saved;
        return false;
    }
    if (!writeSlice(out, pos, " \"")) {
        pos.* = saved;
        return false;
    }
    let i: usize = 0;
    while (i < subsection.len) {
        const c = subsection[i];
        if (c == '"' || c == '\\') {
            if (!writeByte(out, pos, '\\')) {
                pos.* = saved;
                return false;
            }
        }
        if (!writeByte(out, pos, c)) {
            pos.* = saved;
            return false;
        }
        i += 1;
    }
    if (!writeSlice(out, pos, "\"]\n")) {
        pos.* = saved;
        return false;
    }
    return true;
}

pub fn appendIniPair(out: []u8, pos: *usize, key: []const u8, val: []const u8): bool {
    if (!isValidIniKey(key)) {
        return false;
    }
    const saved = pos.*;
    if (!writeSlice(out, pos, key)) {
        pos.* = saved;
        return false;
    }
    if (!writeByte(out, pos, '=')) {
        pos.* = saved;
        return false;
    }
    if (needsIniQuotes(val)) {
        if (!writeQuotedEscaped(out, pos, val)) {
            pos.* = saved;
            return false;
        }
    } else {
        if (!writeSlice(out, pos, val)) {
            pos.* = saved;
            return false;
        }
    }
    if (!writeByte(out, pos, '\n')) {
        pos.* = saved;
        return false;
    }
    return true;
}

pub fn appendIniComment(out: []u8, pos: *usize, comment: []const u8): bool {
    const saved = pos.*;
    if (comment.len == 0) {
        if (!writeSlice(out, pos, "# \n")) {
            pos.* = saved;
            return false;
        }
        return true;
    }

    let i: usize = 0;
    while (i < comment.len) {
        if (!writeSlice(out, pos, "# ")) {
            pos.* = saved;
            return false;
        }
        const line_start = i;
        while (i < comment.len && comment[i] != '\n' && comment[i] != '\r') {
            i += 1;
        }
        if (!writeSlice(out, pos, comment[line_start..i])) {
            pos.* = saved;
            return false;
        }
        if (!writeByte(out, pos, '\n')) {
            pos.* = saved;
            return false;
        }
        if (i < comment.len) {
            if (comment[i] == '\r') {
                i += 1;
                if (i < comment.len && comment[i] == '\n') {
                    i += 1;
                }
            } else if (comment[i] == '\n') {
                i += 1;
            }
        }
    }
    return true;
}
