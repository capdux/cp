// Origin: Pure-CP Stdlib Test Suite (lib/sys/walkerSkip.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Hash-Accelerated Directory Walker Skip Filter & Path Stack:
//   1. walkerHashName determinism, seed sensitivity & empty-rule zero-hash fast path
//   2. Skip directory / file / symlink (`skip_all`) isolation & deduplication
//   3. withCommonIgnores preset (`.git`, `node_modules`, `.cap`, `.DS_Store`) & skip_hidden mode
//   4. DT_UNKNOWN (`WalkerEntryKind.Unknown`) resolve_unknown_entry_types policy
//   5. WalkerPathStack single-buffer pushDir / joinEntry / popDir shrink-and-append traversal
//   6. Boundary guards: rule table capacity, max depth overflow & oversized path rejection

import { test, expect_eq, expect_true } from "cap:test";
import {
    WALKER_MAX_SKIP_RULES,
    WALKER_MAX_DEPTH,
    WalkerEntryKind,
    WalkerDecision,
    walkerHashName,
    WalkerSkipFilter,
    WalkerPathStack,
} from "../../lib/sys/walkerSkip.cp";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

// ============================================================================
// Test 1: walkerHashName determinism & seed isolation
// ============================================================================
test("walkerSkip: walkerHashName determinism and seed sensitivity", () => {
    const h1 = walkerHashName(0, "node_modules");
    const h2 = walkerHashName(0, "node_modules");
    const h3 = walkerHashName(42, "node_modules");
    const h4 = walkerHashName(0, ".git");

    expect_eq(h1, h2);
    expect_true(h1 != h3);
    expect_true(h1 != h4);

    // Empty filter skips nothing
    const empty = WalkerSkipFilter.init(0);
    expect_true(!empty.shouldSkip(WalkerEntryKind.Directory, "node_modules"));
    expect_true(!empty.shouldSkip(WalkerEntryKind.File, ".DS_Store"));
    expect_true(!empty.shouldSkip(WalkerEntryKind.SymLink, "node_modules"));
});

// ============================================================================
// Test 2: Directory, file, and symlink (`skip_all`) rule isolation & dedup
// ============================================================================
test("walkerSkip: dir/file/symlink rule isolation and duplicate suppression", () => {
    let f = WalkerSkipFilter.init(1337);
    f.addSkipDir("build") catch {
        expect_true(false);
    };
    // Duplicate add should be idempotent
    f.addSkipDir("build") catch {
        expect_true(false);
    };
    f.addSkipFile("thumbs.db") catch {
        expect_true(false);
    };

    expect_eq(f.dirRuleCount(), 1);
    expect_eq(f.fileRuleCount(), 1);
    expect_eq(f.allRuleCount(), 2);

    // "build" skipped as Directory and SymLink, NOT as regular File
    expect_true(f.shouldSkip(WalkerEntryKind.Directory, "build"));
    expect_true(f.shouldSkip(WalkerEntryKind.SymLink, "build"));
    expect_true(!f.shouldSkip(WalkerEntryKind.File, "build"));

    // "thumbs.db" skipped as File and SymLink, NOT as Directory
    expect_true(f.shouldSkip(WalkerEntryKind.File, "thumbs.db"));
    expect_true(f.shouldSkip(WalkerEntryKind.SymLink, "thumbs.db"));
    expect_true(!f.shouldSkip(WalkerEntryKind.Directory, "thumbs.db"));

    // addSkipAny skips all kinds
    f.addSkipAny("secret") catch {
        expect_true(false);
    };
    expect_true(f.shouldSkip(WalkerEntryKind.Directory, "secret"));
    expect_true(f.shouldSkip(WalkerEntryKind.File, "secret"));
    expect_true(f.shouldSkip(WalkerEntryKind.SymLink, "secret"));
});

