// Source: Absorbed from /data/dev/bun/src/cli/pm_version_command.cp | Synced: 2026-11-22T12:43:45Z
// Pure-CP zero-allocation `pm version` semver bump calculator, argument parser,
// prerelease identifier resolver, `from-git` tag normalizer, and `%s` commit message formatter.

/// `pm version` 参数解析、版本递进与消息格式化错误集
pub err PmVersionError {
    InvalidArgument,
    InvalidSemver,
    InvalidPreid,
    SameVersion,
    FromGitRequiresTag,
    Overflow,
    BufferTooSmall,
}

fn isWs(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn isSemverIdentChar(c: u8): bool {
    return (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') ||
        c == '-';
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

fn indexOfChar(s: []const u8, ch: u8): usize? {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == ch) {
            return i;
        }
        i += 1;
    }
    return null;
}

fn lastIndexOfChar(s: []const u8, ch: u8): usize? {
    let i = s.len;
    while (i > 0) {
        i -= 1;
        if (s[i] == ch) {
            return i;
        }
    }
    return null;
}

fn parseStrictU32(s: []const u8): u32? {
    if (s.len == 0) {
        return null;
    }
    // Semver 2.0 数值标识符不允许前导零（"0" 除外）
    if (s.len > 1 && s[0] == '0') {
        return null;
    }
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (!isDigit(c)) {
            return null;
        }
        acc = acc * 10 + ((c - '0') as u64);
        if (acc > 0xFFFF_FFFF) {
            return null;
        }
        i += 1;
    }
    return acc as u32;
}

fn isValidDotSeparatedIdents(s: []const u8): bool {
    if (s.len == 0) {
        return false;
    }
    let seg_start: usize = 0;
    let i: usize = 0;
    while (i <= s.len) {
        if (i == s.len || s[i] == '.') {
            if (i == seg_start) {
                return false;
            }
            let k = seg_start;
            while (k < i) {
                if (!isSemverIdentChar(s[k])) {
                    return false;
                }
                k += 1;
            }
            seg_start = i + 1;
        }
        i += 1;
    }
    return true;
}

fn writeBytes(out: []u8, pos: usize, src: []const u8): usize! {
    if (pos + src.len > out.len) {
        return err.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < src.len) {
        out[pos + i] = src[i];
        i += 1;
    }
    return pos + src.len;
}

fn writeByte(out: []u8, pos: usize, b: u8): usize! {
    if (pos >= out.len) {
        return err.BufferTooSmall;
    }
    out[pos] = b;
    return pos + 1;
}

fn writeU32(out: []u8, start: usize, val: u32): usize! {
    if (val == 0) {
        return writeByte(out, start, '0');
    }
    let tmp: [16]u8 = [0; 16];
    let n: usize = 0;
    let v = val;
    while (v > 0) {
        tmp[n] = ('0' as u8) + ((v % 10) as u8);
        v = v / 10;
        n += 1;
    }
    let pos = start;
    while (n > 0) {
        n -= 1;
        pos = writeByte(out, pos, tmp[n])?;
    }
    return pos;
}

fn stripVersionPrefix(raw: []const u8): []const u8 {
    let s = trimWs(raw);
    if (s.len > 0 && (s[0] == 'v' || s[0] == 'V' || s[0] == '=')) {
        s = s[1..];
    }
    return s;
}

/// `pm version` 支持的版本递进策略（对应上游 `PmVersionType`）
pub enum PmVersionType: u8 {
    patch = 0,
    minor = 1,
    major = 2,
    prepatch = 3,
    preminor = 4,
    premajor = 5,
    prerelease = 6,
    specific = 7,
    from_git = 8,

    /// 从 CLI 参数字符串解析关键字策略（不含具体版本号）
    static fn fromString(str: []const u8): PmVersionType? {
        if (bytesEql(str, "patch")) return .patch;
        if (bytesEql(str, "minor")) return .minor;
        if (bytesEql(str, "major")) return .major;
        if (bytesEql(str, "prepatch")) return .prepatch;
        if (bytesEql(str, "preminor")) return .preminor;
        if (bytesEql(str, "premajor")) return .premajor;
        if (bytesEql(str, "prerelease")) return .prerelease;
        if (bytesEql(str, "from-git")) return .from_git;
        return null;
    }

