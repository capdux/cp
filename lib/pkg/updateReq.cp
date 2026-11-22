// Source: Absorbed from /data/dev/bun/src/install/PackageManager/UpdateRequest.cp | Synced: 2026-11-22T12:49:17Z
// Pure-CP zero-allocation CLI package update/add/remove/link positional
// specifier parser, protocol classifier, and deduplicating request list.

import { PackageNameHash, hashPackageName } from "./types.cp";
import { isValidPackageName } from "./name.cp";

pub const MAX_UPDATE_REQUESTS: usize = 64;

/// CLI 依赖更新请求解析与批量收集错误集
pub err UpdateRequestError {
    EmptyInput,
    UnrecognizedDependencyFormat,
    CapacityExceeded,
    BufferTooSmall,
}

/// 包管理 CLI 子命令类型
pub enum PackageSubcommand: u8 {
    add = 0,
    remove = 1,
    update = 2,
    link = 3,
    unlink = 4,
}

/// 解析后的依赖说明符协议分类
pub enum UpdateSpecKind: u8 {
    npm_semver = 0,
    dist_tag = 1,
    npm_alias = 2,
    link = 3,
    file = 4,
    workspace = 5,
    tarball = 6,
    git = 7,
}

fn isTrimSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

fn trimSpaces(s: string): string {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && isTrimSpace(s[start])) {
        start += 1;
    }
    while (end > start && isTrimSpace(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn startsWith(s: string, prefix: string): bool {
    if (s.len < prefix.len) {
        return false;
    }
    return std.mem.eql(u8, s[0..prefix.len], prefix);
}

fn endsWith(s: string, suffix: string): bool {
    if (s.len < suffix.len) {
        return false;
    }
    return std.mem.eql(u8, s[s.len - suffix.len ..], suffix);
}

fn indexOfChar(s: string, ch: u8): usize? {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == ch) {
            return i;
        }
        i += 1;
    }
    return null;
}

fn isTarballSpec(s: string): bool {
    return startsWith(s, "http://") ||
        startsWith(s, "https://") ||
        endsWith(s, ".tgz") ||
        endsWith(s, ".tar.gz");
}

fn isGitProtocolSpec(s: string): bool {
    return startsWith(s, "git+") ||
        startsWith(s, "git://") ||
        startsWith(s, "github:") ||
        startsWith(s, "gitlab:") ||
        startsWith(s, "bitbucket:") ||
        endsWith(s, ".git");
}

fn isLocalFileSpec(s: string): bool {
    return startsWith(s, "file:") ||
        startsWith(s, "./") ||
        startsWith(s, "../") ||
        startsWith(s, "/") ||
        startsWith(s, "~/");
}

/// 判断是否为无协议前缀的 GitHub 简写 `owner/repo`（非 scoped `@org/pkg`）
fn isGithubShorthand(s: string): bool {
    if (s.len < 3 || s[0] == '@' || s[0] == '.' || s[0] == '/') {
        return false;
    }
    const slash = indexOfChar(s, '/') ?? return false;
    if (slash == 0 || slash + 1 >= s.len) {
        return false;
    }
    // 禁止多个斜杠（#ref 之前）
    const rest = s[slash + 1 ..];
    const hash_pos = indexOfChar(rest, '#') ?? rest.len;
    if (hash_pos == 0) {
        return false;
    }
    if (indexOfChar(rest[0..hash_pos], '/') != null) {
        return false;
    }
    return true;
}

/// 区分 semver 范围/版本号 vs dist-tag（如 `latest`/`canary`/`beta`）
fn looksLikeSemverOrRange(spec: string): bool {
    if (spec.len == 0) {
        return false;
    }
    const c = spec[0];
    if ((c >= '0' && c <= '9') || c == '^' || c == '~' || c == '>' || c == '<' || c == '=' || c == '*') {
        return true;
    }
    if (c == 'v' && spec.len > 1 && spec[1] >= '0' && spec[1] <= '9') {
        return true;
    }
    return false;
}

