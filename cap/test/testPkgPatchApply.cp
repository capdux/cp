import { test, assert, assertEq } from "cap:test";
import {
    PATCH_CONFLICT_CONTEXT_MISMATCH,
    PATCH_CONFLICT_FILE_ALREADY_EXISTS,
    PatchApplyError,
    PatchFileKind,
    PatchLineOpKind,
    PatchLineOp,
    PatchHunk,
    PatchFileEntry,
    PatchParseOptions,
    PatchApplyOptions,
    PatchConflictInfo,
    SourceLineSpan,
    PatchInstallStage,
    PatchInstallSession,
    parseFileModeOctal,
    isSafePatchRelPath,
    sanitizePatchHeaderPath,
    parseHunkHeaderLine,
    parseUnifiedPatch,
    applyFilePatch,
    applyUnifiedPatch,
    reverseUnifiedPatch,
    dryRunPatch,
    formatPatchSummaryReport,
} from "cap:pkg";

fn strEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("Multi-file Git patch parsing: modify, create, delete, rename, mode change & summary", () => {
    const patch =
        "diff --git a/src/app.cp b/src/app.cp\n" ++
        "index 1111111..2222222 100644\n" ++
        "--- a/src/app.cp\n" ++
        "+++ b/src/app.cp\n" ++
        "@@ -1,3 +1,4 @@ fn main()\n" ++
        " line1\n" ++
        "-line2\n" ++
        "+line2_new\n" ++
        "+line2_extra\n" ++
        " line3\n" ++
        "diff --git a/src/new.cp b/src/new.cp\n" ++
        "new file mode 100644\n" ++
        "--- /dev/null\n" ++
        "+++ b/src/new.cp\n" ++
        "@@ -0,0 +1,2 @@\n" ++
        "+hello\n" ++
        "+world\n" ++
        "diff --git a/src/old.cp b/src/old.cp\n" ++
        "deleted file mode 100644\n" ++
        "--- a/src/old.cp\n" ++
        "+++ /dev/null\n" ++
        "@@ -1,2 +0,0 @@\n" ++
        "-bye1\n" ++
        "-bye2\n" ++
        "diff --git a/src/from.cp b/src/to.cp\n" ++
        "similarity index 100%\n" ++
        "rename from src/from.cp\n" ++
        "rename to src/to.cp\n" ++
        "diff --git a/bin/cli.sh b/bin/cli.sh\n" ++
        "old mode 100644\n" ++
        "new mode 100755\n";

    let files: [8]PatchFileEntry = [PatchFileEntry.empty(); 8];
    let hunks: [16]PatchHunk = [PatchHunk.empty(); 16];
    let ops: [64]PatchLineOp = [PatchLineOp.empty(); 64];

    const parsed = parseUnifiedPatch(
        patch,
        PatchParseOptions.defaults(),
        files[0..],
        hunks[0..],
        ops[0..],
    ) catch unreachable;

    assertEq(parsed.fileCount(), @as(usize, 5));
    assertEq(parsed.hunkCount(), @as(usize, 3));
    assertEq(parsed.summary.total_files, @as(u32, 5));
    assertEq(parsed.summary.modified_files, @as(u32, 1));
    assertEq(parsed.summary.created_files, @as(u32, 1));
    assertEq(parsed.summary.deleted_files, @as(u32, 1));
    assertEq(parsed.summary.renamed_files, @as(u32, 1));
    assertEq(parsed.summary.mode_only_files, @as(u32, 1));
    assertEq(parsed.summary.total_added_lines, @as(u32, 4));
    assertEq(parsed.summary.total_deleted_lines, @as(u32, 3));
    assert(parsed.summary.content_hash64 != 0);

    // Check individual file metadata
    assert(files[0].kind == PatchFileKind.Modify);
    assert(strEql(files[0].effectivePath(), "src/app.cp"));
    assert(strEql(hunks[0].header_context, "fn main()"));

    assert(files[1].kind == PatchFileKind.Create);
    assert(strEql(files[1].effectivePath(), "src/new.cp"));
    assertEq(files[1].new_mode, parseFileModeOctal("100644").?);

    assert(files[2].kind == PatchFileKind.Delete);
    assert(strEql(files[2].effectivePath(), "src/old.cp"));

    assert(files[3].kind == PatchFileKind.Rename);
    assert(strEql(files[3].old_path, "src/from.cp"));
    assert(strEql(files[3].new_path, "src/to.cp"));

    assert(files[4].kind == PatchFileKind.ModeChange);
    assert(files[4].hasModeChange());
    assertEq(files[4].old_mode, parseFileModeOctal("100644").?);
    assertEq(files[4].new_mode, parseFileModeOctal("100755").?);

    // Verify summary formatter
    let report_buf: [128]u8 = [0; 128];
    const rlen = formatPatchSummaryReport(parsed.summary, report_buf[0..]) catch unreachable;
    assert(strEql(report_buf[0..rlen], "files=5 (create=1 mod=1 del=1 ren=1) hunks=3 +4 -3"));
});

