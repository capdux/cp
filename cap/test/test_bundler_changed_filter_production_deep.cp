import std from "std";
import { test, expect_eq } from "cap:test";
import {
    ChangedFilterError,
    ChangedFilterSummary,
    AffectedEntryResult,
    ReverseImportGraph,
    parseChangedPathLines,
    freeChangedPathList,
} from "cap:bundler";

test("1. ReverseImportGraph idempotent registration, edge deduplication, self-loop ignore, and bounds check", () => {
    const alloc = std.testing.allocator;
    let g = ReverseImportGraph.init(alloc);
    defer g.deinit();

    const a = g.addModule("src/a.cp") catch unreachable;
    const b = g.addModule("src/b.cp") catch unreachable;
    const a_again = g.addModule("src/a.cp") catch unreachable;

    expect_eq(a, u32(0));
    expect_eq(b, u32(1));
    expect_eq(a_again, u32(0));
    expect_eq(g.moduleCount(), usize(2));
    expect_eq(g.indexOf("src/b.cp") ?? 99, u32(1));
    expect_eq(g.indexOf("src/missing.cp") == null, true);

    // 自环应静默忽略
    g.addImport(a, a) catch unreachable;
    expect_eq(g.importerCount(a), usize(0));

    // 重复边应去重
    g.addImport(a, b) catch unreachable;
    g.addImport(a, b) catch unreachable;
    expect_eq(g.importerCount(b), usize(1));

    // 越界索引返回 ModuleIndexOutOfRange
    let oob = false;
    g.addImport(a, 99) catch |e| {
        if (e == ChangedFilterError.ModuleIndexOutOfRange) {
            oob = true;
        }
    };
    expect_eq(oob, true);
    expect_eq(g.importerCount(99), usize(0));
});

test("2. ReverseImportGraph transitive and diamond dependency backward BFS propagation", () => {
    const alloc = std.testing.allocator;
    let g = ReverseImportGraph.init(alloc);
    defer g.deinit();

    // 菱形图: entry -> {mid_l, mid_r} -> leaf; unrelated -> other
    g.addImportByPath("entry.cp", "mid_l.cp") catch unreachable;
    g.addImportByPath("entry.cp", "mid_r.cp") catch unreachable;
    g.addImportByPath("mid_l.cp", "leaf.cp") catch unreachable;
    g.addImportByPath("mid_r.cp", "leaf.cp") catch unreachable;
    g.addImportByPath("unrelated.cp", "other.cp") catch unreachable;

    const changed: [1][]const u8 = ["leaf.cp"];
    const affected = g.computeAffectedModules(alloc, changed[0..]) catch unreachable;
    defer alloc.free(affected);

    // leaf, mid_l, mid_r, entry 共 4 个节点，不能重复，也不能波及 unrelated/other
    expect_eq(affected.len, usize(4));
});

test("3. ReverseImportGraph cycle-safe BFS does not loop on cyclic imports", () => {
    const alloc = std.testing.allocator;
    let g = ReverseImportGraph.init(alloc);
    defer g.deinit();

    // 3 节点环: a -> b -> c -> a，外加 test_x -> a
    g.addImportByPath("a.cp", "b.cp") catch unreachable;
    g.addImportByPath("b.cp", "c.cp") catch unreachable;
    g.addImportByPath("c.cp", "a.cp") catch unreachable;
    g.addImportByPath("test_x.cp", "a.cp") catch unreachable;
    * = g.addModule("isolated.cp") catch unreachable;

    const changed: [1][]const u8 = ["b.cp"];
    const affected = g.computeAffectedModules(alloc, changed[0..]) catch unreachable;
    defer alloc.free(affected);

    // b -> a -> c & test_x，共 4 个节点，环路终止无死循环
    expect_eq(affected.len, usize(4));
});