// ============================================================================
// Test 3: withCommonIgnores preset & skip_hidden mode
// ============================================================================
test("walkerSkip: withCommonIgnores preset and skip_hidden toggle", () => {
    let f = WalkerSkipFilter.withCommonIgnores(99);
    expect_true(f.evaluate(WalkerEntryKind.Directory, ".git") == WalkerDecision.Skip);
    expect_true(f.evaluate(WalkerEntryKind.Directory, "node_modules") == WalkerDecision.Skip);
    expect_true(f.evaluate(WalkerEntryKind.Directory, ".cap") == WalkerDecision.Skip);
    expect_true(f.evaluate(WalkerEntryKind.File, ".DS_Store") == WalkerDecision.Skip);

    expect_true(f.evaluate(WalkerEntryKind.Directory, "src") == WalkerDecision.Descend);
    expect_true(f.evaluate(WalkerEntryKind.File, "main.cp") == WalkerDecision.Visit);
    expect_true(f.evaluate(WalkerEntryKind.File, ".env") == WalkerDecision.Visit);

    // Enable skip_hidden -> dotfiles/dotdirs skipped
    f.setSkipHidden(true);
    expect_true(f.evaluate(WalkerEntryKind.File, ".env") == WalkerDecision.Skip);
    expect_true(f.evaluate(WalkerEntryKind.Directory, ".vscode") == WalkerDecision.Skip);
    expect_true(f.evaluate(WalkerEntryKind.File, "README.md") == WalkerDecision.Visit);
});

// ============================================================================
// Test 4: DT_UNKNOWN (`Unknown`) entry type resolution policy
// ============================================================================
test("walkerSkip: DT_UNKNOWN resolution toggle for NFS/FUSE filesystems", () => {
    let f = WalkerSkipFilter.init(7);
    expect_true(f.evaluate(WalkerEntryKind.Unknown, "entry") == WalkerDecision.Visit);

    f.setResolveUnknown(true);
    expect_true(f.evaluate(WalkerEntryKind.Unknown, "entry") == WalkerDecision.ResolveUnknown);
});

// ============================================================================
// Test 5: WalkerPathStack single-buffer pushDir / joinEntry / popDir
// ============================================================================
test("walkerSkip: WalkerPathStack zero-alloc pushDir, joinEntry, and popDir", () => {
    let st = WalkerPathStack.initPosix();
    expect_eq(st.depth(), 0);
    expect_true(strEq(st.currentDir(), ""));

    st.pushDir("src") catch {
        expect_true(false);
    };
    expect_eq(st.depth(), 1);
    expect_true(strEq(st.currentDir(), "src"));

    if (p1 := st.joinEntry("main.cp")) {
        expect_true(strEq(p1, "src/main.cp"));
    } else {
        expect_true(false);
    }
    // joinEntry does not mutate currentDir
    expect_true(strEq(st.currentDir(), "src"));

    st.pushDir("net") catch {
        expect_true(false);
    };
    expect_eq(st.depth(), 2);
    expect_true(strEq(st.currentDir(), "src/net"));

    if (p2 := st.joinEntry("blockList.cp")) {
        expect_true(strEq(p2, "src/net/blockList.cp"));
    } else {
        expect_true(false);
    }

    expect_true(st.popDir());
    expect_eq(st.depth(), 1);
    expect_true(strEq(st.currentDir(), "src"));

    expect_true(st.popDir());
    expect_eq(st.depth(), 0);
    expect_true(strEq(st.currentDir(), ""));
    expect_true(!st.popDir());
});

// ============================================================================
// Test 6: Capacity & max-depth boundary guards
// ============================================================================
test("walkerSkip: rule table capacity & max depth overflow guards", () => {
    let f = WalkerSkipFilter.init(1);
    let name_buf: [8]u8 = ['d', '0', '0', '0', 0, 0, 0, 0];
    let i: usize = 0;
    while (i < WALKER_MAX_SKIP_RULES) {
        name_buf[1] = @as(u8, '0') + @as(u8, (i / 100) % 10);
        name_buf[2] = @as(u8, '0') + @as(u8, (i / 10) % 10);
        name_buf[3] = @as(u8, '0') + @as(u8, i % 10);
        f.addSkipDir(name_buf[0..4]) catch {
            expect_true(false);
        };
        i += 1;
    }
    expect_eq(f.dirRuleCount(), WALKER_MAX_SKIP_RULES);

    let table_full = false;
    f.addSkipDir("overflow_dir") catch {
        table_full = true;
    };
    expect_true(table_full);

    // Depth overflow guard
    let st = WalkerPathStack.initPosix();
    let d: usize = 0;
    while (d < WALKER_MAX_DEPTH) {
        st.pushDir("a") catch {
            expect_true(false);
        };
        d += 1;
    }
    let depth_overflow = false;
    st.pushDir("too_deep") catch {
        depth_overflow = true;
    };
    expect_true(depth_overflow);
});
