// Source: Absorbed from /data/dev/bun/src/install/repository.zig | Synced: 2026-11-21T15:09:08Z (Git Refname Validator & Classifier)

/// Git 引用类型枚举
pub enum GitRefKind: u8 {
    branch = 0,
    tag = 1,
    remote = 2,
    symbolic = 3,
    other = 4,

    fn asStr(): string {
        return switch (_) {
            .branch => "branch",
            .tag => "tag",
            .remote => "remote",
            .symbolic => "symbolic",
            .other => "other",
        };
    }
}

/// 拆解后的完整 Git 引用结构（零拷贝切片）
pub struct ParsedGitRef {
    kind: GitRefKind = .other,
    short_name: []const u8 = "",
    remote_name: ([]const u8)? = null,
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

fn startsWith(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) { return false; }
    return eqlBytes(s[0..prefix.len], prefix);
}

fn endsWith(s: []const u8, suffix: []const u8): bool {
    if (s.len < suffix.len) { return false; }
    return eqlBytes(s[(s.len - suffix.len)..s.len], suffix);
}

fn isForbiddenRefChar(c: u8): bool {
    if (c < 0x20 || c == 0x7F) { return true; }
    return c == ' ' || c == '~' || c == '^' || c == ':' ||
        c == '?' || c == '*' || c == '[' || c == '\\';
}

fn isHexDigit(c: u8): bool {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

/// 严格遵循 `git-check-ref-format` 全部 10 条规则校验 Git 引用名合法性
pub fn isValidGitRefName(ref_name: []const u8): bool {
    if (ref_name.len == 0) { return false; }
    if (ref_name.len == 1 && ref_name[0] == '@') { return false; }
    if (ref_name[0] == '/' || ref_name[ref_name.len - 1] == '/') { return false; }
    if (ref_name[ref_name.len - 1] == '.') { return false; }

    let seg_start: usize = 0;
    let i: usize = 0;
    while (i < ref_name.len) {
        const c = ref_name[i];
        if (isForbiddenRefChar(c)) {
            return false;
        }
        if (i + 1 < ref_name.len) {
            const nxt = ref_name[i + 1];
            if (c == '.' && nxt == '.') { return false; }
            if (c == '/' && nxt == '/') { return false; }
            if (c == '@' && nxt == '{') { return false; }
        }
        if (c == '/') {
            const seg = ref_name[seg_start..i];
            if (seg.len == 0 || seg[0] == '.' || endsWith(seg, ".lock")) {
                return false;
            }
            seg_start = i + 1;
        }
        i += 1;
    }

    const last_seg = ref_name[seg_start..ref_name.len];
    if (last_seg.len == 0 || last_seg[0] == '.' || endsWith(last_seg, ".lock")) {
        return false;
    }

    return true;
}

/// 校验 Git 分支名合法性：满足 `isValidGitRefName`、不以 `-` 开头且不等于 `"HEAD"`
pub fn isValidBranchName(branch: []const u8): bool {
    if (!isValidGitRefName(branch)) { return false; }
    if (branch[0] == '-') { return false; }
    if (eqlBytes(branch, "HEAD")) { return false; }
    return true;
}

/// 拆解完整 Git 引用（`refs/heads/`、`refs/tags/`、`refs/remotes/<remote>/<branch>`、`HEAD`/`FETCH_HEAD`）
pub fn parseFullGitRef(full_ref: []const u8): ParsedGitRef? {
    if (!isValidGitRefName(full_ref)) {
        return null;
    }

    if (eqlBytes(full_ref, "HEAD") ||
        eqlBytes(full_ref, "FETCH_HEAD") ||
        eqlBytes(full_ref, "ORIG_HEAD") ||
        eqlBytes(full_ref, "MERGE_HEAD") ||
        eqlBytes(full_ref, "CHERRY_PICK_HEAD"))
    {
        return ParsedGitRef{
            kind: .symbolic,
            short_name: full_ref,
            remote_name: null,
        };
    }

    if (startsWith(full_ref, "refs/heads/")) {
        const rest = full_ref[11..full_ref.len];
        if (rest.len == 0) { return null; }
        return ParsedGitRef{
            kind: .branch,
            short_name: rest,
            remote_name: null,
        };
    }

    if (startsWith(full_ref, "refs/tags/")) {
        const rest = full_ref[10..full_ref.len];
        if (rest.len == 0) { return null; }
        return ParsedGitRef{
            kind: .tag,
            short_name: rest,
            remote_name: null,
        };
    }

    if (startsWith(full_ref, "refs/remotes/")) {
        const rest = full_ref[13..full_ref.len];
        let slash_idx: usize? = null;
        let i: usize = 0;
        while (i < rest.len) {
            if (rest[i] == '/') {
                slash_idx = i;
                break;
            }
            i += 1;
        }
        if (si := slash_idx) {
            if (si == 0 || si + 1 >= rest.len) { return null; }
            return ParsedGitRef{
                kind: .remote,
                short_name: rest[(si + 1)..rest.len],
                remote_name: rest[0..si],
            };
        }
        return null;
    }

    return ParsedGitRef{
        kind: .other,
        short_name: full_ref,
        remote_name: null,
    };
}

/// 校验完整 Git 对象 ID（40 位 SHA-1 或 64 位 SHA-256 十六进制）
pub fn isValidGitOidHex(oid: []const u8): bool {
    if (oid.len != 40 && oid.len != 64) {
        return false;
    }
    let i: usize = 0;
    while (i < oid.len) {
        if (!isHexDigit(oid[i])) { return false; }
        i += 1;
    }
    return true;
}

/// 校验短/完整 Git 对象 ID（4..=64 位十六进制）
pub fn isValidShortOidHex(oid: []const u8): bool {
    if (oid.len < 4 || oid.len > 64) {
        return false;
    }
    let i: usize = 0;
    while (i < oid.len) {
        if (!isHexDigit(oid[i])) { return false; }
        i += 1;
    }
    return true;
}