test("Strict path traversal guards and binary patch rejection/marking", () => {
    assert(isSafePatchRelPath("src/pkg/index.cp"));
    assert(!isSafePatchRelPath(""));
    assert(!isSafePatchRelPath("/etc/passwd"));
    assert(!isSafePatchRelPath("../escape.cp"));
    assert(!isSafePatchRelPath("src/../../escape.cp"));
    assert(!isSafePatchRelPath("src\\win\\path.cp"));
    assert(!isSafePatchRelPath("C:/Windows/win.ini"));
    assert(!isSafePatchRelPath("src/./file.cp"));

    // Sanitizer handles tab timestamps, quotes, /dev/null, -p1 stripping
    const p1 = sanitizePatchHeaderPath("a/src/mod.cp\t2026-11-22 10:00:00", 1) catch unreachable;
    assert(strEql(p1, "src/mod.cp"));
    const p_null = sanitizePatchHeaderPath("/dev/null", 1) catch unreachable;
    assert(strEql(p_null, ""));

    // Path traversal inside patch header rejected
    let trav_err = false;
    * = sanitizePatchHeaderPath("a/../../etc/hosts", 1) catch |e| {
        trav_err = (e == PatchApplyError.UnsafePatchPath);
    };
    assert(trav_err);

    let abs_err = false;
    * = sanitizePatchHeaderPath("/etc/passwd", 1) catch |e| {
        abs_err = (e == PatchApplyError.UnsafePatchPath);
    };
    assert(abs_err);

    // Binary patch rejected by default, marked as Binary when allowed
    const bin_patch =
        "diff --git a/assets/logo.png b/assets/logo.png\n" ++
        "Binary files a/assets/logo.png and b/assets/logo.png differ\n";
    let files: [4]PatchFileEntry = [PatchFileEntry.empty(); 4];
    let hunks: [4]PatchHunk = [PatchHunk.empty(); 4];
    let ops: [8]PatchLineOp = [PatchLineOp.empty(); 8];

    let bin_rejected = false;
    * = parseUnifiedPatch(bin_patch, PatchParseOptions.defaults(), files[0..], hunks[0..], ops[0..]) catch |e| {
        bin_rejected = (e == PatchApplyError.BinaryPatchUnsupported);
    };
    assert(bin_rejected);

    let allow_opts = PatchParseOptions.defaults();
    allow_opts.allow_binary_markers = true;
    const bin_parsed = parseUnifiedPatch(bin_patch, allow_opts, files[0..], hunks[0..], ops[0..]) catch unreachable;
    assertEq(bin_parsed.summary.binary_files, @as(u32, 1));
    assert(files[0].is_binary);
    assert(files[0].kind == PatchFileKind.Binary);
});

