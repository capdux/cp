// Source: Absorbed from /data/dev/bun/src/install/PackageManager.zig | Synced: 2026-11-22T08:20:58Z (npm Package Name & Module Specifier Parser)

/// 解析后的 npm / Node.js 模块说明符（零拷贝切片）
pub struct PackageSpecifier {
    scope: ([]const u8)? = null,
    name: []const u8 = "",
    full_name: []const u8 = "",
    subpath: ([]const u8)? = null,
}

fn eqlBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

fn isAlpha(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

fn isSchemeChar(c: u8): bool {
    return isAlpha(c) || (c >= '0' && c <= '9') || c == '+' || c == '-' || c == '.';
}

fn hasProtocolScheme(spec: []const u8): bool {
    if (spec.len < 2 || !isAlpha(spec[0])) {
        return false;
    }
    let i: usize = 1;
    while (i < spec.len) {
        const c = spec[i];
        if (c == ':') {
            return true;
        }
        if (!isSchemeChar(c)) {
            return false;
        }
        i += 1;
    }
    return false;
}

fn hasEmptySegment(sub: []const u8): bool {
    if (sub.len == 0 || sub[0] == '/' || sub[sub.len - 1] == '/') {
        return true;
    }
    let i: usize = 0;
    while (i + 1 < sub.len) {
        if (sub[i] == '/' && sub[i + 1] == '/') {
            return true;
        }
        i += 1;
    }
    return false;
}

fn isValidNpmSegmentChar(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.';
}

fn isValidNpmSegment(seg: []const u8): bool {
    if (seg.len == 0) { return false; }
    if (seg[0] == '.' || seg[0] == '_') { return false; }
    let i: usize = 0;
    while (i < seg.len) {
        if (!isValidNpmSegmentChar(seg[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// 判断模块说明符是否为相对路径（`"."`、`".."`、`"./..."`、`"../..."`）
pub fn isRelativeSpecifier(spec: []const u8): bool {
    if (spec.len == 0 || spec[0] != '.') {
        return false;
    }
    if (spec.len == 1) { return true; }
    if (spec[1] == '/') { return true; }
    if (spec[1] == '.') {
        return spec.len == 2 || spec[2] == '/';
    }
    return false;
}

/// 判断是否为裸模块说明符（非空、非相对路径、非绝对路径、非 `node:`/`bun:`/`cap:` 等协议前缀）
pub fn isBareSpecifier(spec: []const u8): bool {
    if (spec.len == 0) { return false; }
    if (isRelativeSpecifier(spec)) { return false; }
    if (spec[0] == '/' || spec[0] == '\\') { return false; }
    if (hasProtocolScheme(spec)) { return false; }
    return true;
}

/// 按 `validate-npm-package-name` 规范校验 npm 包名合法性：
/// - 总长 1..=214、全小写无大写、无首尾空白
/// - 不以 `.` 或 `_` 开头、不含 `~'!*()` 等非 URL 安全字符
/// - 禁止黑名单名 `node_modules` 与 `favicon.ico`
pub fn isValidPackageName(pkg_name: []const u8): bool {
    if (pkg_name.len == 0 || pkg_name.len > 214) {
        return false;
    }
    if (eqlBytes(pkg_name, "node_modules") || eqlBytes(pkg_name, "favicon.ico")) {
        return false;
    }

    if (pkg_name[0] == '@') {
        let slash_idx: usize? = null;
        let i: usize = 1;
        while (i < pkg_name.len) {
            if (pkg_name[i] == '/') {
                if (slash_idx != null) {
                    return false; // 超过一个 '/'
                }
                slash_idx = i;
            }
            i += 1;
        }
        if (si := slash_idx) {
            const scope_body = pkg_name[1..si];
            const name_part = pkg_name[(si + 1)..pkg_name.len];
            return isValidNpmSegment(scope_body) && isValidNpmSegment(name_part);
        }
        return false;
    }

    return isValidNpmSegment(pkg_name);
}

/// 解析 npm / Node.js 模块说明符为 `scope`、`name`、`full_name` 与可选 `subpath`
/// 相对路径、绝对路径、协议前缀、裸 `@scope` 或空路径段均返回 `null`
pub fn parsePackageSpecifier(spec: []const u8): PackageSpecifier? {
    if (!isBareSpecifier(spec)) {
        return null;
    }

    if (spec[0] == '@') {
        let first_slash: usize? = null;
        let i: usize = 1;
        while (i < spec.len) {
            if (spec[i] == '/') {
                first_slash = i;
                break;
            }
            i += 1;
        }
        if (s1 := first_slash) {
            if (s1 <= 1 || s1 + 1 >= spec.len) {
                return null;
            }
            const scope = spec[0..s1];
            let second_slash: usize? = null;
            let k = s1 + 1;
            while (k < spec.len) {
                if (spec[k] == '/') {
                    second_slash = k;
                    break;
                }
                k += 1;
            }
            if (s2 := second_slash) {
                const pkg = spec[(s1 + 1)..s2];
                const sub = spec[(s2 + 1)..spec.len];
                if (pkg.len == 0 || hasEmptySegment(sub)) {
                    return null;
                }
                return PackageSpecifier{
                    scope: scope,
                    name: pkg,
                    full_name: spec[0..s2],
                    subpath: sub,
                };
            }
            const pkg = spec[(s1 + 1)..spec.len];
            if (pkg.len == 0) {
                return null;
            }
            return PackageSpecifier{
                scope: scope,
                name: pkg,
                full_name: spec,
                subpath: null,
            };
        }
        return null;
    }

    // 非作用域包: pkg 或 pkg/a/b
    let slash_idx: usize? = null;
    let i: usize = 0;
    while (i < spec.len) {
        if (spec[i] == '/') {
            slash_idx = i;
            break;
        }
        i += 1;
    }
    if (si := slash_idx) {
        const pkg = spec[0..si];
        const sub = spec[(si + 1)..spec.len];
        if (pkg.len == 0 || hasEmptySegment(sub)) {
            return null;
        }
        return PackageSpecifier{
            scope: null,
            name: pkg,
            full_name: pkg,
            subpath: sub,
        };
    }

    return PackageSpecifier{
        scope: null,
        name: spec,
        full_name: spec,
        subpath: null,
    };
}
