// Source: Absorbed from /data/dev/bun/src/install/lockfile/printer/Yarn.cp | Synced: 2026-11-22T02:27:30Z
// Pure-CP zero-allocation Yarn Lockfile v1 deterministic serializer with
// YAML-safe key quoting, multi-specifier deduplication, alphabetical package
// ordering, integrity emission, and dependency section grouping.

pub const MAX_YARN_LOCK_PACKAGES: usize = 64;

/// Yarn Lockfile v1 打印器错误集
pub err YarnPrintError {
    EmptyPackageName,
    EmptyVersion,
    CapacityExceeded,
    BufferTooSmall,
}

/// 依赖分类（用于分组输出 `dependencies:` / `optionalDependencies:` / `devDependencies:`）
pub enum YarnLockDepKind: u8 {
    prod = 0,
    optional = 1,
    dev = 2,
}

/// 单条子依赖条目
pub struct YarnLockDep {
    name: string,
    spec: string,
    kind: YarnLockDepKind = .prod,
}

/// 单个已解析的包条目（用于生成 Yarn Lockfile v1 块）
pub struct YarnLockPackage {
    name: string,
    version: string,
    resolved_url: string,
    integrity: string = "",
    /// 指向同一解析包的多个请求版本范围（如 `["^4.17.0", "~4.17.20"]`；为空时默认输出 `^<version>`）
    requested_specs: []const string = &[],
    /// 子依赖列表（自动按 `prod` / `optional` / `dev` 分组输出）
    dependencies: []const YarnLockDep = &[],
}

fn startsWith(s: string, prefix: string): bool {
    if (s.len < prefix.len) {
        return false;
    }
    return std.mem.eql(u8, s[0..prefix.len], prefix);
}

fn lexLessThan(a: string, b: string): bool {
    const min_len = if (a.len < b.len) a.len else b.len;
    let i: usize = 0;
    while (i < min_len) {
        if (a[i] != b[i]) {
            return a[i] < b[i];
        }
        i += 1;
    }
    return a.len < b.len;
}

/// 判断 YAML / Yarn v1 键名是否需要双引号包裹（对齐 Yarn stringify & Bun `mustEscapeYAMLString`）
pub fn mustQuoteYarnName(name: string): bool {
    if (name.len == 0) {
        return true;
    }
    if (name[0] == '@' || name[0] == '-' || name[0] == '?' || name[0] == ':' || name[0] == '*' || name[0] == '&' || name[0] == '!' || name[0] == '%' || name[0] == '`' || name[0] == '"' || name[0] == '\'') {
        return true;
    }
    let i: usize = 0;
    while (i < name.len) {
        const c = name[i];
        if (c == ' ' || c == '\t' || c == ':' || c == '#' || c == ',' || c == '[' || c == ']' || c == '{' || c == '}' || c == '/' || c == '|' || c == '!' || c == '"' || c == '\'') {
            return true;
        }
        i += 1;
    }
    return false;
}

/// 判断 Yarn v1 头部 `<name>@<spec>` 键是否需要双引号包裹（对齐 Bun `Yarn.cp` 第 160 行）
pub fn mustQuoteYarnKey(name: string, version_spec: string): bool {
    if (mustQuoteYarnName(name)) {
        return true;
    }
    if (startsWith(version_spec, "npm:")) {
        return true;
    }
    let i: usize = 0;
    while (i < version_spec.len) {
        const c = version_spec[i];
        if (c == ' ' || c == '|' || c == '\t' || c == '-' || c == '/' || c == '!' || c == ':' || c == '^' || c == '~' || c == '>' || c == '<' || c == '=' || c == '*') {
            return true;
        }
        i += 1;
    }
    return false;
}

fn appendBytes(buf: []u8, pos: usize, s: string): usize! {
    if (pos + s.len > buf.len) {
        return err.BufferTooSmall;
    }
    @memcpy(buf[pos .. pos + s.len], s);
    return pos + s.len;
}

fn appendByte(buf: []u8, pos: usize, b: u8): usize! {
    if (pos >= buf.len) {
        return err.BufferTooSmall;
    }
    buf[pos] = b;
    return pos + 1;
}

fn writeDepSection(
    buf: []u8,
    pos_in: usize,
    header: string,
    deps: []const YarnLockDep,
    target_kind: YarnLockDepKind,
): usize! {
    let has_any = false;
    let i: usize = 0;
    while (i < deps.len) {
        if (deps[i].kind == target_kind) {
            has_any = true;
            break;
        }
        i += 1;
    }
    if (!has_any) {
        return pos_in;
    }

    let pos = appendBytes(buf, pos_in, header)?;
    i = 0;
    while (i < deps.len) {
        if (deps[i].kind == target_kind) {
            pos = appendBytes(buf, pos, "    ")?;
            const need_q = mustQuoteYarnName(deps[i].name);
            if (need_q) { pos = appendByte(buf, pos, '"')?; }
            pos = appendBytes(buf, pos, deps[i].name)?;
            if (need_q) { pos = appendByte(buf, pos, '"')?; }
            pos = appendBytes(buf, pos, " \"")?;
            pos = appendBytes(buf, pos, deps[i].spec)?;
            pos = appendBytes(buf, pos, "\"\n")?;
        }
        i += 1;
    }
    return pos;
}

/// 零堆分配 Yarn Lockfile v1 打印器（固定容量 `MAX_YARN_LOCK_PACKAGES = 64`）
pub struct YarnLockPrinter {
    packages: [MAX_YARN_LOCK_PACKAGES]YarnLockPackage = undefined,
    len: usize = 0,
    meta_hash: string = "",

