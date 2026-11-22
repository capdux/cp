// Source: Absorbed from /data/dev/bun/src/cli/bunx_command.cp | Synced: 2026-11-22T10:34:15Z
// Ported from Bun: /data/dev/bun/src/cli/bunx_command.cp & /data/dev/bun/docs/pm/bunx.mdx
// Synced: 2026-11-22
// Pure-CP zero-allocation `bunx` / `bun x` CLI option parser, `create-*` package prefixer,
// scoped/versioned package specifier parser, 24h cache TTL validator, safe cache directory
// formatter, and shebang runtime resolver (`--bun` override vs `#!/usr/bin/env node`).

/// `bunx` 临时安装缓存有效时长（24 小时 = 86400 秒，对齐 Bun `seconds_cache_valid`）
pub const BUNX_CACHE_TTL_SECS: i64 = 60 * 60 * 24;

/// `bunx` 参数解析与格式化错误集
pub err BunxError {
    MissingPackageValue,
    EmptyPackageName,
    MissingBinaryName,
    MissingPackageName,
    InvalidPackageSpec,
    UnsafeCacheSpec,
    BufferTooSmall,
}

/// 可执行脚本运行时选择结果（对齐 `docs/pm/bunx.mdx` Shebang 规范）
pub enum BunxRuntimeChoice: u8 {
    /// 使用 Bun 运行时执行（显式 `--bun`、`#!/usr/bin/env bun` 或无 shebang）
    Bun = 0,
    /// 使用 Node.js 运行时执行（`#!/usr/bin/env node` 且未指定 `--bun`）
    Node = 1,
    /// 交由脚本自身声明的其他 shebang 解释器执行（如 `#!/bin/sh`）
    Shebang = 2,
}

/// 解析后的包说明符（区分包名、可选作用域与可选版本/标签）
pub struct BunxPackageSpec {
    /// 规范包名（如 `cowsay` 或 `@angular/cli`）
    package_name: []const u8 = "",
    /// 可选版本或 dist-tag（如 `1.5.0`、`latest`，无则为 `null`）
    version: ([]const u8)? = null,
    /// 是否为作用域包（以 `@` 开头）
    is_scoped: bool = false,
}

/// 零堆分配 `bunx` / `bun x` 命令行解析结果
pub struct BunxOptions {
    /// 需要安装或解析的包说明符（若使用 `-p/--package` 则为其值，否则为首个位置参数）
    package_name: []const u8 = "",
    /// 显式指定的目标可执行文件名（仅在 `-p/--package` 时非空）
    binary_name: ([]const u8)? = null,
    /// 显式 `--package` / `-p` 原始参数值
    specified_package: ([]const u8)? = null,
    /// 透传给目标可执行文件的参数在输入 `argv` 中的起始下标
    passthrough_start: usize = 0,
    /// 透传参数个数
    passthrough_count: usize = 0,
    /// 是否强制以 Bun 运行时执行（`--bun` / `-b`）
    run_in_bun: bool = false,
    /// 是否输出详细安装日志（`--verbose`）
    verbose_install: bool = false,
    /// 是否静默安装（`--silent`）
    silent_install: bool = false,
    /// 是否禁止自动安装（`--no-install`）
    no_install: bool = false,
    /// 无包名时是否请求打印版本（`--version` / `-v`）
    show_version: bool = false,
    /// 无包名时是否请求打印修订号（`--revision`）
    show_revision: bool = false,

