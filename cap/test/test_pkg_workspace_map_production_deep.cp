// Production Deep Tests for cap:pkg Monorepo WorkspaceMap (lib/pkg/workspace_map.cp)
import { test, expect_eq } from "cap:test";
import {
    MAX_WORKSPACE_MAP_ENTRIES,
    WorkspaceMapError,
    isIgnoredWorkspacePath,
    hasIgnoredWorkspaceSegment,
    normalizeWorkspacePath,
    WorkspaceMap,
} from "cap:pkg";

test("pkg workspace_map: ignored directory segments (POSIX & Windows)", fn() {
    expect_eq(isIgnoredWorkspacePath("node_modules"), true);
    expect_eq(isIgnoredWorkspacePath(".git"), true);
    expect_eq(isIgnoredWorkspacePath("CMakeFiles"), true);
    expect_eq(isIgnoredWorkspacePath("packages"), false);

    expect_eq(hasIgnoredWorkspaceSegment("packages/app/node_modules/pkg"), true);
    expect_eq(hasIgnoredWorkspaceSegment("packages\\.git\\hooks"), true);
    expect_eq(hasIgnoredWorkspaceSegment("build/CMakeFiles/tmp"), true);
    expect_eq(hasIgnoredWorkspaceSegment("packages/core/src"), false);
});

test("pkg workspace_map: path normalization strips ./ and leading/trailing slashes", fn() {
    expect_eq(normalizeWorkspacePath("./packages/core/"), "packages/core");
    expect_eq(normalizeWorkspacePath("././apps/web///"), "apps/web");
    expect_eq(normalizeWorkspacePath("/packages/ui/"), "packages/ui");
    expect_eq(normalizeWorkspacePath("packages/cli"), "packages/cli");
    expect_eq(normalizeWorkspacePath("./"), "");
});

test("pkg workspace_map: bidirectional lookup by normalized path and package name", fn() {
    let wm = WorkspaceMap.init();
    expect_eq(wm.isEmpty(), true);

    * = wm.put("./packages/core/", "@acme/core", "1.2.0", 14) catch unreachable;
    * = wm.put("apps/web", "web-app", null, 28) catch unreachable;

    expect_eq(wm.count(), @as(usize, 2));
    expect_eq(wm.containsPath("packages/core"), true);
    expect_eq(wm.containsPath("./packages/core/"), true);
    expect_eq(wm.containsName("@acme/core"), true);

    const by_path = wm.getByPath("packages/core") ?? unreachable;
    expect_eq(by_path.name, "@acme/core");
    expect_eq(by_path.version ?? "", "1.2.0");
    expect_eq(by_path.name_loc, @as(u32, 14));

    const by_name = wm.getByName("web-app") ?? unreachable;
    expect_eq(by_name.rel_path, "apps/web");
    expect_eq(by_name.version == null, true);

    expect_eq(wm.getByPath("missing") == null, true);
    expect_eq(wm.getByName("missing") == null, true);
});

test("pkg workspace_map: deterministic sortByName lexicographical ordering", fn() {
    let wm = WorkspaceMap.init();
    * = wm.put("packages/z", "zeta", "1.0.0", 0) catch unreachable;
    * = wm.put("packages/b", "@acme/b", "1.0.0", 0) catch unreachable;
    * = wm.put("packages/a", "alpha", "1.0.0", 0) catch unreachable;
    * = wm.put("packages/a2", "@acme/a", "1.0.0", 0) catch unreachable;

    wm.sortByName();

    expect_eq((wm.at(0) ?? unreachable).name, "@acme/a");
    expect_eq((wm.at(1) ?? unreachable).name, "@acme/b");
    expect_eq((wm.at(2) ?? unreachable).name, "alpha");
    expect_eq((wm.at(3) ?? unreachable).name, "zeta");
    expect_eq(wm.at(4) == null, true);
});

test("pkg workspace_map: empty, duplicate path, and duplicate package name guards", fn() {
    let wm = WorkspaceMap.init();
    * = wm.put("packages/core", "@acme/core", "1.0.0", 0) catch unreachable;

    // Empty path or name -> EmptyPathOrName
    let empty_err = false;
    * = wm.put("./", "valid", null, 0) catch |e| {
        empty_err = (e == WorkspaceMapError.EmptyPathOrName);
        @as(usize, 0)
    };
    expect_eq(empty_err, true);

    let empty_name_err = false;
    * = wm.put("packages/x", "", null, 0) catch |e| {
        empty_name_err = (e == WorkspaceMapError.EmptyPathOrName);
        @as(usize, 0)
    };
    expect_eq(empty_name_err, true);

    // Duplicate path (after normalization) -> DuplicateWorkspacePath
    let dup_path = false;
    * = wm.put("./packages/core/", "@acme/other", null, 0) catch |e| {
        dup_path = (e == WorkspaceMapError.DuplicateWorkspacePath);
        @as(usize, 0)
    };
    expect_eq(dup_path, true);

    // Duplicate package name -> DuplicateWorkspaceName
    let dup_name = false;
    * = wm.put("packages/core2", "@acme/core", null, 0) catch |e| {
        dup_name = (e == WorkspaceMapError.DuplicateWorkspaceName);
        @as(usize, 0)
    };
    expect_eq(dup_name, true);
});

test("pkg workspace_map: capacity guard at MAX_WORKSPACE_MAP_ENTRIES", fn() {
    expect_eq(MAX_WORKSPACE_MAP_ENTRIES, @as(usize, 128));
    let wm = WorkspaceMap.init();
    let paths: [128][8]u8 = [[0; 8]; 128];
    let i: usize = 0;
    while (i < MAX_WORKSPACE_MAP_ENTRIES) {
        paths[i][0] = 'p';
        paths[i][1] = @intCast('0' + (i / 100));
        paths[i][2] = @intCast('0' + ((i / 10) % 10));
        paths[i][3] = @intCast('0' + (i % 10));
        const s = paths[i][0..4];
        * = wm.put(s, s, null, 0) catch unreachable;
        i += 1;
    }
    expect_eq(wm.count(), MAX_WORKSPACE_MAP_ENTRIES);

    let cap_err = false;
    * = wm.put("overflow", "overflow", null, 0) catch |e| {
        cap_err = (e == WorkspaceMapError.CapacityExceeded);
        @as(usize, 0)
    };
    expect_eq(cap_err, true);
});
