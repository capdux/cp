// Source: Absorbed from /data/dev/bun/src/resolver/fs.cp | Synced: 2026-11-22T12:49:17Z
// Ported from Bun: /data/dev/bun/src/resolver/fs.cp & /data/dev/bun/docs/runtime/module-resolution.mdx
// Synced: 2026-11-22
// Pure-CP zero-allocation Module Resolver Path Decomposition & Package Specifier Engine:
// - ResolverPathName (aligned with Bun Fs.PathName): zero-copy {dir, base, ext, filename} decomposition
//   with POSIX/Windows drive-letter support, trailing-slash folding, index-dir naming & JS identifier formatting
// - NodeJSPathName: Node-compatible {dir, base, ext, filename} decomposition
// - PackageSpecifier: zero-copy @scope/name/subpath parsing & bare/relative specifier classification
// - stripNodeModulesPrefix: extracts package-relative path after the deepest /node_modules/ segment.

/// 路径分隔符平台策略
pub enum PathPlatform: u8 {
    Posix = 0,
    Windows = 1,
    Any = 2,
}

/// 解析器路径与包名错误集
pub err ResolverPathError {
    BufferTooSmall,
    InvalidSpecifier,
}

fn isSepFor(c: u8, platform: PathPlatform): bool {
    if (platform == .Posix) return c == '/';
    return c == '/' || c == '\\';
}

fn lastIndexOfSepFor(s: []const u8, platform: PathPlatform): usize? {
    let i: usize = s.len;
    while (i > 0) {
        i -= 1;
        if (isSepFor(s[i], platform)) return i;
    }
    return null;
}

fn lastIndexOfDot(s: []const u8): usize? {
    let i: usize = s.len;
    while (i > 0) {
        i -= 1;
        if (s[i] == '.') return i;
    }
    return null;
}