/// 根据说明符值分类其目标协议
fn classifySpecValue(spec: string): UpdateSpecKind {
    if (spec.len == 0) {
        return .dist_tag;
    }
    if (startsWith(spec, "npm:")) {
        return .npm_alias;
    }
    if (startsWith(spec, "link:")) {
        return .link;
    }
    if (startsWith(spec, "workspace:")) {
        return .workspace;
    }
    if (isLocalFileSpec(spec)) {
        return .file;
    }
    if (isGitProtocolSpec(spec)) {
        return .git;
    }
    if (isTarballSpec(spec)) {
        return .tarball;
    }
    if (looksLikeSemverOrRange(spec)) {
        return .npm_semver;
    }
    return .dist_tag;
}

fn appendBytes(buf: []u8, pos: usize, s: string): usize! {
    if (pos + s.len > buf.len) {
        return err.BufferTooSmall;
    }
    @memcpy(buf[pos .. pos + s.len], s);
    return pos + s.len;
}

/// 单条已解析的包管理 CLI 更新请求（零堆分配）
pub struct UpdateRequest {
    name: string = "",
    name_hash: PackageNameHash = 0,
    spec: string = "",
    raw: string = "",
    kind: UpdateSpecKind = .dist_tag,
    is_aliased: bool = false,

    /// 解析单条 CLI 位置参数（对齐 Bun `UpdateRequest.parseWithError` 核心规则）
    static fn parse(raw_positional: string, subcommand: PackageSubcommand): UpdateRequest! {
        const input = trimSpaces(raw_positional);
        if (input.len == 0) {
            return err.EmptyInput;
        }

        // 1. `link` / `unlink` 子命令自动将裸包名归类为 `.link`
        if (subcommand == .link || subcommand == .unlink) {
            if (startsWith(input, "link:")) {
                const target = input[5..];
                if (target.len == 0 || !isValidPackageName(target)) {
                    return err.UnrecognizedDependencyFormat;
                }
                return UpdateRequest{
                    name: target,
                    name_hash: hashPackageName(target),
                    spec: input,
                    raw: input,
                    kind: .link,
                    is_aliased: true,
                };
            }
            if (!isValidPackageName(input)) {
                return err.UnrecognizedDependencyFormat;
            }
            return UpdateRequest{
                name: input,
                name_hash: hashPackageName(input),
                spec: input,
                raw: input,
                kind: .link,
                is_aliased: true,
            };
        }

        // 2. 直接本地路径 / tarball / git URL（无 `name@` 别名）
        if (isLocalFileSpec(input)) {
            return UpdateRequest{
                name: "",
                name_hash: hashPackageName(input),
                spec: input,
                raw: input,
                kind: .file,
                is_aliased: false,
            };
        }
        if (isGitProtocolSpec(input)) {
            return UpdateRequest{
                name: "",
                name_hash: hashPackageName(input),
                spec: input,
                raw: input,
                kind: .git,
                is_aliased: false,
            };
        }
        if (isTarballSpec(input)) {
            return UpdateRequest{
                name: "",
                name_hash: hashPackageName(input),
                spec: input,
                raw: input,
                kind: .tarball,
                is_aliased: false,
            };
        }

        // 3. 纯 npm 包名（无 `@version` 后缀，如 `lodash` 或 `@scope/pkg`）
        if (isValidPackageName(input)) {
            return UpdateRequest{
                name: input,
                name_hash: hashPackageName(input),
                spec: "",
                raw: input,
                kind: .dist_tag,
                is_aliased: true,
            };
        }

        // 4. `name@spec` 切分（从索引 1 开始寻找 `@` 以正确兼容 scoped 包 `@scope/pkg@1.0.0`）
        if (input.len > 1) {
            if (at_rel := indexOfChar(input[1..], '@')) {
                const pkg_name = input[0 .. at_rel + 1];
                const spec_val = input[at_rel + 2 ..];
                if (isValidPackageName(pkg_name)) {
                    if (spec_val.len == 0) {
                        return err.UnrecognizedDependencyFormat;
                    }
                    const kind = classifySpecValue(spec_val);
                    // 校验 `npm:` 别名内部非空
                    if (kind == .npm_alias && spec_val.len <= 4) {
                        return err.UnrecognizedDependencyFormat;
                    }
                    return UpdateRequest{
                        name: pkg_name,
                        name_hash: hashPackageName(pkg_name),
                        spec: spec_val,
                        raw: input,
                        kind: kind,
                        is_aliased: true,
                    };
                }
            }
        }

        // 5. GitHub 简写 `owner/repo` 或 `owner/repo#ref`
        if (isGithubShorthand(input)) {
            return UpdateRequest{
                name: "",
                name_hash: hashPackageName(input),
                spec: input,
                raw: input,
                kind: .git,
                is_aliased: false,
            };
        }

        return err.UnrecognizedDependencyFormat;
    }

