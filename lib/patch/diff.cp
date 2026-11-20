// Source: /data/dev/bun/src/patch/patch.cp
// lib/patch/diff.cp
// Full Unified Diff Parser, Multi-File AST, Hunk Applicator (Exact + Fuzz/Offset) & Diff Generator
// Absorbed from Bun src/patch/patch.cp

const std = @import("std");
import { DiffOpKind, DiffOp, LineDiffer } from "./myers.cp";

pub struct HunkRange {
    start_line: usize = 1,
    line_count: usize = 1,
}

pub struct UnifiedHunkHeader {
    old_range: HunkRange = HunkRange {},
    new_range: HunkRange = HunkRange {},
    section: []const u8 = "",

    /// Parses a unified diff hunk header line like `@@ -10,5 +12,7 @@ fn update()`.
    pub static fn parse(line: []const u8): UnifiedHunkHeader? {
        const trimmed = std.mem.trim(u8, line, " \t\r\n");
        if (!std.mem.startsWith(u8, trimmed, "@@ ")) {
            return null;
        }
        const close_opt = std.mem.indexOfPos(u8, trimmed, 3, " @@");
        if (close_opt == null) {
            return null;
        }
        const close_idx = close_opt.?;
        const body = std.mem.trim(u8, trimmed[3..close_idx], " \t");
        const space_opt = std.mem.indexOfScalar(u8, body, ' ');
        if (space_opt == null) {
            return null;
        }
        const old_part = body[0..space_opt.?];
        const new_part = std.mem.trimStart(u8, body[space_opt.? + 1 ..], " ");
        if (old_part.len < 2 || old_part[0] != '-' || new_part.len < 2 || new_part[0] != '+') {
            return null;
        }

        const old_r_opt = UnifiedHunkHeader.parseRangeSpec(old_part[1..]);
        const new_r_opt = UnifiedHunkHeader.parseRangeSpec(new_part[1..]);
        if (old_r_opt == null || new_r_opt == null) {
            return null;
        }

        let sec: []const u8 = "";
        if (close_idx + 3 < trimmed.len) {
            sec = std.mem.trim(u8, trimmed[close_idx + 3 ..], " \t");
        }

        return UnifiedHunkHeader {
            old_range: old_r_opt.?,
            new_range: new_r_opt.?,
            section: sec,
        };
    }

    pub static fn parseRangeSpec(spec: []const u8): HunkRange? {
        if (comma_idx := std.mem.indexOfScalar(u8, spec, ',')) {
            const start_opt = UnifiedHunkHeader.parseUint(spec[0..comma_idx]);
            const count_opt = UnifiedHunkHeader.parseUint(spec[comma_idx + 1 ..]);
            if (start_opt == null || count_opt == null) {
                return null;
            }
            return HunkRange {
                start_line: start_opt.?,
                line_count: count_opt.?,
            };
        }
        if (start_only := UnifiedHunkHeader.parseUint(spec)) {
            return HunkRange {
                start_line: start_only,
                line_count: 1,
            };
        }
        return null;
    }

    pub static fn parseUint(s: []const u8): usize? {
        if (s.len == 0) {
            return null;
        }
        let acc: usize = 0;
        let i: usize = 0;
        while (i < s.len) {
            const c = s[i];
            if (c < '0' || c > '9') {
                return null;
            }
            acc = acc * 10 + ((c - '0') as usize);
            i += 1;
        }
        return acc;
    }
}

pub struct LineDiffStats {
    equal_lines: usize = 0,
    inserted_lines: usize = 0,
    deleted_lines: usize = 0,
    hunk_count: usize = 0,
    file_count: usize = 0,
}

pub enum PatchFileKind: u8 {
    Modify = 0,
    Create = 1,
    Delete = 2,
    Rename = 3,
    ModeChange = 4,
}

pub enum PatchLineKind: u8 {
    Context = 0,
    Insertion = 1,
    Deletion = 2,
}

pub struct PatchLine {
    kind: PatchLineKind = PatchLineKind.Context,
    text: []const u8 = "",
    no_newline_at_eof: bool = false,
}

pub struct PatchHunk {
    header: UnifiedHunkHeader = UnifiedHunkHeader {},
    lines: std.ArrayListUnmanaged(PatchLine) = .empty,
    old_no_newline: bool = false,
    new_no_newline: bool = false,

