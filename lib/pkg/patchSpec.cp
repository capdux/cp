// Source: Absorbed from /data/dev/bun/src/install/PackageManager/patchPackage.cp | Synced: 2026-11-22T12:49:17Z
// Ported from Bun: /data/dev/bun/src/install/PackageManager/patchPackage.cp & /data/dev/bun/src/install/patch_install.cp & /data/dev/bun/docs/pm/cli/patch.mdx
// Synced: 2026-11-22
// Pure-CP zero-allocation `bun patch` / `bun patch --commit` target argument parser,
// `patchedDependencies` key & patch filename formatter, percent-escape sanitizer,
// Global Cache `__buntag_<16hex>` content hash tag codec, and unified diff sanity validator.

/// 默认补丁存放目录（对应 `bun patch --commit` 默认 `patches/`）
pub const PATCH_DEFAULT_DIR: []const u8 = "patches";

/// 全局缓存补丁隔离标签前缀（对应 Bun `bun_hash_tag`）
pub const PATCH_CACHE_HASH_TAG: []const u8 = "__buntag_";

/// 十六进制 64 位补丁哈希长度
pub const PATCH_HEX_HASH_LEN: usize = 16;

/// `bun patch` 参数解析、路径格式化与缓存标签错误集
pub err PatchSpecError {
    EmptyArgument,
    InvalidPackageName,
    InvalidVersion,
    InvalidPatchesDir,
    PathTraversalDetected,
    InvalidCacheTag,
    EmptyPatchContent,
    MalformedUnifiedDiff,
    BufferTooSmall,
}

/// `bun patch` 入参分类：按目录路径定位 或 按包名/版本定位
pub enum PatchTargetKind: u8 {
    PackageSpec = 0,
    NodeModulesPath = 1,
}

/// 解析后的 `bun patch <arg>` 目标定位结构（零堆分配）
pub struct PatchTargetArg {
    kind: PatchTargetKind = PatchTargetKind.PackageSpec,
    raw: []const u8 = "",
    package_name: []const u8 = "",
    version: ([]const u8)? = null,

    /// 解析 `bun patch` 目标参数：
    /// - 目录路径：`node_modules/react`、`./node_modules/@scope/pkg`、`node_modules/a/node_modules/@scope/pkg`
    /// - 精确包名+版本：`react@17.0.2`、`@scope/pkg@1.2.3-beta.1`
    /// - 纯包名：`react`、`@scope/pkg`
    static fn parse(raw_arg: []const u8): PatchTargetArg! {
        const arg = trimWs(raw_arg);
        if (arg.len == 0) {
            return PatchSpecError.EmptyArgument;
        }

        // 检测是否为目录路径（以 `.`、`/`、`\` 开头，或包含 `node_modules/` 或 `node_modules\`）
        const looks_like_path =
            arg[0] == '.' ||
            arg[0] == '/' ||
            arg[0] == '\\' ||
            containsNodeModules(arg);

        if (looks_like_path) {
            const pkg_name = extractPackageNameFromPath(arg)?;
            return PatchTargetArg{
                kind: PatchTargetKind.NodeModulesPath,
                raw: arg,
                package_name: pkg_name,
                version: null,
            };
        }

        // 包名或包名@版本：作用域包从第一个 `/` 之后寻找 `@`
        let search_start: usize = 0;
        if (arg[0] == '@') {
            const slash = indexOfByte(arg, '/');
            if (slash == null) {
                return PatchSpecError.InvalidPackageName;
            }
            const s_idx = slash.?;
            if (s_idx <= 1 || s_idx + 1 >= arg.len) {
                return PatchSpecError.InvalidPackageName;
            }
            search_start = s_idx + 1;
        }

        if (at_rel := indexOfByte(arg[search_start..], '@')) {
            const at_idx = search_start + at_rel;
            const name = arg[0..at_idx];
            const ver = arg[at_idx + 1..];
            if (!isValidPatchPkgName(name)) {
                return PatchSpecError.InvalidPackageName;
            }
            if (ver.len == 0) {
                return PatchSpecError.InvalidVersion;
            }
            return PatchTargetArg{
                kind: PatchTargetKind.PackageSpec,
                raw: arg,
                package_name: name,
                version: ver,
            };
        }

        if (!isValidPatchPkgName(arg)) {
            return PatchSpecError.InvalidPackageName;
        }
        return PatchTargetArg{
            kind: PatchTargetKind.PackageSpec,
            raw: arg,
            package_name: arg,
            version: null,
        };
    }

