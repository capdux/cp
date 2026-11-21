// Source: Absorbed from /data/dev/bun/src/cli.zig (Git Unified Diff Numstat & File Summary Parser)

/// Git Diff 文件变更类型枚举
pub enum FileChangeKind: u8 {
    modified = 0,
    added = 1,
    deleted = 2,
    renamed = 3,
    binary = 4,

    fn asStr(): string {
        return switch (_) {
            .modified => "modified",
            .added => "added",
            .deleted => "deleted",
            .renamed => "renamed",
            .binary => "binary",
        };
    }
}

/// 单个文件的 Git Unified Diff 统计结果（零拷贝路径切片）
pub struct FileDiffStat {
    old_path: []const u8 = "",
    new_path: []const u8 = "",
    kind: FileChangeKind = .modified,
    additions: usize = 0,
    deletions: usize = 0,
    hunks: usize = 0,
    is_binary: bool = false,
}

/// 整份 Git Diff 的汇总统计
pub struct DiffSummary {
    files_changed: usize = 0,
    insertions: usize = 0,
    deletions: usize = 0,
    binary_files: usize = 0,
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

struct PathPair {
    old_path: []const u8,
    new_path: []const u8,
}

fn parseDiffGitHeader(line: []const u8): PathPair {
    // 格式: "diff --git a/<old> b/<new>"
    const rest = line[11..line.len];
    let sep_idx: usize? = null;
    let i: usize = 0;
    while (i + 3 <= rest.len) {
        if (rest[i] == ' ' && rest[i + 1] == 'b' && rest[i + 2] == '/') {
            sep_idx = i;
            break;
        }
        i += 1;
    }

    if (si := sep_idx) {
        let p1 = rest[0..si];
        if (startsWith(p1, "a/")) {
            p1 = p1[2..p1.len];
        }
        const p2 = rest[(si + 3)..rest.len];
        return PathPair{ old_path: p1, new_path: p2 };
    }

    // 兜底：按首个空格拆分
    let sp_idx: usize? = null;
    let k: usize = 0;
    while (k < rest.len) {
        if (rest[k] == ' ') {
            sp_idx = k;
            break;
        }
        k += 1;
    }
    if (sp := sp_idx) {
        return PathPair{
            old_path: rest[0..sp],
            new_path: rest[(sp + 1)..rest.len],
        };
    }
    return PathPair{ old_path: rest, new_path: rest };
}

struct LineSlice {
    text: []const u8,
    next_pos: usize,
}

fn peekLine(src: []const u8, pos: usize): LineSlice? {
    if (pos >= src.len) { return null; }
    let i = pos;
    while (i < src.len && src[i] != '\n') {
        i += 1;
    }
    let end = i;
    if (end > pos && src[end - 1] == '\r') {
        end -= 1;
    }
    const next_pos = if (i < src.len && src[i] == '\n') i + 1 else i;
    return LineSlice{
        text: src[pos..end],
        next_pos: next_pos,
    };
}

/// 零堆分配 Git Unified Diff 文件块统计迭代器
pub struct GitDiffIter {
    src: []const u8,
    pos: usize = 0,

    static fn init(diff_text: []const u8): GitDiffIter {
        return GitDiffIter{ src: diff_text, pos: 0 };
    }

    /// 迭代下一个 `diff --git` 文件块的变更统计
    fn next(*_): FileDiffStat? {
        // 定位下一条 "diff --git " 头行（自动跳过提交日志等前置文本）
        let header_line: []const u8 = "";
        let found = false;
        while (ls := peekLine(_.src, _.pos)) {
            _.pos = ls.next_pos;
            if (startsWith(ls.text, "diff --git ")) {
                header_line = ls.text;
                found = true;
                break;
            }
        }
        if (!found) {
            return null;
        }

        const paths = parseDiffGitHeader(header_line);
        let stat = FileDiffStat{
            old_path: paths.old_path,
            new_path: paths.new_path,
            kind: .modified,
        };
        let in_hunk = false;

        while (ls := peekLine(_.src, _.pos)) {
            if (startsWith(ls.text, "diff --git ")) {
                // 留给下一次 next() 处理
                break;
            }
            _.pos = ls.next_pos;
            const line = ls.text;

            if (!in_hunk) {
                if (startsWith(line, "new file mode ")) {
                    stat.kind = .added;
                } else if (startsWith(line, "deleted file mode ")) {
                    stat.kind = .deleted;
                } else if (startsWith(line, "rename from ")) {
                    stat.old_path = line[12..line.len];
                    stat.kind = .renamed;
                } else if (startsWith(line, "rename to ")) {
                    stat.new_path = line[10..line.len];
                    stat.kind = .renamed;
                } else if (startsWith(line, "Binary files ") || eqlBytes(line, "GIT binary patch")) {
                    stat.is_binary = true;
                    stat.kind = .binary;
                } else if (startsWith(line, "@@ ")) {
                    in_hunk = true;
                    stat.hunks += 1;
                }
            } else {
                if (startsWith(line, "@@ ")) {
                    stat.hunks += 1;
                } else if (startsWith(line, "\\ No newline at end of file")) {
                    continue;
                } else if (line.len > 0 && line[0] == '+') {
                    stat.additions += 1;
                } else if (line.len > 0 && line[0] == '-') {
                    stat.deletions += 1;
                } else if (startsWith(line, "Binary files ") || eqlBytes(line, "GIT binary patch")) {
                    stat.is_binary = true;
                    stat.kind = .binary;
                    in_hunk = false;
                }
            }
        }

        return stat;
    }
}

/// 汇总整份 Git Diff 的变更文件数、总新增行数、总删除行数与二进制文件数
pub fn summarizeGitDiff(diff_text: []const u8): DiffSummary {
    let summary = DiffSummary{};
    let it = GitDiffIter.init(diff_text);
    while (stat := it.next()) {
        summary.files_changed += 1;
        summary.insertions += stat.additions;
        summary.deletions += stat.deletions;
        if (stat.is_binary) {
            summary.binary_files += 1;
        }
    }
    return summary;
}
