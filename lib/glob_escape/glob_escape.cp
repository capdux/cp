// Source: Absorbed from /data/dev/bun/src/glob.zig | Synced: 2026-11-21T14:40:55Z (Glob Pattern Escaping & Prefix Analyzer)

/// 判断字节是否为 Glob 通配符元字符（`*`, `?`, `[`, `]`, `{`, `}`, `(`, `)`, `!`, `\`）
pub fn isGlobMagic(c: u8): bool {
    return c == '*' || c == '?' || c == '[' || c == ']' ||
        c == '{' || c == '}' || c == '(' || c == ')' ||
        c == '!' || c == '\\';
}

fn isUnescapedMagicChar(c: u8): bool {
    return c == '*' || c == '?' || c == '[' || c == ']' ||
        c == '{' || c == '}' || c == '(' || c == ')' || c == '!';
}

/// 检测模式中是否存在未转义的 Glob 通配符元字符
pub fn hasGlobMagic(pattern: []const u8): bool {
    let i: usize = 0;
    while (i < pattern.len) {
        if (pattern[i] == '\\') {
            if (i + 1 < pattern.len) {
                i += 2;
            } else {
                i += 1;
            }
            continue;
        }
        if (isUnescapedMagicChar(pattern[i])) {
            return true;
        }
        i += 1;
    }
    return false;
}

/// 将字面量路径中的所有 Glob 元字符前加 `\` 转义写入 `out`，缓冲区不足返回 `null`
pub fn escapeGlob(literal: []const u8, out: []u8): ([]const u8)? {
    let wpos: usize = 0;
    let i: usize = 0;
    while (i < literal.len) {
        const c = literal[i];
        if (isGlobMagic(c)) {
            if (wpos + 2 > out.len) { return null; }
            out[wpos] = '\\';
            out[wpos + 1] = c;
            wpos += 2;
        } else {
            if (wpos >= out.len) { return null; }
            out[wpos] = c;
            wpos += 1;
        }
        i += 1;
    }
    return out[0..wpos];
}

/// 剥离反斜杠转义还原字面量写入 `out`，缓冲区不足返回 `null`
pub fn unescapeGlob(escaped: []const u8, out: []u8): ([]const u8)? {
    let wpos: usize = 0;
    let i: usize = 0;
    while (i < escaped.len) {
        if (escaped[i] == '\\' && i + 1 < escaped.len) {
            if (wpos >= out.len) { return null; }
            out[wpos] = escaped[i + 1];
            wpos += 1;
            i += 2;
        } else {
            if (wpos >= out.len) { return null; }
            out[wpos] = escaped[i];
            wpos += 1;
            i += 1;
        }
    }
    return out[0..wpos];
}

/// 提取第一个含通配符路径段之前的静态目录前缀，用于文件系统遍历剪枝
/// 例：`"src/http/*.cp"` -> `"src/http"`、`"*.cp"` -> `""`、`"a/b/c.cp"` -> `"a/b/c.cp"`
pub fn staticBaseDir(pattern: []const u8): []const u8 {
    let last_slash: usize? = null;
    let i: usize = 0;
    while (i < pattern.len) {
        const c = pattern[i];
        if (c == '\\') {
            if (i + 1 < pattern.len) {
                i += 2;
            } else {
                i += 1;
            }
            continue;
        }
        if (c == '/') {
            last_slash = i;
            i += 1;
            continue;
        }
        if (isUnescapedMagicChar(c)) {
            if (ls := last_slash) {
                if (ls == 0) {
                    return pattern[0..1];
                }
                return pattern[0..ls];
            }
            return "";
        }
        i += 1;
    }
    return pattern;
}

/// 判断模式是否带 `.gitignore` / Glob 否定前缀 `!`（`\!` 视为转义字面量而非否定）
pub fn isNegatedPattern(pattern: []const u8): bool {
    return pattern.len > 0 && pattern[0] == '!';
}

/// 剥离前导否定符 `!`（若非否定模式则原样返回）
pub fn stripNegation(pattern: []const u8): []const u8 {
    if (isNegatedPattern(pattern)) {
        return pattern[1..pattern.len];
    }
    return pattern;
}