    fn isPath(): bool {
        return _.kind == PatchTargetKind.NodeModulesPath;
    }

    fn isScoped(): bool {
        return _.package_name.len > 0 && _.package_name[0] == '@';
    }
}

/// 补丁提交元数据视图（零堆分配）
pub struct PatchCommitSpec {
    package_name: []const u8 = "",
    version: []const u8 = "",
    patches_dir: []const u8 = PATCH_DEFAULT_DIR,

    /// 格式化 `package.json` `"patchedDependencies"` 键：`<name>@<version>`
    fn formatPatchKey(out: []u8): usize! {
        if (!isValidPatchPkgName(_.package_name)) {
            return PatchSpecError.InvalidPackageName;
        }
        if (_.version.len == 0) {
            return PatchSpecError.InvalidVersion;
        }
        let pos: usize = 0;
        pos = appendBytes(out, pos, _.package_name)?;
        pos = appendByte(out, pos, '@')?;
        pos = appendBytes(out, pos, _.version)?;
        return pos;
    }

    /// 格式化补丁文件相对路径：`<patches_dir>/<sanitized_name>@<version>.patch`（作用域 `/` 替换为 `+`）
    fn formatPatchFilePath(out: []u8): usize! {
        const dir = normalizePatchesDir(_.patches_dir)?;
        let pos: usize = 0;
        if (dir.len > 0) {
            pos = appendBytes(out, pos, dir)?;
            pos = appendByte(out, pos, '/')?;
        }
        const fname_len = formatPatchFilename(_.package_name, _.version, out[pos..])?;
        return pos + fname_len;
    }
}

/// 按 Bun `patchPackage.cp` `EscapeVal` 规则对路径片段执行百分号转义：
/// `/` -> `%2F`, `\` -> `%5c`, ` ` -> `%20`, `\n` -> `%0A`, `\r` -> `%0D`, `\t` -> `%09`
pub fn escapePatchFilenameSegment(input: []const u8, out: []u8): usize! {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        const c = input[i];
        if (c == '/') {
            pos = appendBytes(out, pos, "%2F")?;
        } else if (c == '\\') {
            pos = appendBytes(out, pos, "%5c")?;
        } else if (c == ' ') {
            pos = appendBytes(out, pos, "%20")?;
        } else if (c == '\n') {
            pos = appendBytes(out, pos, "%0A")?;
        } else if (c == '\r') {
            pos = appendBytes(out, pos, "%0D")?;
        } else if (c == '\t') {
            pos = appendBytes(out, pos, "%09")?;
        } else {
            pos = appendByte(out, pos, c)?;
        }
        i += 1;
    }
    return pos;
}

/// 格式化标准 `.patch` 文件名（作用域包 `@scope/pkg` 规范化为 `@scope+pkg@<version>.patch`）
pub fn formatPatchFilename(package_name: []const u8, version: []const u8, out: []u8): usize! {
    if (!isValidPatchPkgName(package_name)) {
        return PatchSpecError.InvalidPackageName;
    }
    if (version.len == 0) {
        return PatchSpecError.InvalidVersion;
    }
    let pos: usize = 0;
    let i: usize = 0;
    while (i < package_name.len) {
        const c = package_name[i];
        pos = appendByte(out, pos, if (c == '/') '+' else c)?;
        i += 1;
    }
    pos = appendByte(out, pos, '@')?;
    pos = appendBytes(out, pos, version)?;
    pos = appendBytes(out, pos, ".patch")?;
    return pos;
}

