// Source: Absorbed from /data/dev/bun/src/shell/util.cp | Synced: 2026-11-21T17:04:34Z

/// 环境变量赋值拆分结果（`KEY=VALUE`）
pub struct EnvAssignment {
    key: []const u8,
    value: []const u8,
}

fn isSafeUnquotedChar(c: u8): bool {
    return (c >= 'a' && c <= 'z')
        || (c >= 'A' && c <= 'Z')
        || (c >= '0' && c <= '9')
        || c == '_'
        || c == '@'
        || c == '%'
        || c == '+'
        || c == '='
        || c == ':'
        || c == ','
        || c == '.'
        || c == '/'
        || c == '-';
}

/// 判断字符串是否仅包含 POSIX Shell 安全无引号字符（`[A-Za-z0-9_@%+=:,./-]+`），空串返回 `false`。
pub fn isSafeUnquoted(s: []const u8): bool {
    if (s.len == 0) {
        return false;
    }
    for (c of s) {
        if (!isSafeUnquotedChar(c)) {
            return false;
        }
    }
    return true;
}

/// POSIX 单引号安全转义（空串 -> `''`，安全串原样输出，含单引号用 `'\''` 拼接；缓冲区不足返回 `null`）。
pub fn quotePosix(input: []const u8, out: []u8): ([]const u8)? {
    if (input.len == 0) {
        if (out.len < 2) { return null; }
        out[0] = '\'';
        out[1] = '\'';
        return out[0..2];
    }
    if (isSafeUnquoted(input)) {
        if (out.len < input.len) { return null; }
        @memcpy(out[0..input.len], input);
        return out[0..input.len];
    }
    if (out.len < 2) { return null; }
    let pos: usize = 0;
    out[pos] = '\'';
    pos += 1;
    for (c of input) {
        if (c == '\'') {
            if (pos + 4 > out.len) { return null; }
            out[pos] = '\'';
            out[pos + 1] = '\\';
            out[pos + 2] = '\'';
            out[pos + 3] = '\'';
            pos += 4;
        } else {
            if (pos + 1 > out.len) { return null; }
            out[pos] = c;
            pos += 1;
        }
    }
    if (pos + 1 > out.len) { return null; }
    out[pos] = '\'';
    pos += 1;
    return out[0..pos];
}

/// 转义 POSIX 双引号内部特殊字符（`$`、`` ` ``、`"`、`\`），缓冲区不足返回 `null`。
pub fn escapeDoubleQuoted(input: []const u8, out: []u8): ([]const u8)? {
    let pos: usize = 0;
    for (c of input) {
        if (c == '$' || c == '`' || c == '"' || c == '\\') {
            if (pos + 2 > out.len) { return null; }
            out[pos] = '\\';
            out[pos + 1] = c;
            pos += 2;
        } else {
            if (pos + 1 > out.len) { return null; }
            out[pos] = c;
            pos += 1;
        }
    }
    return out[0..pos];
}

/// 安全拼接 POSIX Shell 命令行参数数组，缓冲区不足返回 `null`。
pub fn joinCommand(args: []const []const u8, out: []u8): ([]const u8)? {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < args.len) {
        if (i > 0) {
            if (pos + 1 > out.len) { return null; }
            out[pos] = ' ';
            pos += 1;
        }
        const quoted = quotePosix(args[i], out[pos..]) ?? return null;
        pos += quoted.len;
        i += 1;
    }
    return out[0..pos];
}

/// 严格遵循 POSIX `basename(1)` 语义提取路径基名（支持可选后缀剥离；若后缀等于整个基名则不剥离）。
pub fn posixBasename(path: []const u8, suffix: []const u8): []const u8 {
    if (path.len == 0) {
        return ".";
    }
    let end = path.len;
    while (end > 0 && path[end - 1] == '/') {
        end -= 1;
    }
    if (end == 0) {
        return "/";
    }
    let start: usize = end;
    while (start > 0 && path[start - 1] != '/') {
        start -= 1;
    }
    const base = path[start..end];
    if (suffix.len > 0 && suffix.len < base.len) {
        if (std.mem.eql(u8, base[base.len - suffix.len ..], suffix)) {
            return base[0 .. base.len - suffix.len];
        }
    }
    return base;
}

/// 严格遵循 POSIX `dirname(1)` 语义提取目录名（空串/无 `/` 返回 `"."`，根路径返回 `"/"`，自动剥离多余尾 `/`）。
pub fn posixDirname(path: []const u8): []const u8 {
    if (path.len == 0) {
        return ".";
    }
    let end = path.len;
    while (end > 0 && path[end - 1] == '/') {
        end -= 1;
    }
    if (end == 0) {
        return "/";
    }
    let slash_pos = end;
    while (slash_pos > 0 && path[slash_pos - 1] != '/') {
        slash_pos -= 1;
    }
    if (slash_pos == 0) {
        return ".";
    }
    while (slash_pos > 0 && path[slash_pos - 1] == '/') {
        slash_pos -= 1;
    }
    if (slash_pos == 0) {
        return "/";
    }
    return path[0..slash_pos];
}

/// 校验 POSIX 环境变量名（`[A-Za-z_][A-Za-z0-9_]*`）
pub fn isValidEnvVarName(name: []const u8): bool {
    if (name.len == 0) {
        return false;
    }
    const c0 = name[0];
    if (!((c0 >= 'a' && c0 <= 'z') || (c0 >= 'A' && c0 <= 'Z') || c0 == '_')) {
        return false;
    }
    let i: usize = 1;
    while (i < name.len) {
        const c = name[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_')) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// 解析 `KEY=VALUE` 环境变量赋值（要求 `KEY` 为合法 POSIX 环境变量名，非法返回 `null`）。
pub fn splitEnvAssignment(s: []const u8): EnvAssignment? {
    const eq = std.mem.indexOfScalar(u8, s, '=') ?? return null;
    const key = s[0..eq];
    if (!isValidEnvVarName(key)) {
        return null;
    }
    return EnvAssignment{
        key: key,
        value: s[eq + 1 ..],
    };
}