    pub fn deinit(*_, allocator: std.mem.Allocator) {
        _.lines.deinit(allocator);
    }

    pub fn verifyIntegrity(): bool {
        let old_len: usize = 0;
        let new_len: usize = 0;
        for (ln of _.lines.items) {
            switch (ln.kind) {
                PatchLineKind.Context => {
                    old_len += 1;
                    new_len += 1;
                },
                PatchLineKind.Deletion => {
                    old_len += 1;
                },
                PatchLineKind.Insertion => {
                    new_len += 1;
                },
            }
        }
        return old_len == _.header.old_range.line_count && new_len == _.header.new_range.line_count;
    }
}

pub struct PatchFileEntry {
    kind: PatchFileKind = PatchFileKind.Modify,
    old_path: []const u8 = "",
    new_path: []const u8 = "",
    old_mode: []const u8 = "",
    new_mode: []const u8 = "",
    before_hash: []const u8 = "",
    after_hash: []const u8 = "",
    is_new_file: bool = false,
    is_deleted_file: bool = false,
    is_rename: bool = false,
    hunks: std.ArrayListUnmanaged(PatchHunk) = .empty,

    pub fn targetPath(): []const u8 {
        if (_.new_path.len > 0) return _.new_path;
        return _.old_path;
    }

    pub fn deinit(*_, allocator: std.mem.Allocator) {
        let i: usize = 0;
        while (i < _.hunks.items.len) {
            _.hunks.items[i].deinit(allocator);
            i += 1;
        }
        _.hunks.deinit(allocator);
    }
}

pub struct PatchDocument {
    files: std.ArrayListUnmanaged(PatchFileEntry) = .empty,

    pub fn deinit(*_, allocator: std.mem.Allocator) {
        let i: usize = 0;
        while (i < _.files.items.len) {
            _.files.items[i].deinit(allocator);
            i += 1;
        }
        _.files.deinit(allocator);
    }

    pub fn fileCount(): usize {
        return _.files.items.len;
    }

    pub fn totalHunkCount(): usize {
        let total: usize = 0;
        for (f of _.files.items) {
            total += f.hunks.items.len;
        }
        return total;
    }
}

pub enum PatchErrorKind: u8 {
    None = 0,
    InvalidHeader = 1,
    IntegrityCheckFailed = 2,
    HunkLineBeforeHeader = 3,
    UnrecognizedPragma = 4,
    HunkMismatch = 5,
    OutOfBounds = 6,
    EmptyPatch = 7,
}

pub struct PatchApplyOptions {
    max_offset: usize = 64,
    max_fuzz: usize = 2,
}

pub struct PatchApplyResult {
    ok: bool = false,
    text: []u8 = "",
    applied_hunks: usize = 0,
    exact_hunks: usize = 0,
    offset_hunks: usize = 0,
    fuzz_hunks: usize = 0,
    error_kind: PatchErrorKind = PatchErrorKind.None,
    failed_hunk_index: usize = 0,
    expected_line: usize = 0,
    applied_offset: isize = 0,
    error_message: []const u8 = "",
}

fn stripHeaderPath(raw: []const u8, prefix_to_strip: []const u8): []const u8 {
    let s = raw;
    // Strip optional timestamp after '\t'
    if (tab_idx := std.mem.indexOfScalar(u8, s, '\t')) {
        s = s[0..tab_idx];
    }
    s = std.mem.trim(u8, s, " \t\r\n");
    if (std.mem.eql(u8, s, "/dev/null")) {
        return "/dev/null";
    }
    if (std.mem.startsWith(u8, s, prefix_to_strip)) {
        return s[prefix_to_strip.len..];
    }
    return s;
}

struct DiffGitPaths {
    a: []const u8,
    b: []const u8,
}

fn parseDiffGitLine(line: []const u8): DiffGitPaths? {
    const prefix = "diff --git a/";
    if (!std.mem.startsWith(u8, line, prefix)) return null;
    const rest = line[prefix.len..];
    if (rest.len == 0) return null;

    let i: usize = 0;
    while (i < rest.len) {
        if (rest[i] == 'b' && i > 0 && rest[i - 1] == ' ' && i + 1 < rest.len && rest[i + 1] == '/') {
            return DiffGitPaths {
                a: rest[0 .. i - 1],
                b: std.mem.trim(u8, rest[i + 2 ..], " \t\r\n"),
            };
        }
        i += 1;
    }
    return null;
}