/// 校验并规范化 `--patches-dir` 目录（拒绝空目录、绝对路径及 `..` 路径穿越）
pub fn normalizePatchesDir(raw_dir: []const u8): ([]const u8)! {
    const t = trimWs(raw_dir);
    if (t.len == 0) {
        return PatchSpecError.InvalidPatchesDir;
    }
    if (t[0] == '/' || t[0] == '\\') {
        return PatchSpecError.PathTraversalDetected;
    }

    // 剥离前导 `./` 与尾部 `/`
    let start: usize = 0;
    while (start + 2 <= t.len && t[start] == '.' && (t[start + 1] == '/' || t[start + 1] == '\\')) {
        start += 2;
    }
    let end = t.len;
    while (end > start && (t[end - 1] == '/' || t[end - 1] == '\\')) {
        end -= 1;
    }
    const clean = t[start..end];
    if (clean.len == 0) {
        return PatchSpecError.InvalidPatchesDir;
    }

    // 检查任何路径段是否为 `..`
    let i: usize = 0;
    while (i < clean.len) {
        while (i < clean.len && (clean[i] == '/' || clean[i] == '\\')) {
            i += 1;
        }
        const seg_start = i;
        while (i < clean.len && clean[i] != '/' && clean[i] != '\\') {
            i += 1;
        }
        const seg = clean[seg_start..i];
        if (seg.len == 2 && seg[0] == '.' && seg[1] == '.') {
            return PatchSpecError.PathTraversalDetected;
        }
    }

    return clean;
}

/// 计算补丁内容 64 位确定性指纹（FNV-1a 64-bit，供 Global Cache `__buntag_<16hex>` 使用）
pub fn computePatchContentHash(patch_bytes: []const u8): u64 {
    let h: u64 = 0xcbf29ce484222325;
    let i: usize = 0;
    while (i < patch_bytes.len) {
        h ^= @as(u64, patch_bytes[i]);
        h = h *% 0x100000001b3;
        i += 1;
    }
    return h;
}

/// 格式化带 `__buntag_<16hex>` 隔离标签的全局缓存子路径（对齐 `patch_install.cp`）
pub fn formatPatchCacheSubpath(base_subpath: []const u8, patch_hash: u64, out: []u8): usize! {
    let pos: usize = 0;
    pos = appendBytes(out, pos, base_subpath)?;
    pos = appendBytes(out, pos, PATCH_CACHE_HASH_TAG)?;
    const hex = "0123456789abcdef";
    let shift: u6 = 60;
    let k: usize = 0;
    while (k < PATCH_HEX_HASH_LEN) {
        const nibble = @as(usize, @intCast((patch_hash >> shift) & 0x0f));
        pos = appendByte(out, pos, hex[nibble])?;
        if (shift >= 4) {
            shift -= 4;
        }
        k += 1;
    }
    return pos;
}

/// 从带 `__buntag_<16hex>` 后缀的缓存路径中反解出 64 位补丁哈希（支持 round-trip 校验）
pub fn parsePatchCacheHash(subpath: []const u8): u64! {
    const tag_len = PATCH_CACHE_HASH_TAG.len;
    if (subpath.len < tag_len + PATCH_HEX_HASH_LEN) {
        return PatchSpecError.InvalidCacheTag;
    }
    const tag_start = subpath.len - PATCH_HEX_HASH_LEN - tag_len;
    if (!bytesEql(subpath[tag_start..tag_start + tag_len], PATCH_CACHE_HASH_TAG)) {
        return PatchSpecError.InvalidCacheTag;
    }
    const hex_slice = subpath[tag_start + tag_len..];
    let val: u64 = 0;
    let i: usize = 0;
    while (i < PATCH_HEX_HASH_LEN) {
        const d = hexDigit(hex_slice[i]) ?? return PatchSpecError.InvalidCacheTag;
        val = (val << 4) | @as(u64, d);
        i += 1;
    }
    return val;
}

/// 预检 `.patch` 文本是否符合基本 Unified Diff 结构（至少包含 `--- `、`+++ ` 与 `@@ ` 块头）
pub fn validateUnifiedPatchText(patch_text: []const u8)! {
    if (trimWs(patch_text).len == 0) {
        return PatchSpecError.EmptyPatchContent;
    }
    let has_minus = false;
    let has_plus = false;
    let has_hunk = false;

    let i: usize = 0;
    while (i < patch_text.len) {
        const line_start = i;
        while (i < patch_text.len && patch_text[i] != '\n') {
            i += 1;
        }
        const line = patch_text[line_start..i];
        if (i < patch_text.len && patch_text[i] == '\n') {
            i += 1;
        }
        if (startsWith(line, "--- ")) {
            has_minus = true;
        } else if (startsWith(line, "+++ ")) {
            has_plus = true;
        } else if (startsWith(line, "@@ ")) {
            has_hunk = true;
        }
    }

    if (!has_minus || !has_plus || !has_hunk) {
        return PatchSpecError.MalformedUnifiedDiff;
    }
}

// ============================================================================
// 内部零分配辅助函数
// ============================================================================