    /// 返回规范关键字名
    fn asKeyword(): []const u8 {
        if (_ == .patch) return "patch";
        if (_ == .minor) return "minor";
        if (_ == .major) return "major";
        if (_ == .prepatch) return "prepatch";
        if (_ == .preminor) return "preminor";
        if (_ == .premajor) return "premajor";
        if (_ == .prerelease) return "prerelease";
        if (_ == .from_git) return "from-git";
        return "specific";
    }
}

/// 解析后的 Semver 2.0 版本结构（支持剥离可选前导 `v`/`=`）
pub struct PmSemver {
    major: u32,
    minor: u32,
    patch: u32,
    prerelease: []const u8,
    build: []const u8,

    /// 严格解析 Semver 2.0 字符串（如 `1.2.3`、`v1.2.3-beta.0+sha.1`）
    static fn parse(input: []const u8): PmSemver! {
        const s = stripVersionPrefix(input);
        if (s.len == 0) {
            return err.InvalidSemver;
        }

        // 剥离 +build
        let main_and_pre = s;
        let build: []const u8 = "";
        if (plus_idx := indexOfChar(s, '+')) {
            main_and_pre = s[0..plus_idx];
            build = s[plus_idx + 1..];
            if (!isValidDotSeparatedIdents(build)) {
                return err.InvalidSemver;
            }
        }

        // 剥离 -prerelease
        let core = main_and_pre;
        let prerelease: []const u8 = "";
        if (dash_idx := indexOfChar(main_and_pre, '-')) {
            core = main_and_pre[0..dash_idx];
            prerelease = main_and_pre[dash_idx + 1..];
            if (!isValidDotSeparatedIdents(prerelease)) {
                return err.InvalidSemver;
            }
        }

        // 解析 major.minor.patch
        const dot1_opt = indexOfChar(core, '.');
        if (dot1 := dot1_opt) {
            const rest1 = core[dot1 + 1..];
            const dot2_opt = indexOfChar(rest1, '.');
            if (dot2 := dot2_opt) {
                const major_s = core[0..dot1];
                const minor_s = rest1[0..dot2];
                const patch_s = rest1[dot2 + 1..];
                if (indexOfChar(patch_s, '.') != null) {
                    return err.InvalidSemver;
                }
                const maj_opt = parseStrictU32(major_s);
                const min_opt = parseStrictU32(minor_s);
                const pat_opt = parseStrictU32(patch_s);
                if (maj := maj_opt) {
                    if (min := min_opt) {
                        if (pat := pat_opt) {
                            return PmSemver{
                                major: maj,
                                minor: min,
                                patch: pat,
                                prerelease: prerelease,
                                build: build,
                            };
                        }
                    }
                }
            }
        }
        return err.InvalidSemver;
    }

    /// 是否包含预发布标识
    fn hasPrerelease(): bool {
        return _.prerelease.len > 0;
    }

    /// 比较核心三段与预发布标识是否相等（忽略 build metadata，符合 Semver 2.0 优先级语义）
    fn eqlCoreAndPre(other: PmSemver): bool {
        return _.major == other.major &&
            _.minor == other.minor &&
            _.patch == other.patch &&
            bytesEql(_.prerelease, other.prerelease);
    }

    /// 格式化为规范 Semver 字符串（不含前导 `v`）
    fn format(out: []u8): usize! {
        let pos = writeU32(out, 0, _.major)?;
        pos = writeByte(out, pos, '.')?;
        pos = writeU32(out, pos, _.minor)?;
        pos = writeByte(out, pos, '.')?;
        pos = writeU32(out, pos, _.patch)?;
        if (_.prerelease.len > 0) {
            pos = writeByte(out, pos, '-')?;
            pos = writeBytes(out, pos, _.prerelease)?;
        }
        if (_.build.len > 0) {
            pos = writeByte(out, pos, '+')?;
            pos = writeBytes(out, pos, _.build)?;
        }
        return pos;
    }
}