fn isAsciiAlpha(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

fn isAsciiDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn isJsIdentChar(c: u8): bool {
    return isAsciiAlpha(c) || isAsciiDigit(c) || c == '_' || c == '$';
}

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

/// 零拷贝解析器路径视图（严格对齐 Bun `Fs.PathName`）
///
/// 示例：`/bar/foo/app.tsx` -> `dir="/bar/foo"`, `filename="app.tsx"`, `base="app"`, `ext=".tsx"`
pub struct ResolverPathName {
    /// 目录部分（不含尾斜杠；无目录时为空字符串 `""`）
    dir: []const u8 = "",
    /// 不含扩展名的文件主干名（如 `"app"`；点文件 `".gitignore"` 保留原样）
    base: []const u8 = "",
    /// 含前导 `.` 的扩展名（如 `".tsx"`；无扩展名或点文件为空字符串 `""`）
    ext: []const u8 = "",
    /// 含扩展名的完整文件名（如 `"app.tsx"`）
    filename: []const u8 = "",

    /// 提取路径的扩展名（含前导 `.`；点文件如 `.env` 返回 `""`）
    static fn findExtname(raw_path: []const u8): []const u8 {
        let start: usize = 0;
        if (i := lastIndexOfSepFor(raw_path, .Any)) {
            start = i + 1;
        }
        const fname = raw_path[start..raw_path.len];
        if (dot := lastIndexOfDot(fname)) {
            if (dot > 0) return fname[dot..fname.len];
        }
        return "";
    }

    /// 零拷贝拆解 POSIX / Windows 路径（对齐 Bun `Fs.PathName.init`）
    static fn init(raw_path: []const u8): ResolverPathName {
        let path = raw_path;
        let has_drive = false;
        let drive_prefix: []const u8 = "";

        // 处理 Windows 盘符前缀（如 C:\foo\bar.ts 或 C:/foo/bar.ts）
        if (path.len > 2 && path[1] == ':' && isAsciiAlpha(path[0]) && isSepFor(path[2], .Any)) {
            has_drive = true;
            drive_prefix = path[0..2];
            path = path[2..path.len];
        }

        // 剥离尾部冗余分隔符（保留单根 `/`）
        while (path.len > 1 && isSepFor(path[path.len - 1], .Any)) {
            path = path[0..path.len - 1];
        }

        let dir: []const u8 = "";
        let filename: []const u8 = path;
        if (sep := lastIndexOfSepFor(path, .Any)) {
            if (sep == 0) {
                if (path.len == 1) {
                    dir = if (has_drive) raw_path[0..3] else path;
                    filename = "";
                } else {
                    dir = if (has_drive) raw_path[0..3] else path[0..1];
                    filename = path[1..path.len];
                }
            } else {
                dir = if (has_drive) raw_path[0..sep + 2] else path[0..sep];
                filename = path[sep + 1..path.len];
            }
        } else if (has_drive) {
            dir = drive_prefix;
            filename = path;
        }

        let base = filename;
        let ext: []const u8 = "";
        if (filename.len > 1) {
            if (dot := lastIndexOfDot(filename)) {
                if (dot > 0) {
                    base = filename[0..dot];
                    ext = filename[dot..filename.len];
                }
            }
        }

        return ResolverPathName{
            dir: dir,
            base: base,
            ext: ext,
            filename: filename,
        };
    }

    /// 返回不含前导 `.` 的扩展名（如 `"tsx"`）
    fn extWithoutLeadingDot(): []const u8 {
        if (_.ext.len > 0 && _.ext[0] == '.') {
            return _.ext[1.._.ext.len];
        }
        return _.ext;
    }

    /// 返回目录部分；若为空目录则回退为 `"."`
    fn dirOrDot(): []const u8 {
        if (_.dir.len == 0) return ".";
        return _.dir;
    }

    /// 生成可读模块基名（对齐 Bun `nonUniqueNameStringBase`：`/bar/foo/index.js` -> `"foo"`，`/bar/foo.js` -> `"foo"`）
    fn nonUniqueNameStringBase(): []const u8 {
        if (_.dir.len > 0 && strEq(_.base, "index")) {
            const parent = ResolverPathName.init(_.dir);
            if (parent.base.len > 0) return parent.base;
        }
        return _.base;
    }

    /// 零堆分配将 `nonUniqueNameStringBase()` 清洗为合法 JS 标识符（如 `my-pkg.v2` -> `my_pkg_v2`）
    fn formatValidIdentifier(out: []u8): usize! {
        const raw = _.nonUniqueNameStringBase();
        if (raw.len == 0) {
            if (out.len < 1) return err.BufferTooSmall;
            out[0] = '_';
            return 1;
        }

        let pos: usize = 0;
        // 首字符为数字时前补 '_' 满足 JS 标识符首字符要求
        if (isAsciiDigit(raw[0])) {
            if (pos >= out.len) return err.BufferTooSmall;
            out[pos] = '_';
            pos += 1;
        }

        let i: usize = 0;
        while (i < raw.len) {
            if (pos >= out.len) return err.BufferTooSmall;
            const ch = raw[i];
            if (isJsIdentChar(ch)) {
                out[pos] = ch;
            } else {
                out[pos] = '_';
            }
            pos += 1;
            i += 1;
        }
        return pos;
    }
}

/// Node.js 兼容路径拆解视图（对齐 Bun `NodeJSPathName`：`base` 含扩展名，`filename` 不含扩展名）
pub struct NodeJSPathName {
    dir: []const u8 = "",
    base: []const u8 = "",
    ext: []const u8 = "",
    filename: []const u8 = "",

    static fn init(raw_path: []const u8, platform: PathPlatform): NodeJSPathName {
        let path = raw_path;
        let base = path;
        let ext: []const u8 = "";
        let dir = path;
        let is_absolute = true;
        let first = true;

        while (true) {
            const maybe_i = lastIndexOfSepFor(path, platform);
            if (i := maybe_i) {
                if (i + 1 != path.len && path.len >= i + 1) {
                    base = path[i + 1..path.len];
                    dir = path[0..i];
                    is_absolute = false;
                    break;
                }
                if (i == 0 && first) {
                    base = path[1..path.len];
                    dir = "";
                    break;
                }
                first = false;
                path = path[0..i];
            } else {
                break;
            }
        }

        if (base.len > 1 && isSepFor(base[base.len - 1], platform)) {
            base = base[0..base.len - 1];
        }

        let filename = base;
        if (filename.len > 1) {
            if (dot := lastIndexOfDot(filename)) {
                if (dot > 0) {
                    filename = filename[0..dot];
                    ext = base[dot..base.len];
                }
            }
        }

        if (is_absolute) {
            dir = "";
        }

        return NodeJSPathName{
            dir: dir,
            base: base,
            ext: ext,
            filename: filename,
        };
    }
}

/// 零拷贝 npm / ESM 裸包名拆解结果（如 `@scope/pkg/sub/a.js`）
pub struct PackageSpecifier {
    scope: []const u8 = "",
    name: []const u8 = "",
    full_name: []const u8 = "",
    subpath: []const u8 = "",
    is_scoped: bool = false,

    static fn parse(spec: []const u8): PackageSpecifier! {
        if (!isBarePackageSpecifier(spec)) {
            return err.InvalidSpecifier;
        }

        if (spec[0] == '@') {
            // 作用域包必须形如 @scope/pkg 或 @scope/pkg/subpath
            let first_slash: usize = 0;
            let found_first = false;
            let i: usize = 1;
            while (i < spec.len) {
                if (spec[i] == '/') {
                    first_slash = i;
                    found_first = true;
                    break;
                }
                i += 1;
            }
            if (!found_first || first_slash <= 1 || first_slash + 1 >= spec.len) {
                return err.InvalidSpecifier;
            }

            let second_slash: usize = spec.len;
            let j: usize = first_slash + 1;
            while (j < spec.len) {
                if (spec[j] == '/') {
                    second_slash = j;
                    break;
                }
                j += 1;
            }
            if (second_slash == first_slash + 1) {
                return err.InvalidSpecifier;
            }

            let sub: []const u8 = "";
            if (second_slash < spec.len) {
                sub = spec[second_slash + 1..spec.len];
            }

            return PackageSpecifier{
                scope: spec[0..first_slash],
                name: spec[first_slash + 1..second_slash],
                full_name: spec[0..second_slash],
                subpath: sub,
                is_scoped: true,
            };
        }

        // 普通裸包：pkg 或 pkg/subpath
        let slash: usize = spec.len;
        let k: usize = 0;
        while (k < spec.len) {
            if (spec[k] == '/') {
                slash = k;
                break;
            }
            k += 1;
        }
        if (slash == 0) return err.InvalidSpecifier;

        let sub: []const u8 = "";
        if (slash < spec.len) {
            sub = spec[slash + 1..spec.len];
        }

        return PackageSpecifier{
            scope: "",
            name: spec[0..slash],
            full_name: spec[0..slash],
            subpath: sub,
            is_scoped: false,
        };
    }
}

/// 判断模块说明符是否为相对路径（`.`、`..`、`./...`、`../...`）
pub fn isRelativeModuleSpecifier(spec: []const u8): bool {
    if (spec.len == 0) return false;
    if (spec[0] != '.') return false;
    if (spec.len == 1) return true;
    if (spec[1] == '/' || spec[1] == '\\') return true;
    if (spec[1] == '.') {
        if (spec.len == 2) return true;
        if (spec[2] == '/' || spec[2] == '\\') return true;
    }
    return false;
}

/// 判断模块说明符是否为裸包名（排除空串、相对路径、绝对路径、Windows 盘符与 `scheme:` URI）
pub fn isBarePackageSpecifier(spec: []const u8): bool {
    if (spec.len == 0) return false;
    if (isRelativeModuleSpecifier(spec)) return false;
    if (spec[0] == '/' || spec[0] == '\\' || spec[0] == '#') return false;
    // 排除 Windows 盘符路径（如 C:\...）
    if (spec.len >= 2 && spec[1] == ':' && isAsciiAlpha(spec[0])) return false;
    // 排除带协议前缀的说明符（如 node:fs, bun:sqlite, cap:io, https://..., data:...）
    let i: usize = 0;
    while (i < spec.len) {
        const ch = spec[i];
        if (ch == '/') break;
        if (ch == ':') return false;
        i += 1;
    }
    return true;
}

/// 提取路径中最深一层 `/node_modules/` 之后的包相对路径（无 `node_modules` 段返回 `null`）
pub fn stripNodeModulesPrefix(path: []const u8): ([]const u8)? {
    const marker = "node_modules";
    if (path.len <= marker.len) return null;

    let best: usize? = null;
    let i: usize = 0;
    while (i + marker.len < path.len) {
        const left_ok = (i == 0) || isSepFor(path[i - 1], .Any);
        const right_ok = isSepFor(path[i + marker.len], .Any);
        if (left_ok && right_ok && strEq(path[i..i + marker.len], marker)) {
            best = i + marker.len + 1;
        }
        i += 1;
    }

    if (pos := best) {
        if (pos < path.len) return path[pos..path.len];
    }
    return null;
}
