// cap/test/testPkgFileMaterialize.cp — Deep production tests for cap:pkg fileMaterialize
// Covers:
//   1. SIMD relative path normalization, depth counting & Windows-illegal/traversal rejection
//   2. Skip filter rules (.git, node_modules, cap_modules, .DS_Store, Thumbs.db, symlink depth loop)
//   3. Byte-level content diff & incremental entry diffing (New, ModifiedContent, ModifiedModeOnly, SkipUnchanged)
//   4. Topological batch planning: MkdirAll before nested files + ApplyExecBit for bin/ & shebang
//   5. Cross-device EXDEV strategy downgrade state machine (CloneFile -> HardlinkFile -> CopyFile)
//   6. Incremental skip savings & diagnostic report formatting (`formatFileMaterializeReport`)
//   7. 500-iteration deterministic random directory tree / permission / malformed path Fuzz gate

import { expect, expectEqual, test } from "cap:test";
import {
    FILE_MAT_MODE_EXEC_MASK,
    FileMatEntryKind,
    FileMatSkipReason,
    normalizeMaterializeRelPath,
    countMaterializePathDepth,
    shouldSkipMaterializeEntry,
    needsExecPermission,
    FileMatSourceEntry,
    FileMatExistingEntry,
    FileMatDiffStatus,
    compareMaterializeContentBytes,
    diffMaterializeEntries,
    FileMatStrategy,
    FileMatOpKind,
    getFileMatOpPath,
    FileMatOp,
    FileMatPlanOptions,
    FileMaterializePlanner,
    formatFileMaterializeReport,
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

fn containsSubstr(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    while (i + needle.len <= haystack.len) {
        if (strEql(haystack[i..i + needle.len], needle)) return true;
        i += 1;
    }
    return false;
}

test("fileMaterialize: SIMD relative path normalization, depth & illegal char rejection", () => {
    let out: [128]u8 = [0; 128];
    const n1 = normalizeMaterializeRelPath("dist\\esm//./index.js", out[0..]) catch unreachable;
    expect(strEql(out[0..n1], "dist/esm/index.js"));
    expectEqual(countMaterializePathDepth(out[0..n1]), @as(u32, 2));

    // Reject absolute path
    let abs_err = false;
    * = normalizeMaterializeRelPath("/etc/passwd", out[0..]) catch { abs_err = true; };
    expect(abs_err);

    // Reject parent traversal `..`
    let trav_err = false;
    * = normalizeMaterializeRelPath("src/../../secret", out[0..]) catch { trav_err = true; };
    expect(trav_err);

    // Reject Windows illegal chars (`*`, `?`, `|`, `:`)
    let win_err = false;
    * = normalizeMaterializeRelPath("dist/file*.js", out[0..]) catch { win_err = true; };
    expect(win_err);
});

test("fileMaterialize: skip rules (.git, node_modules, cap_modules, .DS_Store, symlink depth)", () => {
    const extra: [1][]const u8 = ["__pycache__"];
    expect(shouldSkipMaterializeEntry(".git/config", FileMatEntryKind.File, 0, 32, extra[0..]) == FileMatSkipReason.IgnoredDirName);
    expect(shouldSkipMaterializeEntry("node_modules/a/index.js", FileMatEntryKind.File, 0, 32, extra[0..]) == FileMatSkipReason.IgnoredDirName);
    expect(shouldSkipMaterializeEntry("cap_modules/b/index.cp", FileMatEntryKind.File, 0, 32, extra[0..]) == FileMatSkipReason.IgnoredDirName);
    expect(shouldSkipMaterializeEntry("src/__pycache__/a.pyc", FileMatEntryKind.File, 0, 32, extra[0..]) == FileMatSkipReason.IgnoredDirName);
    expect(shouldSkipMaterializeEntry("src/.DS_Store", FileMatEntryKind.File, 0, 32, extra[0..]) == FileMatSkipReason.IgnoredMetaFile);
    expect(shouldSkipMaterializeEntry("Thumbs.db", FileMatEntryKind.File, 0, 32, extra[0..]) == FileMatSkipReason.IgnoredMetaFile);
    expect(shouldSkipMaterializeEntry("loop/link", FileMatEntryKind.Symlink, 33, 32, extra[0..]) == FileMatSkipReason.DepthExceeded);
    expect(shouldSkipMaterializeEntry("fifo-pipe", FileMatEntryKind.Other, 0, 32, extra[0..]) == FileMatSkipReason.UnsupportedKind);
    expect(shouldSkipMaterializeEntry("src/index.ts", FileMatEntryKind.File, 0, 32, extra[0..]) == FileMatSkipReason.Keep);

    expect(needsExecPermission("bin/cap", 0o644, false));
    expect(needsExecPermission("scripts/run.sh", 0o644, true));
    expect(needsExecPermission("tool", 0o755, false));
    expect(!needsExecPermission("src/lib.ts", 0o644, false));
});

test("fileMaterialize: byte comparison & incremental diffMaterializeEntries", () => {
    expect(compareMaterializeContentBytes("hello", "hello") == null);
    expectEqual(compareMaterializeContentBytes("hello", "heLlo") ?? 999, @as(usize, 2));
    expectEqual(compareMaterializeContentBytes("abc", "abcd") ?? 999, @as(usize, 3));

    const srcs: [4]FileMatSourceEntry = [
        FileMatSourceEntry{ rel_path: "index.js", size: 1000, mtime_ns: 100, content_hash: 0xAAA1, posix_mode: 0o644 },
        FileMatSourceEntry{ rel_path: "bin/cli.js", size: 2000, mtime_ns: 200, content_hash: 0xBBB2, posix_mode: 0o755, has_shebang: true },
        FileMatSourceEntry{ rel_path: "changed.js", size: 3000, mtime_ns: 301, content_hash: 0xCCC9, posix_mode: 0o644 },
        FileMatSourceEntry{ rel_path: "brand-new.js", size: 4000, mtime_ns: 400, content_hash: 0xDDD4, posix_mode: 0o644 },
    ];
    const existing: [3]FileMatExistingEntry = [
        FileMatExistingEntry{ rel_path: "./index.js", size: 1000, mtime_ns: 100, content_hash: 0xAAA1, posix_mode: 0o644 },
        // Same content, missing exec bit -> ModifiedModeOnly
        FileMatExistingEntry{ rel_path: "bin/cli.js", size: 2000, mtime_ns: 200, content_hash: 0xBBB2, posix_mode: 0o644 },
        // Different content hash -> ModifiedContent
        FileMatExistingEntry{ rel_path: "changed.js", size: 3000, mtime_ns: 300, content_hash: 0xCCC3, posix_mode: 0o644 },
    ];

    let statuses: [4]FileMatDiffStatus = [FileMatDiffStatus.NewEntry; 4];
    const dsum = diffMaterializeEntries(srcs[0..], existing[0..], statuses[0..]) catch unreachable;

    expectEqual(dsum.unchanged_skipped, @as(usize, 1));
    expectEqual(dsum.modified_mode_only, @as(usize, 1));
    expectEqual(dsum.modified_content, @as(usize, 1));
    expectEqual(dsum.new_entries, @as(usize, 1));
    expectEqual(dsum.bytes_saved_by_skip, @as(u64, 1000));
    expect(statuses[0] == FileMatDiffStatus.SkipUnchanged);
    expect(statuses[1] == FileMatDiffStatus.ModifiedModeOnly);
    expect(statuses[2] == FileMatDiffStatus.ModifiedContent);
    expect(statuses[3] == FileMatDiffStatus.NewEntry);
});

test("fileMaterialize: topological batch planning (MkdirAll -> HardlinkFile -> ApplyExecBit)", () => {
    let planner = FileMaterializePlanner.init(FileMatStrategy.HardlinkFile);
    const srcs: [5]FileMatSourceEntry = [
        FileMatSourceEntry{ rel_path: "dist/sub/deep.js", size: 5000 },
        FileMatSourceEntry{ rel_path: "bin/my-tool", size: 1200, has_shebang: true },
        FileMatSourceEntry{ rel_path: ".git/HEAD", size: 24 }, // filtered
        FileMatSourceEntry{ rel_path: "package.json", size: 400 },
        FileMatSourceEntry{ rel_path: "dist/.DS_Store", size: 12 }, // filtered
    ];
    const no_ex: [0]FileMatExistingEntry = [];
    const no_skip: [0][]const u8 = [];

    let order: [8]usize = [0; 8];
    let ops: [16]FileMatOp = [FileMatOp.empty(); 16];
    let arena: [1024]u8 = [0; 1024];

    const sum = planner.planBatch(srcs[0..], no_ex[0..], no_skip[0..], FileMatPlanOptions.defaults(), order[0..], ops[0..], arena[0..]) catch unreachable;
    expectEqual(sum.skipped_filtered, @as(usize, 2));
    expectEqual(sum.mkdir_ops, @as(usize, 2)); // "bin" and "dist/sub"
    expectEqual(sum.file_ops, @as(usize, 3));
    expectEqual(sum.chmod_ops, @as(usize, 1)); // bin/my-tool
    expectEqual(sum.total_bytes_to_materialize, @as(u64, 6600));

    // Depth-0 file ("package.json") comes first, before depth-1 "bin/my-tool", before depth-2 "dist/sub/deep.js"
    expect(ops[0].kind == FileMatOpKind.HardlinkFile);
    expect(strEql(getFileMatOpPath(ops[0].rel_path, arena[0..]), "package.json"));
    expect(ops[1].kind == FileMatOpKind.MkdirAll);
    expect(strEql(getFileMatOpPath(ops[1].rel_path, arena[0..]), "bin"));
    expect(ops[2].kind == FileMatOpKind.HardlinkFile);
    expect(strEql(getFileMatOpPath(ops[2].rel_path, arena[0..]), "bin/my-tool"));
    expect(ops[3].kind == FileMatOpKind.ApplyExecBit);
    expect((ops[3].target_mode & FILE_MAT_MODE_EXEC_MASK) != 0);
    expect(ops[4].kind == FileMatOpKind.MkdirAll);
    expect(strEql(getFileMatOpPath(ops[4].rel_path, arena[0..]), "dist/sub"));
    expect(ops[5].kind == FileMatOpKind.HardlinkFile);
    expect(strEql(getFileMatOpPath(ops[5].rel_path, arena[0..]), "dist/sub/deep.js"));
});

test("fileMaterialize: cross-device EXDEV automatic fallback (CloneFile -> HardlinkFile -> CopyFile)", () => {
    let planner = FileMaterializePlanner.init(FileMatStrategy.CloneFile);
    expect(planner.strategy == FileMatStrategy.CloneFile);
    expect(planner.onCrossDeviceExdev() == FileMatStrategy.HardlinkFile);
    expect(planner.onCrossDeviceExdev() == FileMatStrategy.CopyFile);
    expect(planner.onCrossDeviceExdev() == FileMatStrategy.CopyFile); // stays at CopyFile
    expectEqual(planner.exdev_fallbacks, @as(u32, 2));

    const srcs: [1]FileMatSourceEntry = [
        FileMatSourceEntry{ rel_path: "index.js", size: 256 },
    ];
    const no_ex: [0]FileMatExistingEntry = [];
    const no_skip: [0][]const u8 = [];
    let order: [2]usize = [0; 2];
    let ops: [4]FileMatOp = [FileMatOp.empty(); 4];
    let arena: [256]u8 = [0; 256];
    const sum = planner.planBatch(srcs[0..], no_ex[0..], no_skip[0..], FileMatPlanOptions.defaults(), order[0..], ops[0..], arena[0..]) catch unreachable;
    expect(ops[0].kind == FileMatOpKind.CopyFile);
    expectEqual(sum.exdev_fallbacks, @as(u32, 2));
});

test("fileMaterialize: incremental skip savings & diagnostic report formatting", () => {
    let planner = FileMaterializePlanner.init(FileMatStrategy.CloneFile);
    const srcs: [2]FileMatSourceEntry = [
        FileMatSourceEntry{ rel_path: "unchanged.js", size: 8192, mtime_ns: 500, content_hash: 0x9999 },
        FileMatSourceEntry{ rel_path: "fresh.js", size: 1024, mtime_ns: 600, content_hash: 0x8888 },
    ];
    const existing: [1]FileMatExistingEntry = [
        FileMatExistingEntry{ rel_path: "unchanged.js", size: 8192, mtime_ns: 500, content_hash: 0x9999 },
    ];
    const no_skip: [0][]const u8 = [];
    let order: [4]usize = [0; 4];
    let ops: [8]FileMatOp = [FileMatOp.empty(); 8];
    let arena: [512]u8 = [0; 512];

    const sum = planner.planBatch(srcs[0..], existing[0..], no_skip[0..], FileMatPlanOptions.defaults(), order[0..], ops[0..], arena[0..]) catch unreachable;
    expectEqual(sum.skipped_unchanged, @as(usize, 1));
    expectEqual(sum.bytes_saved_by_skip, @as(u64, 8192));
    expectEqual(sum.file_ops, @as(usize, 1));
    expectEqual(sum.total_bytes_to_materialize, @as(u64, 1024));

    let rep: [256]u8 = [0; 256];
    const rlen = formatFileMaterializeReport(sum, rep[0..]) catch unreachable;
    expect(containsSubstr(rep[0..rlen], "FileMaterialize(strategy=clonefile"));
    expect(containsSubstr(rep[0..rlen], "skipped=1"));
    expect(containsSubstr(rep[0..rlen], "saved=8192"));
});

test("fileMaterialize: 500-iteration deterministic random directory tree / path Fuzz gate", () => {
    const chars = "abc0123/\\._-*?:|";
    const no_skip: [0][]const u8 = [];
    let raw_buf: [96]u8 = [0; 96];
    let norm_buf: [128]u8 = [0; 128];

    let rng: u64 = 0xF11E9A7123456789;
    let completed: usize = 0;
    let iter: usize = 0;

    while (iter < 500) {
        rng ^= rng >> 12;
        rng ^= rng << 25;
        rng ^= rng >> 27;
        const len = 1 + @as(usize, @intCast((rng *% 0x2545F4914F6CDD1D) % 80));

        let k: usize = 0;
        while (k < len) {
            rng ^= rng >> 12;
            rng ^= rng << 25;
            rng ^= rng >> 27;
            raw_buf[k] = chars[@as(usize, @intCast(rng % @as(u64, chars.len)))];
            k += 1;
        }
        if (iter % 5 == 0 && len >= 3) {
            raw_buf[0] = '.';
            raw_buf[1] = '.';
            raw_buf[2] = '/';
        }

        const kind = if (iter % 3 == 0) FileMatEntryKind.File
            else if (iter % 3 == 1) FileMatEntryKind.Directory
            else FileMatEntryKind.Symlink;
        const sym_depth = @as(u32, @intCast(iter % 40));

        const verdict = shouldSkipMaterializeEntry(raw_buf[0..len], kind, sym_depth, 16, no_skip[0..]);
        if (verdict == FileMatSkipReason.Keep) {
            const nlen = normalizeMaterializeRelPath(raw_buf[0..len], norm_buf[0..]) catch unreachable;
            expect(nlen > 0 && nlen <= norm_buf.len);
            expect(countMaterializePathDepth(norm_buf[0..nlen]) <= 16);
        }

        completed += 1;
        iter += 1;
    }

    expectEqual(completed, @as(usize, 500));
});