test("Hunk header parser handles omitted counts and rejects malformed headers", () => {
    const h1 = parseHunkHeaderLine("@@ -10 +12,3 @@ pub fn foo()", 5) catch unreachable;
    assertEq(h1.old_start, @as(u32, 10));
    assertEq(h1.old_count, @as(u32, 1));
    assertEq(h1.new_start, @as(u32, 12));
    assertEq(h1.new_count, @as(u32, 3));
    assert(strEql(h1.header_context, "pub fn foo()"));
    assertEq(h1.patch_line_no, @as(u32, 5));

    let bad_hdr = false;
    * = parseHunkHeaderLine("@@ -abc +1 @@", 1) catch |e| {
        bad_hdr = (e == PatchApplyError.InvalidHunkHeader);
    };
    assert(bad_hdr);

    // Hunk line count mismatch caught under strict_hunk_counts
    const bad_count_patch =
        "--- a/a.txt\n" ++
        "+++ b/a.txt\n" ++
        "@@ -1,3 +1,3 @@\n" ++
        " only_one_line\n";
    let files: [2]PatchFileEntry = [PatchFileEntry.empty(); 2];
    let hunks: [2]PatchHunk = [PatchHunk.empty(); 2];
    let ops: [8]PatchLineOp = [PatchLineOp.empty(); 8];
    let mismatch = false;
    * = parseUnifiedPatch(bad_count_patch, PatchParseOptions.defaults(), files[0..], hunks[0..], ops[0..]) catch |e| {
        mismatch = (e == PatchApplyError.HunkLineCountMismatch);
    };
    assert(mismatch);
});

test("Forward apply and reverse rollback round-trip across multiple hunks", () => {
    const orig =
        "alpha\n" ++
        "beta\n" ++
        "gamma\n" ++
        "delta\n" ++
        "epsilon\n" ++
        "zeta\n" ++
        "eta\n" ++
        "theta\n";

    const patch =
        "--- a/greek.txt\n" ++
        "+++ b/greek.txt\n" ++
        "@@ -1,3 +1,4 @@\n" ++
        " alpha\n" ++
        "-beta\n" ++
        "+BETA_1\n" ++
        "+BETA_2\n" ++
        " gamma\n" ++
        "@@ -6,3 +7,2 @@\n" ++
        " zeta\n" ++
        "-eta\n" ++
        " theta\n";

    const expected =
        "alpha\n" ++
        "BETA_1\n" ++
        "BETA_2\n" ++
        "gamma\n" ++
        "delta\n" ++
        "epsilon\n" ++
        "zeta\n" ++
        "theta\n";

    let files: [4]PatchFileEntry = [PatchFileEntry.empty(); 4];
    let hunks: [8]PatchHunk = [PatchHunk.empty(); 8];
    let ops: [32]PatchLineOp = [PatchLineOp.empty(); 32];
    let scratch: [32]SourceLineSpan = [SourceLineSpan.empty(); 32];
    let fwd_buf: [256]u8 = [0; 256];
    let rev_buf: [256]u8 = [0; 256];
    let conflict = PatchConflictInfo.none();

    const parsed = parseUnifiedPatch(patch, PatchParseOptions.defaults(), files[0..], hunks[0..], ops[0..]) catch unreachable;

    // Forward apply
    const fwd = applyUnifiedPatch(
        orig,
        files[0],
        hunks[0..parsed.hunks_len],
        ops[0..parsed.ops_len],
        scratch[0..],
        fwd_buf[0..],
        &conflict,
    ) catch unreachable;
    assert(!conflict.has_conflict);
    assertEq(fwd.hunks_applied, @as(u32, 2));
    assertEq(fwd.lines_added, @as(u32, 2));
    assertEq(fwd.lines_deleted, @as(u32, 2));
    assertEq(fwd.max_abs_offset, @as(u32, 0));
    assert(strEql(fwd_buf[0..fwd.output_len], expected));

    // Reverse rollback restores original byte-for-byte
    const rev = reverseUnifiedPatch(
        fwd_buf[0..fwd.output_len],
        files[0],
        hunks[0..parsed.hunks_len],
        ops[0..parsed.ops_len],
        scratch[0..],
        rev_buf[0..],
        &conflict,
    ) catch unreachable;
    assert(!conflict.has_conflict);
    assertEq(rev.hunks_applied, @as(u32, 2));
    assert(strEql(rev_buf[0..rev.output_len], orig));
});