fn linesMatchSlice(
    file_lines: []const []const u8,
    pos: usize,
    seq: []const []const u8,
): bool {
    if (pos + seq.len > file_lines.len) return false;
    let i: usize = 0;
    while (i < seq.len) {
        if (!std.mem.eql(u8, file_lines[pos + i], seq[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

pub struct UnifiedDiff {
    /// Scans a unified diff text and tallies file count, hunk count, inserted (`+`), deleted (`-`), and context (` `) lines.
    pub static fn analyzePatch(patch_text: []const u8): LineDiffStats {
        let stats = LineDiffStats {};
        let line_iter = std.mem.splitScalar(u8, patch_text, '\n');
        let in_hunk: bool = false;

        while (raw_line := line_iter.next()) {
            const line = std.mem.trimEnd(u8, raw_line, "\r");
            if (line.len == 0) {
                continue;
            }
            if (std.mem.startsWith(u8, line, "diff --git ")) {
                stats.file_count += 1;
                in_hunk = false;
                continue;
            }
            if (std.mem.startsWith(u8, line, "--- ")) {
                if (stats.file_count == 0 || in_hunk) {
                    stats.file_count += 1;
                    in_hunk = false;
                }
                continue;
            }
            if (std.mem.startsWith(u8, line, "+++ ")) {
                continue;
            }
            if (std.mem.startsWith(u8, line, "@@ ")) {
                if (UnifiedHunkHeader.parse(line) != null) {
                    stats.hunk_count += 1;
                    in_hunk = true;
                }
                continue;
            }
            if (!in_hunk) {
                continue;
            }
            if (line[0] == '+') {
                stats.inserted_lines += 1;
            } else if (line[0] == '-') {
                stats.deleted_lines += 1;
            } else if (line[0] == ' ') {
                stats.equal_lines += 1;
            }
        }

        return stats;
    }

    /// Parses a complete multi-file, multi-hunk Unified Diff into a `PatchDocument`.
    pub static fn parse(allocator: std.mem.Allocator, patch_text: []const u8): PatchDocument! {
        let doc = PatchDocument {};
        errdefer doc.deinit(allocator);

        if (patch_text.len == 0) {
            return doc;
        }

        let cur_file = PatchFileEntry {};
        let has_file = false;
        errdefer cur_file.deinit(allocator);

        let cur_hunk: PatchHunk? = null;
        errdefer {
            if (ch := cur_hunk) {
                let mut_h = ch;
                mut_h.deinit(allocator);
            }
        }

        let in_hunks = false;
        let line_iter = std.mem.splitScalar(u8, patch_text, '\n');

        while (raw_line := line_iter.next()) {
            // Skip trailing empty segment after final newline
            if (raw_line.len == 0 && line_iter.peek() == null) {
                break;
            }
            const line = std.mem.trimEnd(u8, raw_line, "\r");

            // Check for new file header boundary
            if (std.mem.startsWith(u8, line, "diff --git ")) {
                if (ch := cur_hunk) {
                    if (!ch.verifyIntegrity()) {
                        let mut_h = ch;
                        mut_h.deinit(allocator);
                        cur_hunk = null;
                        return error.IntegrityCheckFailed;
                    }
                    try cur_file.hunks.append(allocator, ch);
                    cur_hunk = null;
                }
                if (has_file) {
                    finalizeFileKind(&cur_file);
                    try doc.files.append(allocator, cur_file);
                    cur_file = PatchFileEntry {};
                }
                has_file = true;
                in_hunks = false;
                if (gp := parseDiffGitLine(line)) {
                    cur_file.old_path = gp.a;
                    cur_file.new_path = gp.b;
                }
                continue;
            }

            if (in_hunks && std.mem.startsWith(u8, line, "--- ")) {
                // Multi-file legacy unified diff without "diff --git"
                if (ch := cur_hunk) {
                    if (!ch.verifyIntegrity()) {
                        let mut_h = ch;
                        mut_h.deinit(allocator);
                        cur_hunk = null;
                        return error.IntegrityCheckFailed;
                    }
                    try cur_file.hunks.append(allocator, ch);
                    cur_hunk = null;
                }
                if (has_file) {
                    finalizeFileKind(&cur_file);
                    try doc.files.append(allocator, cur_file);
                    cur_file = PatchFileEntry {};
                }
                has_file = true;
                in_hunks = false;
                const p_old = stripHeaderPath(line[4..], "a/");
                cur_file.old_path = p_old;
                if (std.mem.eql(u8, p_old, "/dev/null")) {
                    cur_file.is_new_file = true;
                }
                continue;
            }

            if (!in_hunks) {
                if (std.mem.startsWith(u8, line, "@@ ")) {
                    const hdr = UnifiedHunkHeader.parse(line) orelse return error.InvalidHeader;
                    has_file = true;
                    in_hunks = true;
                    cur_hunk = PatchHunk { header: hdr };
                    continue;
                }
                if (std.mem.startsWith(u8, line, "old mode ")) {
                    has_file = true;
                    cur_file.old_mode = std.mem.trim(u8, line["old mode ".len..], " \t");
                    continue;
                }
                if (std.mem.startsWith(u8, line, "new mode ")) {
                    has_file = true;
                    cur_file.new_mode = std.mem.trim(u8, line["new mode ".len..], " \t");
                    continue;
                }
                if (std.mem.startsWith(u8, line, "deleted file mode ")) {
                    has_file = true;
                    cur_file.is_deleted_file = true;
                    cur_file.old_mode = std.mem.trim(u8, line["deleted file mode ".len..], " \t");
                    continue;
                }
                if (std.mem.startsWith(u8, line, "new file mode ")) {
                    has_file = true;
                    cur_file.is_new_file = true;
                    cur_file.new_mode = std.mem.trim(u8, line["new file mode ".len..], " \t");
                    continue;
                }
                if (std.mem.startsWith(u8, line, "rename from ")) {
                    has_file = true;
                    cur_file.is_rename = true;
                    cur_file.old_path = std.mem.trim(u8, line["rename from ".len..], " \t");
                    continue;
                }
                if (std.mem.startsWith(u8, line, "rename to ")) {
                    has_file = true;
                    cur_file.is_rename = true;
                    cur_file.new_path = std.mem.trim(u8, line["rename to ".len..], " \t");
                    continue;
                }
                if (std.mem.startsWith(u8, line, "index ")) {
                    has_file = true;
                    const rest = line["index ".len..];
                    if (dots := std.mem.indexOf(u8, rest, "..")) {
                        cur_file.before_hash = rest[0..dots];
                        const after_start = dots + 2;
                        let after_end = after_start;
                        while (after_end < rest.len && rest[after_end] != ' ') {
                            after_end += 1;
                        }
                        cur_file.after_hash = rest[after_start..after_end];
                    }
                    continue;
                }
                if (std.mem.startsWith(u8, line, "--- ")) {
                    has_file = true;
                    const p_old = stripHeaderPath(line[4..], "a/");
                    cur_file.old_path = p_old;
                    if (std.mem.eql(u8, p_old, "/dev/null")) {
                        cur_file.is_new_file = true;
                    }
                    continue;
                }
                if (std.mem.startsWith(u8, line, "+++ ")) {
                    has_file = true;
                    const p_new = stripHeaderPath(line[4..], "b/");
                    cur_file.new_path = p_new;
                    if (std.mem.eql(u8, p_new, "/dev/null")) {
                        cur_file.is_deleted_file = true;
                    }
                    continue;
                }
                continue;
            }

            // Inside hunks
            if (std.mem.startsWith(u8, line, "@@ ")) {
                if (ch := cur_hunk) {
                    if (!ch.verifyIntegrity()) {
                        let mut_h = ch;
                        mut_h.deinit(allocator);
                        cur_hunk = null;
                        return error.IntegrityCheckFailed;
                    }
                    try cur_file.hunks.append(allocator, ch);
                    cur_hunk = null;
                }
                const hdr = UnifiedHunkHeader.parse(line) orelse return error.InvalidHeader;
                cur_hunk = PatchHunk { header: hdr };
                continue;
            }

            if (line.len > 0 && line[0] == '\\') {
                if (!std.mem.startsWith(u8, line, "\\ No newline at end of file")) {
                    return error.UnrecognizedPragma;
                }
                if (cur_hunk != null) {
                    let ch = cur_hunk.?;
                    if (ch.lines.items.len > 0) {
                        const last_i = ch.lines.items.len - 1;
                        ch.lines.items[last_i].no_newline_at_eof = true;
                        const k = ch.lines.items[last_i].kind;
                        if (k == PatchLineKind.Context) {
                            ch.old_no_newline = true;
                            ch.new_no_newline = true;
                        } else if (k == PatchLineKind.Deletion) {
                            ch.old_no_newline = true;
                        } else if (k == PatchLineKind.Insertion) {
                            ch.new_no_newline = true;
                        }
                    }
                    cur_hunk = ch;
                }
                continue;
            }

            if (cur_hunk == null) {
                return error.HunkLineBeforeHeader;
            }

            let ch = cur_hunk.?;
            // Check if current hunk already satisfied both counts -> could be trailing blank/next file header
            let cur_old: usize = 0;
            let cur_new: usize = 0;
            for (ln of ch.lines.items) {
                if (ln.kind != PatchLineKind.Insertion) cur_old += 1;
                if (ln.kind != PatchLineKind.Deletion) cur_new += 1;
            }

            if (line.len == 0) {
                if (cur_old < ch.header.old_range.line_count || cur_new < ch.header.new_range.line_count) {
                    try ch.lines.append(allocator, PatchLine { kind: PatchLineKind.Context, text: "" });
                    cur_hunk = ch;
                }
                continue;
            }

            const tag = line[0];
            if (tag == ' ') {
                try ch.lines.append(allocator, PatchLine { kind: PatchLineKind.Context, text: line[1..] });
                cur_hunk = ch;
            } else if (tag == '-') {
                try ch.lines.append(allocator, PatchLine { kind: PatchLineKind.Deletion, text: line[1..] });
                cur_hunk = ch;
            } else if (tag == '+') {
                try ch.lines.append(allocator, PatchLine { kind: PatchLineKind.Insertion, text: line[1..] });
                cur_hunk = ch;
            } else {
                // Unrecognized line outside hunk body after complete hunk
                if (!ch.verifyIntegrity()) {
                    ch.deinit(allocator);
                    cur_hunk = null;
                    return error.IntegrityCheckFailed;
                }
                try cur_file.hunks.append(allocator, ch);
                cur_hunk = null;
                in_hunks = false;
            }
        }

        if (ch := cur_hunk) {
            if (!ch.verifyIntegrity()) {
                let mut_h = ch;
                mut_h.deinit(allocator);
                cur_hunk = null;
                return error.IntegrityCheckFailed;
            }
            try cur_file.hunks.append(allocator, ch);
            cur_hunk = null;
        }

        if (has_file) {
            finalizeFileKind(&cur_file);
            try doc.files.append(allocator, cur_file);
        }

        return doc;
    }

    static fn finalizeFileKind(entry: *PatchFileEntry) {
        if (entry.is_rename) {
            entry.kind = PatchFileKind.Rename;
        } else if (entry.is_deleted_file) {
            entry.kind = PatchFileKind.Delete;
        } else if (entry.is_new_file) {
            entry.kind = PatchFileKind.Create;
        } else if (entry.hunks.items.len > 0) {
            entry.kind = PatchFileKind.Modify;
        } else if (entry.old_mode.len > 0 || entry.new_mode.len > 0) {
            entry.kind = PatchFileKind.ModeChange;
        }
    }

    /// Applies a parsed `PatchFileEntry` to `original_text` with exact + offset + context fuzz matching.
    pub static fn applyFilePatch(
        allocator: std.mem.Allocator,
        original_text: []const u8,
        file_patch: PatchFileEntry,
        options: PatchApplyOptions,
    ): PatchApplyResult! {
        let res = PatchApplyResult {};

        if (file_patch.is_deleted_file) {
            res.ok = true;
            res.text = try allocator.alloc(u8, 0);
            res.applied_hunks = file_patch.hunks.items.len;
            res.exact_hunks = file_patch.hunks.items.len;
            return res;
        }

        let trailing_nl = false;
        let lines = try LineDiffer.splitLinesAlloc(allocator, original_text, &trailing_nl);
        defer lines.deinit(allocator);

        let cumulative_delta: isize = 0;

        let h_idx: usize = 0;
        while (h_idx < file_patch.hunks.items.len) {
            const hunk = file_patch.hunks.items[h_idx];

            let old_seq: std.ArrayListUnmanaged(string) = .empty;
            defer old_seq.deinit(allocator);
            let new_seq: std.ArrayListUnmanaged(string) = .empty;
            defer new_seq.deinit(allocator);

            let lead_ctx: usize = 0;
            let counting_lead = true;
            let trail_ctx: usize = 0;

            for (ln of hunk.lines.items) {
                if (ln.kind == PatchLineKind.Context) {
                    try old_seq.append(allocator, ln.text);
                    try new_seq.append(allocator, ln.text);
                    if (counting_lead) {
                        lead_ctx += 1;
                    } else {
                        trail_ctx += 1;
                    }
                } else {
                    counting_lead = false;
                    trail_ctx = 0;
                    if (ln.kind == PatchLineKind.Deletion) {
                        try old_seq.append(allocator, ln.text);
                    } else {
                        try new_seq.append(allocator, ln.text);
                    }
                }
            }

            // Compute expected 0-based start line
            const raw_start: isize = if (hunk.header.old_range.start_line == 0) 0 else ((hunk.header.old_range.start_line - 1) as isize);
            let base_pos_i = raw_start + cumulative_delta;
            if (base_pos_i < 0) base_pos_i = 0;
            const base_pos: usize = base_pos_i as usize;

            let matched_pos: usize? = null;
            let matched_fuzz: usize = 0;
            let matched_trim_l: usize = 0;
            let matched_trim_t: usize = 0;
            let matched_offset: isize = 0;

            // Fuzz loop: fuzz_lvl = 0 (exact context) .. options.max_fuzz
            let fuzz_lvl: usize = 0;
            while (fuzz_lvl <= options.max_fuzz) {
                const trim_l = if (fuzz_lvl <= lead_ctx) fuzz_lvl else lead_ctx;
                const trim_t = if (fuzz_lvl <= trail_ctx) fuzz_lvl else trail_ctx;
                if (fuzz_lvl > 0 && trim_l == 0 && trim_t == 0) {
                    fuzz_lvl += 1;
                    continue;
                }
                if (trim_l + trim_t >= old_seq.items.len && old_seq.items.len > 0) {
                    break;
                }
                const sub_old = old_seq.items[trim_l .. old_seq.items.len - trim_t];

                // 1. Check exact position (offset 0)
                if (base_pos + trim_l <= lines.items.len && linesMatchSlice(lines.items, base_pos + trim_l, sub_old)) {
                    matched_pos = base_pos + trim_l;
                    matched_fuzz = fuzz_lvl;
                    matched_trim_l = trim_l;
                    matched_trim_t = trim_t;
                    matched_offset = 0;
                    break;
                }

                // 2. Search outward +-1 .. +-options.max_offset
                let d: usize = 1;
                while (d <= options.max_offset) {
                    if (base_pos + trim_l + d <= lines.items.len && linesMatchSlice(lines.items, base_pos + trim_l + d, sub_old)) {
                        matched_pos = base_pos + trim_l + d;
                        matched_fuzz = fuzz_lvl;
                        matched_trim_l = trim_l;
                        matched_trim_t = trim_t;
                        matched_offset = d as isize;
                        break;
                    }
                    if (base_pos + trim_l >= d) {
                        const cand = base_pos + trim_l - d;
                        if (linesMatchSlice(lines.items, cand, sub_old)) {
                            matched_pos = cand;
                            matched_fuzz = fuzz_lvl;
                            matched_trim_l = trim_l;
                            matched_trim_t = trim_t;
                            matched_offset = -(d as isize);
                            break;
                        }
                    }
                    d += 1;
                }

                if (matched_pos != null) break;
                fuzz_lvl += 1;
            }

            if (matched_pos == null) {
                res.ok = false;
                res.error_kind = PatchErrorKind.HunkMismatch;
                res.failed_hunk_index = h_idx;
                res.expected_line = hunk.header.old_range.start_line;
                res.error_message = "Hunk context or deletion lines failed to match target text";
                return res;
            }

            const pos = matched_pos.?;
            const del_count = old_seq.items.len - matched_trim_l - matched_trim_t;
            const ins_slice = new_seq.items[matched_trim_l .. new_seq.items.len - matched_trim_t];

            try lines.replaceRange(allocator, pos, del_count, ins_slice);
            cumulative_delta += (ins_slice.len as isize) - (del_count as isize);

            // Update trailing newline if this hunk touched the end of file
            if (pos + ins_slice.len == lines.items.len) {
                if (hunk.new_no_newline) {
                    trailing_nl = false;
                } else if (file_patch.is_new_file || hunk.old_no_newline || ins_slice.len > 0) {
                    trailing_nl = true;
                }
            }

            res.applied_hunks += 1;
            if (matched_fuzz == 0 && matched_offset == 0) {
                res.exact_hunks += 1;
            } else {
                if (matched_offset != 0) {
                    res.offset_hunks += 1;
                    res.applied_offset = matched_offset;
                }
                if (matched_fuzz > 0) {
                    res.fuzz_hunks += 1;
                }
            }

            h_idx += 1;
        }

        // Reconstruct output text
        let out: std.ArrayListUnmanaged(u8) = .empty;
        errdefer out.deinit(allocator);

        let li: usize = 0;
        while (li < lines.items.len) {
            if (li > 0) {
                try out.append(allocator, '\n');
            }
            try out.appendSlice(allocator, lines.items[li]);
            li += 1;
        }
        if (trailing_nl && lines.items.len > 0) {
            try out.append(allocator, '\n');
        }

        res.ok = true;
        res.text = try out.toOwnedSlice(allocator);
        return res;
    }

    /// Parses `patch_text` and applies the first file's hunks to `original_text` using default options.
    pub static fn apply(allocator: std.mem.Allocator, original_text: []const u8, patch_text: []const u8): PatchApplyResult! {
        return UnifiedDiff.applyWithOptions(allocator, original_text, patch_text, PatchApplyOptions {});
    }

    /// Parses `patch_text` and applies the first file's hunks to `original_text` with custom fuzz/offset options.
    pub static fn applyWithOptions(
        allocator: std.mem.Allocator,
        original_text: []const u8,
        patch_text: []const u8,
        options: PatchApplyOptions,
    ): PatchApplyResult! {
        let doc = UnifiedDiff.parse(allocator, patch_text) catch |e| {
            let err_kind = PatchErrorKind.InvalidHeader;
            if (e == error.IntegrityCheckFailed) err_kind = PatchErrorKind.IntegrityCheckFailed;
            if (e == error.HunkLineBeforeHeader) err_kind = PatchErrorKind.HunkLineBeforeHeader;
            if (e == error.UnrecognizedPragma) err_kind = PatchErrorKind.UnrecognizedPragma;
            return PatchApplyResult {
                ok: false,
                error_kind: err_kind,
                error_message: @errorName(e),
            };
        };
        defer doc.deinit(allocator);

        if (doc.files.items.len == 0) {
            return PatchApplyResult {
                ok: false,
                error_kind: PatchErrorKind.EmptyPatch,
                error_message: "Patch contains no file entries",
            };
        }

        return UnifiedDiff.applyFilePatch(allocator, original_text, doc.files.items[0], options);
    }

    /// Generates a complete standard Unified Diff string from `old_text` and `new_text`.
    pub static fn diffLines(
        allocator: std.mem.Allocator,
        old_text: []const u8,
        new_text: []const u8,
        old_path: []const u8,
        new_path: []const u8,
        context_lines: usize,
    ): []u8! {
        return LineDiffer.generateUnifiedDiff(allocator, old_text, new_text, old_path, new_path, context_lines);
    }

    /// Applies a single hunk body (` `, `-`, `+` lines) to build the patched output into `out_buf`.
    /// Returns the slice of `out_buf` written.
    pub static fn renderHunkNewSide(hunk_lines: []const u8, out_buf: []u8): []const u8 {
        let out_pos: usize = 0;
        let line_iter = std.mem.splitScalar(u8, hunk_lines, '\n');

        while (raw_line := line_iter.next()) {
            const line = std.mem.trimEnd(u8, raw_line, "\r");
            if (line.len == 0 || std.mem.startsWith(u8, line, "@@ ") || std.mem.startsWith(u8, line, "--- ") || std.mem.startsWith(u8, line, "+++ ") || std.mem.startsWith(u8, line, "\\ No newline")) {
                continue;
            }
            if (line[0] == ' ' || line[0] == '+') {
                const payload = line[1..];
                if (out_pos + payload.len + 1 > out_buf.len) {
                    break;
                }
                let k: usize = 0;
                while (k < payload.len) {
                    out_buf[out_pos + k] = payload[k];
                    k += 1;
                }
                out_pos += payload.len;
                out_buf[out_pos] = '\n';
                out_pos += 1;
            }
        }

        return out_buf[0..out_pos];
    }
}