    /// 解析命令行参数切片（`skip_subcommand = true` 时跳过首个子命令如 `bunx` 或 `x`）。
    /// 严格遵循 Bun 规则：目标包/可执行文件之前出现的标志由 `bunx` 消费，之后的所有参数原样透传。
    static fn parse(argv: []const ([]const u8), skip_subcommand: bool): BunxOptions! {
        let opts = BunxOptions{};
        let found_subcommand = !skip_subcommand;
        let maybe_target: ([]const u8)? = null;
        let has_version = false;
        let has_revision = false;

        let i: usize = 0;
        while (i < argv.len) {
            const arg = argv[i];

            if (maybe_target != null) {
                if (opts.passthrough_count == 0) {
                    opts.passthrough_start = i;
                }
                opts.passthrough_count += 1;
                i += 1;
                continue;
            }

            if (arg.len > 0 && arg[0] == '-') {
                if (bytesEql(arg, "--version") || bytesEql(arg, "-v")) {
                    has_version = true;
                } else if (bytesEql(arg, "--revision")) {
                    has_revision = true;
                } else if (bytesEql(arg, "--verbose")) {
                    opts.verbose_install = true;
                } else if (bytesEql(arg, "--silent")) {
                    opts.silent_install = true;
                } else if (bytesEql(arg, "--bun") || bytesEql(arg, "-b")) {
                    opts.run_in_bun = true;
                } else if (bytesEql(arg, "--no-install")) {
                    opts.no_install = true;
                } else if (bytesEql(arg, "--package") || bytesEql(arg, "-p")) {
                    i += 1;
                    if (i >= argv.len) {
                        return BunxError.MissingPackageValue;
                    }
                    if (argv[i].len == 0) {
                        return BunxError.EmptyPackageName;
                    }
                    opts.specified_package = argv[i];
                } else if (startsWith(arg, "--package=")) {
                    const val = arg["--package=".len..];
                    if (val.len == 0) {
                        return BunxError.EmptyPackageName;
                    }
                    opts.specified_package = val;
                } else if (startsWith(arg, "-p=")) {
                    const val = arg["-p=".len..];
                    if (val.len == 0) {
                        return BunxError.EmptyPackageName;
                    }
                    opts.specified_package = val;
                }
                i += 1;
                continue;
            }

            if (!found_subcommand) {
                found_subcommand = true;
            } else {
                maybe_target = arg;
            }
            i += 1;
        }

        if (spec_pkg := opts.specified_package) {
            if (target := maybe_target) {
                if (target.len == 0) {
                    return BunxError.MissingBinaryName;
                }
                opts.binary_name = target;
                opts.package_name = spec_pkg;
                return opts;
            }
            return BunxError.MissingBinaryName;
        }

        if (target := maybe_target) {
            if (target.len == 0) {
                return BunxError.MissingPackageName;
            }
            opts.package_name = target;
            return opts;
        }

        if (has_revision) {
            opts.show_revision = true;
            return opts;
        }
        if (has_version) {
            opts.show_version = true;
            return opts;
        }
        return BunxError.MissingPackageName;
    }

    /// 返回透传参数切片视图（零拷贝借用原始 `argv`）
    fn passthroughSlice(argv: []const ([]const u8)): []const ([]const u8) {
        if (_.passthrough_count == 0 || _.passthrough_start >= argv.len) {
            return argv[0..0];
        }
        const end = _.passthrough_start + _.passthrough_count;
        const clamped_end = if (end > argv.len) argv.len else end;
        return argv[_.passthrough_start..clamped_end];
    }
}

fn bytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn startsWith(haystack: []const u8, prefix: []const u8): bool {
    if (haystack.len < prefix.len) {
        return false;
    }
    return bytesEql(haystack[0..prefix.len], prefix);
}

fn endsWith(haystack: []const u8, suffix: []const u8): bool {
    if (haystack.len < suffix.len) {
        return false;
    }
    return bytesEql(haystack[haystack.len - suffix.len ..], suffix);
}

fn indexOfChar(s: []const u8, target: u8): usize? {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == target) {
            return i;
        }
        i += 1;
    }
    return null;
}

fn lastIndexOfChar(s: []const u8, target: u8): usize? {
    let i: usize = s.len;
    while (i > 0) {
        i -= 1;
        if (s[i] == target) {
            return i;
        }
    }
    return null;
}

fn appendBytes(out: []u8, pos: *usize, src: []const u8): void! {
    if (pos.* + src.len > out.len) {
        return BunxError.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < src.len) {
        out[pos.* + i] = src[i];
        i += 1;
    }
    pos.* += src.len;
}

fn appendByte(out: []u8, pos: *usize, b: u8): void! {
    if (pos.* + 1 > out.len) {
        return BunxError.BufferTooSmall;
    }
    out[pos.*] = b;
    pos.* += 1;
}

fn appendU64(out: []u8, pos: *usize, val_in: u64): void! {
    if (val_in == 0) {
        appendByte(out, pos, '0')?;
        return;
    }
    let rev: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val_in;
    while (v > 0) {
        rev[n] = ('0' as u8) + ((v % 10) as u8);
        v /= 10;
        n += 1;
    }
    if (pos.* + n > out.len) {
        return BunxError.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < n) {
        out[pos.* + i] = rev[n - 1 - i];
        i += 1;
    }
    pos.* += n;
}