test("Bounded offset-window search locates drifted hunk and dryRunPatch reports exact vs offset", () => {
    // Original file had 3 extra header lines prepended since patch was generated
    const drifted_src =
        "// prepended 1\n" ++
        "// prepended 2\n" ++
        "// prepended 3\n" ++
        "lineA\n" ++
        "lineB\n" ++
        "lineC\n" ++
        "lineD\n";

    const patch =
        "--- a/src/drifted.cp\n" ++
        "+++ b/src/drifted.cp\n" ++
        "@@ -1,3 +1,3 @@\n" ++
        " lineA\n" ++
        "-lineB\n" ++
        "+lineB_patched\n" ++
        " lineC\n";

    let files: [2]PatchFileEntry = [PatchFileEntry.empty(); 2];
    let hunks: [4]PatchHunk = [PatchHunk.empty(); 4];
    let ops: [16]PatchLineOp = [PatchLineOp.empty(); 16];
    let scratch: [32]SourceLineSpan = [SourceLineSpan.empty(); 32];
    let out_buf: [256]u8 = [0; 256];
    let conflict = PatchConflictInfo.none();

    const parsed = parseUnifiedPatch(patch, PatchParseOptions.defaults(), files[0..], hunks[0..], ops[0..]) catch unreachable;

    // Strict exact mode (max_offset_lines=0) must fail with ContextMismatch
    let strict_failed = false;
    * = applyFilePatch(
        drifted_src,
        files[0],
        hunks[0..parsed.hunks_len],
        ops[0..parsed.ops_len],
        PatchApplyOptions.strictExact(),
        scratch[0..],
        out_buf[0..],
        &conflict,
    ) catch |e| {
        strict_failed = (e == PatchApplyError.ContextMismatch);
    };
    assert(strict_failed);
    assert(conflict.has_conflict);
    assertEq(conflict.reason_code, PATCH_CONFLICT_CONTEXT_MISMATCH);
    assertEq(conflict.hunk_index, @as(u32, 0));
    assertEq(conflict.patch_line_no, @as(u32, 3));

    // Dry-run with default offset window succeeds and reports offset match
    const dry = dryRunPatch(
        drifted_src,
        files[0],
        hunks[0..parsed.hunks_len],
        ops[0..parsed.ops_len],
        PatchApplyOptions.forward(),
        scratch[0..],
    );
    assert(dry.can_apply);
    assertEq(dry.hunks_checked, @as(u32, 1));
    assertEq(dry.hunks_matched_exact, @as(u32, 0));
    assertEq(dry.hunks_matched_with_offset, @as(u32, 1));

    // Real apply with offset window succeeds and matches dry-run byte estimate
    const res = applyUnifiedPatch(
        drifted_src,
        files[0],
        hunks[0..parsed.hunks_len],
        ops[0..parsed.ops_len],
        scratch[0..],
        out_buf[0..],
        &conflict,
    ) catch unreachable;
    assertEq(res.max_abs_offset, @as(u32, 3));
    assertEq(res.output_len, dry.estimated_output_bytes);
    const expected =
        "// prepended 1\n" ++
        "// prepended 2\n" ++
        "// prepended 3\n" ++
        "lineA\n" ++
        "lineB_patched\n" ++
        "lineC\n" ++
        "lineD\n";
    assert(strEql(out_buf[0..res.output_len], expected));
});