    static fn init(meta_hash: string): YarnLockPrinter {
        return YarnLockPrinter{
            packages: undefined,
            len: 0,
            meta_hash: meta_hash,
        };
    }

    fn count(): usize {
        return _.len;
    }

    fn addPackage(*_, pkg: YarnLockPackage)! {
        if (pkg.name.len == 0) {
            return err.EmptyPackageName;
        }
        if (pkg.version.len == 0) {
            return err.EmptyVersion;
        }
        if (_.len >= MAX_YARN_LOCK_PACKAGES) {
            return err.CapacityExceeded;
        }
        _.packages[_.len] = pkg;
        _.len += 1;
    }

    /// 按 `(name, version)` 字典序原地稳定排序（对齐 Bun `Lockfile.Package.Alphabetizer`）
    fn sortAlphabetical(*_) {
        let i: usize = 1;
        while (i < _.len) {
            const cur = _.packages[i];
            let j: usize = i;
            while (j > 0) {
                const prev = _.packages[j - 1];
                const is_less = if (!std.mem.eql(u8, cur.name, prev.name))
                    lexLessThan(cur.name, prev.name)
                else
                    lexLessThan(cur.version, prev.version);
                if (!is_less) {
                    break;
                }
                _.packages[j] = prev;
                j -= 1;
            }
            _.packages[j] = cur;
            i += 1;
        }
    }

    /// 序列化完整 `yarn.lock` v1 文本到调用方缓冲区 `buf`
    fn printToBuffer(buf: []u8): string! {
        let pos: usize = 0;
        pos = appendBytes(buf, pos, "# THIS IS AN AUTOGENERATED FILE. DO NOT EDIT THIS FILE DIRECTLY.\n# yarn lockfile v1\n")?;
        if (_.meta_hash.len > 0) {
            pos = appendBytes(buf, pos, "# bun ./bun.lockb --hash: ")?;
            pos = appendBytes(buf, pos, _.meta_hash)?;
            pos = appendBytes(buf, pos, "\n")?;
        }
        pos = appendBytes(buf, pos, "\n")?;

        let i: usize = 0;
        while (i < _.len) {
            const pkg = _.packages[i];
            if (i > 0) {
                pos = appendBytes(buf, pos, "\n")?;
            }

            // 1. Header specifiers (deduplicated)
            if (pkg.requested_specs.len == 0) {
                // Default to `^<version>`
                pos = appendByte(buf, pos, '"')?;
                pos = appendBytes(buf, pos, pkg.name)?;
                pos = appendBytes(buf, pos, "@^")?;
                pos = appendBytes(buf, pos, pkg.version)?;
                pos = appendByte(buf, pos, '"')?;
            } else {
                let emitted: usize = 0;
                let s_idx: usize = 0;
                while (s_idx < pkg.requested_specs.len) {
                    const spec = pkg.requested_specs[s_idx];
                    // Deduplicate identical requested specs
                    let dup = false;
                    let prev_idx: usize = 0;
                    while (prev_idx < s_idx) {
                        if (std.mem.eql(u8, pkg.requested_specs[prev_idx], spec)) {
                            dup = true;
                            break;
                        }
                        prev_idx += 1;
                    }
                    if (!dup) {
                        if (emitted > 0) {
                            pos = appendBytes(buf, pos, ", ")?;
                        }
                        const effective_empty = spec.len == 0;
                        const need_q = effective_empty || mustQuoteYarnKey(pkg.name, spec);
                        if (need_q) { pos = appendByte(buf, pos, '"')?; }
                        pos = appendBytes(buf, pos, pkg.name)?;
                        pos = appendByte(buf, pos, '@')?;
                        if (effective_empty) {
                            pos = appendByte(buf, pos, '^')?;
                            pos = appendBytes(buf, pos, pkg.version)?;
                        } else {
                            pos = appendBytes(buf, pos, spec)?;
                        }
                        if (need_q) { pos = appendByte(buf, pos, '"')?; }
                        emitted += 1;
                    }
                    s_idx += 1;
                }
            }
            pos = appendBytes(buf, pos, ":\n")?;

            // 2. version & resolved (always quoted)
            pos = appendBytes(buf, pos, "  version \"")?;
            pos = appendBytes(buf, pos, pkg.version)?;
            pos = appendBytes(buf, pos, "\"\n")?;

            pos = appendBytes(buf, pos, "  resolved \"")?;
            pos = appendBytes(buf, pos, pkg.resolved_url)?;
            pos = appendBytes(buf, pos, "\"\n")?;

            // 3. integrity (never quoted per Yarn v1 & Bun Yarn.cp)
            if (pkg.integrity.len > 0) {
                pos = appendBytes(buf, pos, "  integrity ")?;
                pos = appendBytes(buf, pos, pkg.integrity)?;
                pos = appendBytes(buf, pos, "\n")?;
            }

            // 4. dependencies / optionalDependencies / devDependencies sections
            if (pkg.dependencies.len > 0) {
                pos = writeDepSection(buf, pos, "  dependencies:\n", pkg.dependencies, .prod)?;
                pos = writeDepSection(buf, pos, "  optionalDependencies:\n", pkg.dependencies, .optional)?;
                pos = writeDepSection(buf, pos, "  devDependencies:\n", pkg.dependencies, .dev)?;
            }

            i += 1;
        }

        return buf[0..pos];
    }
}