/// 严格对齐 Bun `bunxAddCreatePrefix`：为模板包名注入 `create-` 或 `/create` 前缀。
/// - `"vite"` -> `"create-vite"`
/// - `"@scope/app"` -> `"@scope/create-app"`
/// - `"@scope@1.2.3"` -> `"@scope/create@1.2.3"`
/// - `"@scope"` -> `"@scope/create"`
pub fn formatBunxCreatePackageName(input: []const u8, out: []u8): usize! {
    if (input.len == 0) {
        return 0;
    }
    let pos: usize = 0;
    if (input[0] == '@') {
        if (slash_i := indexOfChar(input, '/')) {
            const idx = slash_i + 1;
            appendBytes(out, &pos, input[0..idx])?;
            appendBytes(out, &pos, "create-")?;
            appendBytes(out, &pos, input[idx..])?;
            return pos;
        }
        if (at_rel := indexOfChar(input[1..], '@')) {
            const idx = at_rel + 1;
            appendBytes(out, &pos, input[0..idx])?;
            appendBytes(out, &pos, "/create")?;
            appendBytes(out, &pos, input[idx..])?;
            return pos;
        }
        appendBytes(out, &pos, input)?;
        appendBytes(out, &pos, "/create")?;
        return pos;
    }
    appendBytes(out, &pos, "create-")?;
    appendBytes(out, &pos, input)?;
    return pos;
}

/// 解析 `bunx` 包说明符为 `(package_name, version)`，支持作用域包与 `@version` 后缀。
pub fn parseBunxPackageSpec(raw: []const u8): BunxPackageSpec! {
    if (raw.len == 0) {
        return BunxError.InvalidPackageSpec;
    }
    if (raw[0] == '@') {
        // 作用域包必须包含 `/` 且作用域与包名均非空
        if (slash_i := indexOfChar(raw, '/')) {
            if (slash_i <= 1 || slash_i + 1 >= raw.len) {
                return BunxError.InvalidPackageSpec;
            }
            const after_slash = raw[slash_i + 1 ..];
            if (at_rel := indexOfChar(after_slash, '@')) {
                if (at_rel == 0) {
                    return BunxError.InvalidPackageSpec;
                }
                const pkg_end = slash_i + 1 + at_rel;
                const ver_slice = raw[pkg_end + 1 ..];
                return BunxPackageSpec{
                    package_name: raw[0..pkg_end],
                    version: if (ver_slice.len > 0) ver_slice else null,
                    is_scoped: true,
                };
            }
            return BunxPackageSpec{
                package_name: raw,
                version: null,
                is_scoped: true,
            };
        }
        return BunxError.InvalidPackageSpec;
    }

    if (at_i := indexOfChar(raw, '@')) {
        if (at_i == 0) {
            return BunxError.InvalidPackageSpec;
        }
        const ver_slice = raw[at_i + 1 ..];
        return BunxPackageSpec{
            package_name: raw[0..at_i],
            version: if (ver_slice.len > 0) ver_slice else null,
            is_scoped: false,
        };
    }
    return BunxPackageSpec{
        package_name: raw,
        version: null,
        is_scoped: false,
    };
}

/// 根据包名推导默认可执行文件名：非作用域包取包名本身，作用域包 `@scope/name` 取 `name`
pub fn resolveBunxDefaultBinName(package_name: []const u8): []const u8 {
    if (package_name.len > 0 && package_name[0] == '@') {
        if (slash_i := indexOfChar(package_name, '/')) {
            if (slash_i + 1 < package_name.len) {
                return package_name[slash_i + 1 ..];
            }
        }
    }
    return package_name;
}

/// 从 `package.json` 的 `"bin"` 字符串路径中提取基础文件名（同时支持 `/` 与 `\` 分隔符）
pub fn extractBinBasename(bin_path: []const u8): []const u8 {
    if (slash := lastIndexOfChar(bin_path, '/')) {
        if (backslash := lastIndexOfChar(bin_path, '\\')) {
            const cut = if (slash > backslash) slash else backslash;
            return bin_path[cut + 1 ..];
        }
        return bin_path[slash + 1 ..];
    }
    if (backslash := lastIndexOfChar(bin_path, '\\')) {
        return bin_path[backslash + 1 ..];
    }
    return bin_path;
}