test("No-newline-at-EOF handling, CRLF preservation, and create-on-existing conflict guard", () => {
    // 1. No trailing newline in new file
    const no_nl_patch =
        "--- /dev/null\n" ++
        "+++ b/token.txt\n" ++
        "@@ -0,0 +1,1 @@\n" ++
        "+secret_value\n" ++
        "\\ No newline at end of file\n";

    let files: [2]PatchFileEntry = [PatchFileEntry.empty(); 2];
    let hunks: [2]PatchHunk = [PatchHunk.empty(); 2];
    let ops: [8]PatchLineOp = [PatchLineOp.empty(); 8];
    let scratch: [16]SourceLineSpan = [SourceLineSpan.empty(); 16];
    let out_buf: [128]u8 = [0; 128];
    let conflict = PatchConflictInfo.none();

    const parsed = parseUnifiedPatch(no_nl_patch, PatchParseOptions.defaults(), files[0..], hunks[0..], ops[0..]) catch unreachable;
    assert(ops[0].no_newline);

    const res = applyUnifiedPatch("", files[0], hunks[0..parsed.hunks_len], ops[0..parsed.ops_len], scratch[0..], out_buf[0..], &conflict) catch unreachable;
    assert(!res.has_trailing_newline);
    assert(strEql(out_buf[0..res.output_len], "secret_value"));

    // Create-file patch against non-empty target fails with FileAlreadyExists
    let exists_err = false;
    * = applyUnifiedPatch("already here\n", files[0], hunks[0..parsed.hunks_len], ops[0..parsed.ops_len], scratch[0..], out_buf[0..], &conflict) catch |e| {
        exists_err = (e == PatchApplyError.FileAlreadyExists);
    };
    assert(exists_err);
    assertEq(conflict.reason_code, PATCH_CONFLICT_FILE_ALREADY_EXISTS);

    // 2. CRLF preservation
    const crlf_src = "first\r\nsecond\r\nthird\r\n";
    const crlf_patch =
        "--- a/win.txt\n" ++
        "+++ b/win.txt\n" ++
        "@@ -1,3 +1,3 @@\n" ++
        " first\n" ++
        "-second\n" ++
        "+SECOND\n" ++
        " third\n";
    const p2 = parseUnifiedPatch(crlf_patch, PatchParseOptions.defaults(), files[0..], hunks[0..], ops[0..]) catch unreachable;
    const r2 = applyUnifiedPatch(crlf_src, files[0], hunks[0..p2.hunks_len], ops[0..p2.ops_len], scratch[0..], out_buf[0..], &conflict) catch unreachable;
    assert(strEql(out_buf[0..r2.output_len], "first\r\nSECOND\r\nthird\r\n"));
});

test("PatchInstallSession 6-stage atomic pipeline state machine", () => {
    const patch =
        "--- a/a.txt\n" ++
        "+++ b/a.txt\n" ++
        "@@ -1,1 +1,1 @@\n" ++
        "-old\n" ++
        "+new\n";
    let files: [2]PatchFileEntry = [PatchFileEntry.empty(); 2];
    let hunks: [2]PatchHunk = [PatchHunk.empty(); 2];
    let ops: [8]PatchLineOp = [PatchLineOp.empty(); 8];
    let scratch: [8]SourceLineSpan = [SourceLineSpan.empty(); 8];
    let out_buf: [64]u8 = [0; 64];
    let conflict = PatchConflictInfo.none();

    const parsed = parseUnifiedPatch(patch, PatchParseOptions.defaults(), files[0..], hunks[0..], ops[0..]) catch unreachable;
    let sess = PatchInstallSession.init();
    assert(sess.stage == PatchInstallStage.Init);

    // Out-of-order transition must fail
    let bad_trans = false;
    sess.markCommitted() catch |e| {
        bad_trans = (e == PatchApplyError.InvalidStageTransition);
    };
    assert(bad_trans);

    sess.markParsed(parsed) catch unreachable;
    assert(sess.stage == PatchInstallStage.Parsed);
    assertEq(sess.files_total, @as(u32, 1));

    sess.markStaged() catch unreachable;
    assert(sess.stage == PatchInstallStage.Staged);

    const res = applyUnifiedPatch("old\n", files[0], hunks[0..parsed.hunks_len], ops[0..parsed.ops_len], scratch[0..], out_buf[0..], &conflict) catch unreachable;
    sess.recordFileApplied(res) catch unreachable;
    assert(sess.stage == PatchInstallStage.Applied);
    assertEq(sess.files_applied, @as(u32, 1));
    assertEq(sess.hunks_applied, @as(u32, 1));

    sess.markTagged() catch unreachable;
    assert(sess.stage == PatchInstallStage.Tagged);

    sess.markCommitted() catch unreachable;
    assert(sess.isCommitted());
});