    /// 获取用于匹配或展示的有效名称（对齐 Bun `UpdateRequest.getName()`）
    fn getName(): string {
        return if (_.is_aliased) _.name else _.spec;
    }

    /// 按哈希与包名精确匹配依赖名
    fn matchesName(dep_name: string): bool {
        const h = hashPackageName(dep_name);
        if (_.name_hash != h) {
            return false;
        }
        return std.mem.eql(u8, _.getName(), dep_name);
    }

    /// 规范格式化为 `name@spec` 或 `link:name` 写入目标缓冲
    fn formatSpec(buf: []u8): string! {
        let pos: usize = 0;
        if (_.kind == .link) {
            if (startsWith(_.spec, "link:")) {
                pos = appendBytes(buf, pos, _.spec)?;
            } else {
                pos = appendBytes(buf, pos, "link:")?;
                pos = appendBytes(buf, pos, _.name)?;
            }
            return buf[0..pos];
        }
        if (_.is_aliased) {
            pos = appendBytes(buf, pos, _.name)?;
            if (_.spec.len > 0) {
                pos = appendBytes(buf, pos, "@")?;
                pos = appendBytes(buf, pos, _.spec)?;
            }
            return buf[0..pos];
        }
        pos = appendBytes(buf, pos, _.spec)?;
        return buf[0..pos];
    }
}

/// 零堆分配批量更新请求列表（自动按 `name_hash` + `name.len` 去重，对齐 Bun `UpdateRequest.parseWithError`）
pub struct UpdateRequestList {
    entries: [MAX_UPDATE_REQUESTS]UpdateRequest = undefined,
    len: usize = 0,

    static fn init(): UpdateRequestList {
        return UpdateRequestList{
            entries: undefined,
            len: 0,
        };
    }

    /// 批量解析 CLI 位置参数并自动去重（首次出现优先）
    static fn parseAll(positionals: []const string, subcommand: PackageSubcommand): UpdateRequestList! {
        let list = UpdateRequestList.init();
        let i: usize = 0;
        while (i < positionals.len) {
            const req = UpdateRequest.parse(positionals[i], subcommand)?;
            let dup = false;
            let j: usize = 0;
            while (j < list.len) {
                if (list.entries[j].name_hash == req.name_hash && list.entries[j].name.len == req.name.len) {
                    dup = true;
                    break;
                }
                j += 1;
            }
            if (!dup) {
                if (list.len >= MAX_UPDATE_REQUESTS) {
                    return err.CapacityExceeded;
                }
                list.entries[list.len] = req;
                list.len += 1;
            }
            i += 1;
        }
        return list;
    }

    fn count(): usize {
        return _.len;
    }

    fn items(): []const UpdateRequest {
        return _.entries[0.._.len];
    }

    fn contains(dep_name: string): bool {
        const h = hashPackageName(dep_name);
        let i: usize = 0;
        while (i < _.len) {
            if (_.entries[i].name_hash == h && std.mem.eql(u8, _.entries[i].getName(), dep_name)) {
                return true;
            }
            i += 1;
        }
        return false;
    }
}