/// 判断 `bunx` 临时缓存目录是否已过期（超过 24 小时或无效时间戳视为过期）
pub fn isBunxCacheStale(mtime_sec: i64, now_sec: i64): bool {
    if (mtime_sec <= 0) {
        return true;
    }
    return (now_sec - mtime_sec) > BUNX_CACHE_TTL_SECS;
}

/// 构造安全的 `bunx` 临时缓存目录名（`bunx-<uid>-<sanitized_spec>`）。
/// 自动拦截 `..` 路径穿越段，并将 `/`、`\`、`:` 规范替换为 `+`。
pub fn formatBunxCacheDirName(uid: u32, package_spec: []const u8, out: []u8): usize! {
    if (package_spec.len == 0) {
        return BunxError.EmptyPackageName;
    }
    // 拦截路径穿越 ".." 或控制字符
    let i: usize = 0;
    while (i < package_spec.len) {
        const c = package_spec[i];
        if (c == 0 || c == '\n' || c == '\r') {
            return BunxError.UnsafeCacheSpec;
        }
        if (c == '.' && i + 1 < package_spec.len && package_spec[i + 1] == '.') {
            return BunxError.UnsafeCacheSpec;
        }
        i += 1;
    }

    let pos: usize = 0;
    appendBytes(out, &pos, "bunx-")?;
    appendU64(out, &pos, uid as u64)?;
    appendByte(out, &pos, '-')?;

    let j: usize = 0;
    while (j < package_spec.len) {
        const c = package_spec[j];
        const safe_c = if (c == '/' || c == '\\' || c == ':') '+' else c;
        appendByte(out, &pos, safe_c)?;
        j += 1;
    }
    return pos;
}

fn trimSpaces(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && (s[start] == ' ' || s[start] == '\t' || s[start] == '\r')) {
        start += 1;
    }
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r')) {
        end -= 1;
    }
    return s[start..end];
}

/// 依据脚本首行 Shebang 与 `--bun` 标志决定运行时（对齐 `docs/pm/bunx.mdx`）
pub fn resolveBunxShebangRuntime(script_head: []const u8, force_bun: bool): BunxRuntimeChoice {
    if (force_bun) {
        return BunxRuntimeChoice.Bun;
    }
    if (script_head.len < 2 || script_head[0] != '#' || script_head[1] != '!') {
        return BunxRuntimeChoice.Bun;
    }
    // 截取首行
    let line_end: usize = script_head.len;
    if (nl := indexOfChar(script_head, '\n')) {
        line_end = nl;
    }
    const line = trimSpaces(script_head[2..line_end]);
    if (line.len == 0) {
        return BunxRuntimeChoice.Bun;
    }

    // 解析 interpreter 与可选 `env` 参数（如 `/usr/bin/env -S node` 或 `/usr/local/bin/bun`）
    let first_end: usize = 0;
    while (first_end < line.len && line[first_end] != ' ' && line[first_end] != '\t') {
        first_end += 1;
    }
    const first_tok = line[0..first_end];
    const first_base = extractBinBasename(first_tok);

    if (bytesEql(first_base, "bun") || bytesEql(first_base, "bunx")) {
        return BunxRuntimeChoice.Bun;
    }
    if (bytesEql(first_base, "node") || bytesEql(first_base, "nodejs")) {
        return BunxRuntimeChoice.Node;
    }

    if (bytesEql(first_base, "env")) {
        let rem = trimSpaces(line[first_end..]);
        // 跳过 env 标志（例如 `-S`）
        while (rem.len > 0 && rem[0] == '-') {
            let tok_end: usize = 0;
            while (tok_end < rem.len && rem[tok_end] != ' ' && rem[tok_end] != '\t') {
                tok_end += 1;
            }
            rem = trimSpaces(rem[tok_end..]);
        }
        let target_end: usize = 0;
        while (target_end < rem.len && rem[target_end] != ' ' && rem[target_end] != '\t') {
            target_end += 1;
        }
        const target = extractBinBasename(rem[0..target_end]);
        if (bytesEql(target, "bun") || bytesEql(target, "bunx")) {
            return BunxRuntimeChoice.Bun;
        }
        if (bytesEql(target, "node") || bytesEql(target, "nodejs")) {
            return BunxRuntimeChoice.Node;
        }
    }

    return BunxRuntimeChoice.Shebang;
}