/// 解析后的 `pm version <arg>` 参数结构（对应上游 `ParsedVersionArg`）
pub struct PmParsedVersionArg {
    version_type: PmVersionType,
    specific_version: []const u8,

    /// 解析版本参数（关键字或合法 Semver 字符串）
    static fn parse(arg: []const u8): PmParsedVersionArg! {
        const trimmed = trimWs(arg);
        if (trimmed.len == 0) {
            return err.InvalidArgument;
        }
        if (vtype := PmVersionType.fromString(trimmed)) {
            return PmParsedVersionArg{
                version_type: vtype,
                specific_version: "",
            };
        }
        const cleaned = stripVersionPrefix(trimmed);
        * = PmSemver.parse(cleaned) catch return err.InvalidArgument;
        return PmParsedVersionArg{
            version_type: .specific,
            specific_version: cleaned,
        };
    }
}

/// `pm version` 命令选项（对应上游 `--preid` / `--allow-same-version` / `--no-git-tag-version` / `--force` / `-m`）
pub struct PmVersionOptions {
    preid: []const u8,
    allow_same_version: bool,
    git_tag_version: bool,
    force: bool,
    message: []const u8,

    /// 返回默认选项（开启 git tag、禁止同版本重复）
    static fn defaults(): PmVersionOptions {
        return PmVersionOptions{
            preid: "",
            allow_same_version: false,
            git_tag_version: true,
            force: false,
            message: "",
        };
    }
}

/// 解析 `pm version <arg>` 位置参数
pub fn parsePmVersionArg(arg: []const u8): PmParsedVersionArg! {
    return PmParsedVersionArg.parse(arg);
}

/// 规范化 `git describe --tags` 输出的标签为纯净 Semver 切片（剥离前导 `v`/`V`/`=` 并校验合法性）
pub fn normalizeGitTagVersion(raw_tag: []const u8): []const u8! {
    const cleaned = stripVersionPrefix(raw_tag);
    * = PmSemver.parse(cleaned)?;
    return cleaned;
}

/// 解析有效预发布标识符（忠实复刻上游 `calculateNewVersion` 的 `prerelease_id` 推导规则）
fn resolvePrereleaseId(current: PmSemver, opt_preid: []const u8): []const u8! {
    if (opt_preid.len > 0) {
        if (!isValidDotSeparatedIdents(opt_preid)) {
            return err.InvalidPreid;
        }
        return opt_preid;
    }
    if (!current.hasPrerelease()) {
        return "";
    }
    const cur_pre = current.prerelease;
    if (dot_idx := indexOfChar(cur_pre, '.')) {
        return cur_pre[0..dot_idx];
    }
    if (parseStrictU32(cur_pre) != null) {
        return "";
    }
    return cur_pre;
}

fn formatCoreWithPre(out: []u8, major: u32, minor: u32, patch: u32, preid: []const u8, pre_num: u32): usize! {
    let pos = writeU32(out, 0, major)?;
    pos = writeByte(out, pos, '.')?;
    pos = writeU32(out, pos, minor)?;
    pos = writeByte(out, pos, '.')?;
    pos = writeU32(out, pos, patch)?;
    pos = writeByte(out, pos, '-')?;
    if (preid.len > 0) {
        pos = writeBytes(out, pos, preid)?;
        pos = writeByte(out, pos, '.')?;
    }
    pos = writeU32(out, pos, pre_num)?;
    return pos;
}

