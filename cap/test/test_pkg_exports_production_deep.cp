import { test, expect_eq } from "cap:test";
import {
    ExportConditionRule,
    ResolveStatus,
    ExportsMap,
} from "cap:pkg_exports";

test("1. Root entry '.' conditional priority (bun / import / require / default)", () => {
    let map = ExportsMap.init();
    const rules: [4]ExportConditionRule = [
        ExportConditionRule.toPath("bun", "./src/index.ts"),
        ExportConditionRule.toPath("import", "./dist/esm/index.mjs"),
        ExportConditionRule.toPath("require", "./dist/cjs/index.cjs"),
        ExportConditionRule.toPath("default", "./dist/fallback.js"),
    ];
    expect_eq(map.addSubpath(".", rules[0..]), true);

    let out: [128]u8 = std.mem.zeroes([128]u8);

    // Bun + import active -> declaration order picks "bun"
    const bun_conds: [2][]const u8 = ["import", "bun"];
    const r1 = map.resolve(".", bun_conds[0..], out[0..]);
    expect_eq(r1.isMatched(), true);
    expect_eq(std.mem.eql(u8, r1.path, "./src/index.ts"), true);
    expect_eq(std.mem.eql(u8, r1.matched_condition, "bun"), true);

    // Node CJS -> picks "require"
    const cjs_conds: [1][]const u8 = ["require"];
    const r2 = map.resolve(".", cjs_conds[0..], out[0..]);
    expect_eq(std.mem.eql(u8, r2.path, "./dist/cjs/index.cjs"), true);

    // Unknown condition -> falls back to "default"
    const other_conds: [1][]const u8 = ["browser"];
    const r3 = map.resolve(".", other_conds[0..], out[0..]);
    expect_eq(std.mem.eql(u8, r3.path, "./dist/fallback.js"), true);
});

test("2. Wildcard PATTERN_KEY_COMPARE longest-prefix & longest-suffix competition", () => {
    let map = ExportsMap.init();
    expect_eq(map.addDirect("./*", "./dist/*.js"), true);
    expect_eq(map.addDirect("./features/*", "./dist/feat/*.mjs"), true);
    expect_eq(map.addDirect("./features/*.js", "./dist/feat-js/*.js"), true);

    let out: [128]u8 = std.mem.zeroes([128]u8);
    const conds: [1][]const u8 = ["import"];

    // Generic wildcard
    const r1 = map.resolve("./utils", conds[0..], out[0..]);
    expect_eq(std.mem.eql(u8, r1.path, "./dist/utils.js"), true);

    // Longer prefix "./features/*" beats "./*"
    const r2 = map.resolve("./features/auth", conds[0..], out[0..]);
    expect_eq(std.mem.eql(u8, r2.path, "./dist/feat/auth.mjs"), true);

    // Same prefix "./features/", longer suffix ".js" beats ""
    const r3 = map.resolve("./features/auth.js", conds[0..], out[0..]);
    expect_eq(std.mem.eql(u8, r3.path, "./dist/feat-js/auth.js"), true);
});

test("3. Exact subpath match takes precedence over wildcard", () => {
    let map = ExportsMap.init();
    expect_eq(map.addDirect("./*", "./dist/*.js"), true);
    expect_eq(map.addDirect("./special", "./dist/custom_entry.js"), true);

    let out: [128]u8 = std.mem.zeroes([128]u8);
    const conds: [0][]const u8 = [];
    const r = map.resolve("./special", conds[0..], out[0..]);
    expect_eq(r.isMatched(), true);
    expect_eq(std.mem.eql(u8, r.path, "./dist/custom_entry.js"), true);
});

test("4. null-blocked private subpaths & conditional blocking", () => {
    let map = ExportsMap.init();
    expect_eq(map.addDirect("./*", "./dist/*.js"), true);
    expect_eq(map.blockSubpath("./internal/*"), true);

    const browser_block: [2]ExportConditionRule = [
        ExportConditionRule.block("browser"),
        ExportConditionRule.toPath("default", "./dist/node_only.js"),
    ];
    expect_eq(map.addSubpath("./fs", browser_block[0..]), true);

    let out: [128]u8 = std.mem.zeroes([128]u8);
    const node_conds: [1][]const u8 = ["node"];
    const r_priv = map.resolve("./internal/secret", node_conds[0..], out[0..]);
    expect_eq(r_priv.isBlocked(), true);

    const browser_conds: [1][]const u8 = ["browser"];
    const r_fs_browser = map.resolve("./fs", browser_conds[0..], out[0..]);
    expect_eq(r_fs_browser.isBlocked(), true);

    const r_fs_node = map.resolve("./fs", node_conds[0..], out[0..]);
    expect_eq(r_fs_node.isMatched(), true);
    expect_eq(std.mem.eql(u8, r_fs_node.path, "./dist/node_only.js"), true);
});

test("5. Path traversal ('..') rejection, unexported subpath (.not_found) & tiny buffer guard", () => {
    let map = ExportsMap.init();
    expect_eq(map.addDirect("./utils/*", "./dist/utils/*.js"), true);

    let out: [128]u8 = std.mem.zeroes([128]u8);
    const conds: [1][]const u8 = ["import"];

    // '..' traversal rejected
    const r_trav = map.resolve("./utils/../../etc/passwd", conds[0..], out[0..]);
    expect_eq(r_trav.status == ResolveStatus.invalid_subpath, true);

    // Invalid subpath without leading '.'
    const r_bad = map.resolve("utils/foo", conds[0..], out[0..]);
    expect_eq(r_bad.status == ResolveStatus.invalid_subpath, true);

    // Unexported subpath -> .not_found
    const r_miss = map.resolve("./private", conds[0..], out[0..]);
    expect_eq(r_miss.isNotFound(), true);

    // Buffer too small
    const r_small = map.resolve("./utils/helper", conds[0..], out[0..4]);
    expect_eq(r_small.status == ResolveStatus.buffer_too_small, true);
});