fn isWs(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn trimWs(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isWs(s[start])) {
        start += 1;
    }
    let end = s.len;
    while (end > start && isWs(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
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

fn startsWith(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) {
        return false;
    }
    return bytesEql(s[0..prefix.len], prefix);
}

fn indexOfByte(s: []const u8, needle: u8): usize? {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == needle) {
            return i;
        }
        i += 1;
    }
    return null;
}

fn hexDigit(c: u8): u8? {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return null;
}

fn appendByte(out: []u8, pos: usize, b: u8): usize! {
    if (pos >= out.len) {
        return PatchSpecError.BufferTooSmall;
    }
    out[pos] = b;
    return pos + 1;
}

fn appendBytes(out: []u8, pos: usize, src: []const u8): usize! {
    if (src.len > out.len - pos) {
        return PatchSpecError.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < src.len) {
        out[pos + i] = src[i];
        i += 1;
    }
    return pos + src.len;
}

fn isValidPatchPkgName(name: []const u8): bool {
    if (name.len == 0) {
        return false;
    }
    if (name[0] == '.' || name[0] == '_' || name[0] == '/') {
        return false;
    }
    let slash_count: usize = 0;
    let i: usize = 0;
    while (i < name.len) {
        const c = name[i];
        if (isWs(c) || c == '\\') {
            return false;
        }
        if (c == '/') {
            slash_count += 1;
        }
        i += 1;
    }
    if (name[0] == '@') {
        return slash_count == 1 && name[name.len - 1] != '/';
    }
    return slash_count == 0;
}

fn containsNodeModules(s: []const u8): bool {
    const nm = "node_modules";
    if (s.len < nm.len) {
        return false;
    }
    let i: usize = 0;
    while (i + nm.len <= s.len) {
        if (bytesEql(s[i..i + nm.len], nm)) {
            const left_ok = i == 0 || s[i - 1] == '/' || s[i - 1] == '\\';
            const right_ok = i + nm.len == s.len || s[i + nm.len] == '/' || s[i + nm.len] == '\\';
            if (left_ok && right_ok) {
                return true;
            }
        }
        i += 1;
    }
    return false;
}

fn extractPackageNameFromPath(path: []const u8): ([]const u8)! {
    let end = path.len;
    while (end > 0 && (path[end - 1] == '/' || path[end - 1] == '\\')) {
        end -= 1;
    }
    if (end == 0) {
        return PatchSpecError.InvalidPackageName;
    }

    // 定位最后一个 `node_modules/` 后的包名（如 `node_modules/a/node_modules/@scope/pkg` -> `@scope/pkg`）
    const nm = "node_modules";
    let last_nm_after: usize? = null;
    if (end >= nm.len) {
        let i: usize = 0;
        while (i + nm.len < end) {
            if (bytesEql(path[i..i + nm.len], nm)) {
                const left_ok = i == 0 || path[i - 1] == '/' || path[i - 1] == '\\';
                const sep_ok = path[i + nm.len] == '/' || path[i + nm.len] == '\\';
                if (left_ok && sep_ok) {
                    last_nm_after = i + nm.len + 1;
                }
            }
            i += 1;
        }
    }

    if (after := last_nm_after) {
        const rest = path[after..end];
        if (rest.len == 0) {
            return PatchSpecError.InvalidPackageName;
        }
        if (rest[0] == '@') {
            if (!isValidPatchPkgName(rest)) {
                return PatchSpecError.InvalidPackageName;
            }
            return rest;
        }
        if (!isValidPatchPkgName(rest)) {
            return PatchSpecError.InvalidPackageName;
        }
        return rest;
    }

    // 非 node_modules 路径：取末段（或 `@scope/pkg` 末两段）
    let p = end;
    while (p > 0 && path[p - 1] != '/' && path[p - 1] != '\\') {
        p -= 1;
    }
    if (p >= 2) {
        let prev = p - 1;
        while (prev > 0 && path[prev - 1] != '/' && path[prev - 1] != '\\') {
            prev -= 1;
        }
        if (path[prev] == '@') {
            const scoped = path[prev..end];
            if (isValidPatchPkgName(scoped)) {
                return scoped;
            }
        }
    }
    const base = path[p..end];
    if (!isValidPatchPkgName(base)) {
        return PatchSpecError.InvalidPackageName;
    }
    return base;
}