test("4. filterAffectedEntries preserves entry order, handles direct ungraphed entry changes, and empty changes", () => {
    const alloc = std.testing.allocator;
    let g = ReverseImportGraph.init(alloc);
    defer g.deinit();

    g.addImportByPath("test/t0.cp", "src/core.cp") catch unreachable;
    g.addImportByPath("test/t1.cp", "src/ui.cp") catch unreachable;
    g.addImportByPath("test/t2.cp", "src/api.cp") catch unreachable;
    g.addImportByPath("src/api.cp", "src/core.cp") catch unreachable;

    const entries: [4][]const u8 = [
        "test/t0.cp",
        "test/t1.cp",
        "test/t2.cp",
        "test/ungraphed_new.cp",
    ];

    // core.cp 变更影响 t0 和 t2；ungraphed_new.cp 虽未入图但自身直接变更也应保留
    const changed: [2][]const u8 = ["src/core.cp", "test/ungraphed_new.cp"];
    let res = g.filterAffectedEntries(alloc, entries[0..], changed[0..]) catch unreachable;
    defer res.deinit();

    expect_eq(res.summary.total_entries, usize(4));
    expect_eq(res.summary.affected_entries, usize(3));
    expect_eq(res.summary.changed_input_count, usize(2));
    expect_eq(res.indices.len, usize(3));
    expect_eq(res.indices[0], u32(0));
    expect_eq(res.indices[1], u32(2));
    expect_eq(res.indices[2], u32(3));

    // 空变更集返回 0 个受影响入口
    const no_changed: [0][]const u8 = [];
    let empty_res = g.filterAffectedEntries(alloc, entries[0..], no_changed[0..]) catch unreachable;
    defer empty_res.deinit();
    expect_eq(empty_res.indices.len, usize(0));
    expect_eq(empty_res.summary.affected_modules, usize(0));
});

test("5. parseChangedPathLines normalizes git diff/ls-files output with root trimming and CRLF", () => {
    const alloc = std.testing.allocator;
    const raw = "  src/math.cp \r\n\n\t/src/http/client.cp\r\n   \nREADME.md\n";
    const paths = parseChangedPathLines(alloc, "/repo/app///", raw) catch unreachable;
    defer freeChangedPathList(alloc, paths);

    expect_eq(paths.len, usize(3));
    expect_eq(std.mem.eql(u8, paths[0], "/repo/app/src/math.cp"), true);
    expect_eq(std.mem.eql(u8, paths[1], "/repo/app/src/http/client.cp"), true);
    expect_eq(std.mem.eql(u8, paths[2], "/repo/app/README.md"), true);

    // 空 git_root 保持相对路径
    const rels = parseChangedPathLines(alloc, "", "a/b.cp\nc.cp\n") catch unreachable;
    defer freeChangedPathList(alloc, rels);
    expect_eq(rels.len, usize(2));
    expect_eq(std.mem.eql(u8, rels[0], "a/b.cp"), true);
    expect_eq(std.mem.eql(u8, rels[1], "c.cp"), true);
});

test("6. End-to-end pipeline: parse git changed paths -> ReverseImportGraph -> filter affected test entries", () => {
    const alloc = std.testing.allocator;
    let g = ReverseImportGraph.init(alloc);
    defer g.deinit();

    g.addImportByPath("/ws/test/router.test.cp", "/ws/src/http/router.cp") catch unreachable;
    g.addImportByPath("/ws/src/http/router.cp", "/ws/src/http/url.cp") catch unreachable;
    g.addImportByPath("/ws/test/sql.test.cp", "/ws/src/sql/lexer.cp") catch unreachable;

    const git_out = "src/http/url.cp\n";
    const changed_owned = parseChangedPathLines(alloc, "/ws", git_out) catch unreachable;
    defer freeChangedPathList(alloc, changed_owned);

    const changed_views: [1][]const u8 = [changed_owned[0]];
    const entries: [2][]const u8 = [
        "/ws/test/router.test.cp",
        "/ws/test/sql.test.cp",
    ];

    let res = g.filterAffectedEntries(alloc, entries[0..], changed_views[0..]) catch unreachable;
    defer res.deinit();

    expect_eq(res.indices.len, usize(1));
    expect_eq(res.indices[0], u32(0));
    expect_eq(res.summary.affected_modules, usize(3));
});