/// 根据当前版本号、目标递进策略与选项计算新版本号并写入 `out`（对应上游 `calculateNewVersion` + `incrementVersion`）
pub fn computeNextPmVersion(current_str: []const u8, arg: PmParsedVersionArg, opts: PmVersionOptions, out: []u8): usize! {
    if (arg.version_type == .from_git) {
        return err.FromGitRequiresTag;
    }

    const current = PmSemver.parse(current_str)?;

    if (arg.version_type == .specific) {
        const target = PmSemver.parse(arg.specific_version)?;
        if (!opts.allow_same_version && current.eqlCoreAndPre(target)) {
            return err.SameVersion;
        }
        return target.format(out);
    }

    const preid = resolvePrereleaseId(current, opts.preid)?;

    if (arg.version_type == .patch) {
        if (current.patch == 0xFFFF_FFFF) return err.Overflow;
        const next = PmSemver{ major: current.major, minor: current.minor, patch: current.patch + 1, prerelease: "", build: "" };
        return next.format(out);
    }
    if (arg.version_type == .minor) {
        if (current.minor == 0xFFFF_FFFF) return err.Overflow;
        const next = PmSemver{ major: current.major, minor: current.minor + 1, patch: 0, prerelease: "", build: "" };
        return next.format(out);
    }
    if (arg.version_type == .major) {
        if (current.major == 0xFFFF_FFFF) return err.Overflow;
        const next = PmSemver{ major: current.major + 1, minor: 0, patch: 0, prerelease: "", build: "" };
        return next.format(out);
    }
    if (arg.version_type == .prepatch) {
        if (current.patch == 0xFFFF_FFFF) return err.Overflow;
        return formatCoreWithPre(out, current.major, current.minor, current.patch + 1, preid, 0);
    }
    if (arg.version_type == .preminor) {
        if (current.minor == 0xFFFF_FFFF) return err.Overflow;
        return formatCoreWithPre(out, current.major, current.minor + 1, 0, preid, 0);
    }
    if (arg.version_type == .premajor) {
        if (current.major == 0xFFFF_FFFF) return err.Overflow;
        return formatCoreWithPre(out, current.major + 1, 0, 0, preid, 0);
    }

    // .prerelease 分支（严格对齐上游 Bun incrementVersion 行为）
    if (current.hasPrerelease()) {
        const cur_pre = current.prerelease;
        const identifier = if (preid.len > 0) preid else cur_pre;

        if (dot_idx := lastIndexOfChar(cur_pre, '.')) {
            const num_str = cur_pre[dot_idx + 1..];
            const parsed_num = parseStrictU32(num_str) ?? 0;
            if (parsed_num == 0xFFFF_FFFF) return err.Overflow;
            return formatCoreWithPre(out, current.major, current.minor, current.patch, identifier, parsed_num + 1);
        }

        if (n := parseStrictU32(cur_pre)) {
            if (n == 0xFFFF_FFFF) return err.Overflow;
            return formatCoreWithPre(out, current.major, current.minor, current.patch, preid, n + 1);
        }
        return formatCoreWithPre(out, current.major, current.minor, current.patch, identifier, 1);
    }

    if (current.patch == 0xFFFF_FFFF) return err.Overflow;
    return formatCoreWithPre(out, current.major, current.minor, current.patch + 1, preid, 0);
}

/// 格式化 Git 提交消息：将模板中所有 `%s` 替换为 `version`；空模板默认生成 `v<version>`
pub fn formatPmCommitMessage(template: []const u8, version: []const u8, out: []u8): usize! {
    if (template.len == 0) {
        return formatPmGitTag(version, out);
    }
    let pos: usize = 0;
    let i: usize = 0;
    while (i < template.len) {
        if (i + 1 < template.len && template[i] == '%' && template[i + 1] == 's') {
            pos = writeBytes(out, pos, version)?;
            i += 2;
        } else {
            pos = writeByte(out, pos, template[i])?;
            i += 1;
        }
    }
    return pos;
}

/// 格式化标准 Git 标签名 `v<version>`（若传入已带前导 `v` 则不重复叠加）
pub fn formatPmGitTag(version: []const u8, out: []u8): usize! {
    const cleaned = stripVersionPrefix(version);
    if (cleaned.len == 0) {
        return err.InvalidSemver;
    }
    let pos = writeByte(out, 0, 'v')?;
    pos = writeBytes(out, pos, cleaned)?;
    return pos;
}
