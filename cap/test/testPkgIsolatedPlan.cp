import { test, assert, assertEq } from "cap:test";
import {
    ISO_INVALID_ID,
    ISO_ROOT_ENTRY,
    ISO_DEFAULT_STORE_DIR,
    IsolatedPlanError,
    validateIsoPkgName,
    validateIsoRelPath,
    formatIsoStoreDirKey,
    formatIsoRelativeLinkTarget,
    IsoDepKind,
    IsoInstallState,
    IsoPkgInput,
    IsoDepEdge,
    IsoSccScratch,
    computeIsoSccs,
    computeLeakingPeerMasks,
    IsoStoreEntry,
    IsoSymlinkOp,
    IsoDfsFrame,
    IsoInstallPlanner,
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

test("Security hygiene: package name, workspace relpath, store dir key & relative symlink target", () => {
    // Valid package names
    validateIsoPkgName("lodash") catch unreachable;
    validateIsoPkgName("@scope/ui-kit") catch unreachable;
    validateIsoPkgName("foo.bar_baz-1") catch unreachable;

    // Unsafe package names rejected (path traversal, backslash, hidden, malformed scope)
    let bad_empty = false;
    * = validateIsoPkgName("") catch |e| { bad_empty = (e == IsolatedPlanError.EmptyPackageName); };
    assert(bad_empty);

    let bad_dotdot = false;
    * = validateIsoPkgName("../evil") catch |e| { bad_dotdot = (e == IsolatedPlanError.UnsafePackageName); };
    assert(bad_dotdot);

    let bad_scope = false;
    * = validateIsoPkgName("@scope/../escape") catch |e| { bad_scope = (e == IsolatedPlanError.UnsafePackageName); };
    assert(bad_scope);

    let bad_hidden = false;
    * = validateIsoPkgName(".hidden") catch |e| { bad_hidden = (e == IsolatedPlanError.UnsafePackageName); };
    assert(bad_hidden);

    // Relative workspace path guards
    validateIsoRelPath("packages/app-web") catch unreachable;
    let bad_abs = false;
    * = validateIsoRelPath("/etc/passwd") catch |e| { bad_abs = (e == IsolatedPlanError.UnsafeRelativePath); };
    assert(bad_abs);
    let bad_trav = false;
    * = validateIsoRelPath("packages/../../secret") catch |e| { bad_trav = (e == IsolatedPlanError.UnsafeRelativePath); };
    assert(bad_trav);

    // Store directory key formatting (scoped + peer hash)
    let buf: [128]u8 = [0; 128];
    const k1 = formatIsoStoreDirKey("@scope/ui", "2.1.0", 0, buf[0..]) catch unreachable;
    assert(strEql(buf[0..k1], "@scope+ui@2.1.0"));

    const k2 = formatIsoStoreDirKey("react-dom", "19.0.0", 0x0123456789abcdef, buf[0..]) catch unreachable;
    assert(strEql(buf[0..k2], "react-dom@19.0.0_0123456789abcdef"));

    // Relative POSIX symlink target computation
    const r1 = formatIsoRelativeLinkTarget(
        "node_modules/@scope",
        "node_modules/.bun/@scope+ui@2.1.0/node_modules/@scope/ui",
        buf[0..],
    ) catch unreachable;
    assert(strEql(buf[0..r1], "../.bun/@scope+ui@2.1.0/node_modules/@scope/ui"));

    const r2 = formatIsoRelativeLinkTarget(
        "node_modules/.bun/a@1.0.0/node_modules",
        "node_modules/.bun/b@2.0.0/node_modules/b",
        buf[0..],
    ) catch unreachable;
    assert(strEql(buf[0..r2], "../../b@2.0.0/node_modules/b"));
});

test("computeIsoSccs iterative Tarjan SCC detects DAG, multi-node cycles, and self-loops", () => {
    // 0 -> 1 -> 2 -> 1 (cycle {1,2}), 0 -> 3 -> 3 (self-loop {3}), 0 -> 4 (acyclic)
    const pkgs: [5]IsoPkgInput = [
        IsoPkgInput{ name: "root", version: "1.0.0", dep_start: 0, dep_len: 3 },
        IsoPkgInput{ name: "a", version: "1.0.0", dep_start: 3, dep_len: 1 },
        IsoPkgInput{ name: "b", version: "1.0.0", dep_start: 4, dep_len: 1 },
        IsoPkgInput{ name: "selfpkg", version: "1.0.0", dep_start: 5, dep_len: 1 },
        IsoPkgInput{ name: "leaf", version: "1.0.0", dep_start: 6, dep_len: 0 },
    ];
    const edges: [6]IsoDepEdge = [
        IsoDepEdge.normal("a", 1),
        IsoDepEdge.normal("selfpkg", 3),
        IsoDepEdge.normal("leaf", 4),
        IsoDepEdge.normal("b", 2),
        IsoDepEdge.normal("a", 1),
        IsoDepEdge.normal("selfpkg", 3),
    ];

    let disc: [5]u32 = [0; 5];
    let low: [5]u32 = [0; 5];
    let on_stack: [5]bool = [false; 5];
    let t_stack: [5]u32 = [0; 5];
    let w_pkg: [5]u32 = [0; 5];
    let w_edge: [5]u32 = [0; 5];
    let scc_id: [5]u32 = [0; 5];
    const scratch = IsoSccScratch{
        disc: disc[0..],
        low: low[0..],
        on_stack: on_stack[0..],
        tarjan_stack: t_stack[0..],
        work_pkg: w_pkg[0..],
        work_edge: w_edge[0..],
        scc_id: scc_id[0..],
    };

    const res = computeIsoSccs(pkgs[0..], edges[0..], scratch) catch unreachable;
    assert(res.has_cycles);
    assertEq(res.scc_count, 4);
    assertEq(res.cyclic_scc_count, 2); // {1,2} and {3}
    assertEq(res.max_scc_size, 2);
    assertEq(scc_id[1], scc_id[2]);
    assert(scc_id[0] != scc_id[1]);
    assert(scc_id[3] != scc_id[4]);
});

test("computeLeakingPeerMasks propagates transitive peers and subtracts internally satisfied peers", () => {
    // 0 (host) -> 1 (wrapper) -> 2 (plugin, peer: react)
    // 3 (self-contained) -> 2 (plugin, peer: react) AND 3 -> 4 (react)
    const pkgs: [5]IsoPkgInput = [
        IsoPkgInput{ name: "host", version: "1.0.0", dep_start: 0, dep_len: 1 },
        IsoPkgInput{ name: "wrapper", version: "1.0.0", dep_start: 1, dep_len: 1 },
        IsoPkgInput{ name: "plugin", version: "1.0.0", dep_start: 2, dep_len: 1 },
        IsoPkgInput{ name: "bundled-host", version: "1.0.0", dep_start: 3, dep_len: 2 },
        IsoPkgInput{ name: "react", version: "19.0.0", dep_start: 5, dep_len: 0 },
    ];
    const edges: [5]IsoDepEdge = [
        IsoDepEdge.normal("wrapper", 1),
        IsoDepEdge.normal("plugin", 2),
        IsoDepEdge.peer("react", 4),
        IsoDepEdge.normal("plugin", 2),
        IsoDepEdge.normal("react", 4),
    ];

    let leaking: [5]u64 = [0; 5];
    let own_p: [5]u64 = [0; 5];
    let own_np: [5]u64 = [0; 5];
    computeLeakingPeerMasks(pkgs[0..], edges[0..], leaking[0..], own_p[0..], own_np[0..]) catch unreachable;

    assert(leaking[2] != 0); // plugin leaks `react`
    assertEq(leaking[1], leaking[2]); // wrapper inherits leaking `react`
    assertEq(leaking[0], leaking[2]); // host inherits leaking `react`
    assertEq(leaking[3], 0); // bundled-host satisfies `react` internally -> 0 leaking!
    assertEq(leaking[4], 0);
});

test("IsoInstallPlanner forks store entries on conflicting peer contexts and deduplicates shared deps", () => {
    // Graph:
    //   0: root -> 1 (app-a), 2 (app-b), 6 (lodash)
    //   1: app-a -> 3 (react@18), 5 (ui-kit), 6 (lodash)
    //   2: app-b -> 4 (react@19), 5 (ui-kit), 6 (lodash)
    //   5: ui-kit -> peer "react" (default 3)
    // Expected:
    //   - `lodash` has no leaking peers -> deduplicates into 1 store entry (ref_count == 3)!
    //   - `ui-kit` under app-a resolves peer react -> react@18; under app-b resolves peer react -> react@19
    //     -> forks into 2 distinct isolated store entries with different `peer_ctx_hash`!
    const pkgs: [7]IsoPkgInput = [
        IsoPkgInput{ name: "root", version: "0.0.0", is_workspace: true, dep_start: 0, dep_len: 3 },
        IsoPkgInput{ name: "app-a", version: "1.0.0", dep_start: 3, dep_len: 3 },
        IsoPkgInput{ name: "app-b", version: "1.0.0", dep_start: 6, dep_len: 3 },
        IsoPkgInput{ name: "react", version: "18.3.1", dep_start: 9, dep_len: 0 },
        IsoPkgInput{ name: "react", version: "19.0.0", dep_start: 9, dep_len: 0 },
        IsoPkgInput{ name: "ui-kit", version: "2.0.0", dep_start: 9, dep_len: 1 },
        IsoPkgInput{ name: "lodash", version: "4.17.21", dep_start: 10, dep_len: 0 },
    ];
    const edges: [10]IsoDepEdge = [
        // root
        IsoDepEdge.normal("app-a", 1),
        IsoDepEdge.normal("app-b", 2),
        IsoDepEdge.normal("lodash", 6),
        // app-a
        IsoDepEdge.normal("react", 3),
        IsoDepEdge.normal("ui-kit", 5),
        IsoDepEdge.normal("lodash", 6),
        // app-b
        IsoDepEdge.normal("react", 4),
        IsoDepEdge.normal("ui-kit", 5),
        IsoDepEdge.normal("lodash", 6),
        // ui-kit
        IsoDepEdge.peer("react", 3),
    ];

    let disc: [7]u32 = [0; 7];
    let low: [7]u32 = [0; 7];
    let on_stack: [7]bool = [false; 7];
    let t_stack: [7]u32 = [0; 7];
    let w_pkg: [7]u32 = [0; 7];
    let w_edge: [7]u32 = [0; 7];
    let scc_id: [7]u32 = [0; 7];
    let leaking: [7]u64 = [0; 7];
    let own_p: [7]u64 = [0; 7];
    let own_np: [7]u64 = [0; 7];
    let dfs_stack: [16]IsoDfsFrame = [IsoDfsFrame.empty(); 16];
    let entries: [16]IsoStoreEntry = [IsoStoreEntry.empty(); 16];
    let links: [32]IsoSymlinkOp = [IsoSymlinkOp.empty(); 32];

    let planner = IsoInstallPlanner.init(entries[0..], links[0..]);
    const summary = planner.build(
        pkgs[0..],
        edges[0..],
        0,
        IsoSccScratch{
            disc: disc[0..],
            low: low[0..],
            on_stack: on_stack[0..],
            tarjan_stack: t_stack[0..],
            work_pkg: w_pkg[0..],
            work_edge: w_edge[0..],
            scc_id: scc_id[0..],
        },
        leaking[0..],
        own_p[0..],
        own_np[0..],
        dfs_stack[0..],
    ) catch unreachable;

    // 7 distinct store entries: app-a, react@18, ui-kit(react@18), lodash, app-b, react@19, ui-kit(react@19)
    assertEq(summary.entry_count, 7);
    assertEq(summary.root_link_count, 3);
    assertEq(summary.peer_link_count, 2);
    assertEq(summary.cyclic_scc_count, 0);

    // Count ui-kit entries (must be 2 with distinct peer_ctx_hash) and lodash entries (must be 1 with ref_count == 3)
    let ui_count: usize = 0;
    let ui_hash_0: u64 = 0;
    let ui_hash_1: u64 = 0;
    let lodash_count: usize = 0;
    let lodash_refs: u32 = 0;
    let i: usize = 0;
    while (i < planner.entryCount()) {
        const e = planner.getEntry(@as(u32, @intCast(i))) catch unreachable;
        if (e.pkg_id == 5) {
            if (ui_count == 0) ui_hash_0 = e.peer_ctx_hash else ui_hash_1 = e.peer_ctx_hash;
            ui_count += 1;
        }
        if (e.pkg_id == 6) {
            lodash_count += 1;
            lodash_refs = e.ref_count;
        }
        i += 1;
    }
    assertEq(ui_count, 2);
    assert(ui_hash_0 != 0 && ui_hash_1 != 0 && ui_hash_0 != ui_hash_1);
    assertEq(lodash_count, 1);
    assertEq(lodash_refs, 3);
});

test("IsoInstallPlanner handles cyclic dependencies, workspaces, store paths, and state transitions", () => {
    // 0: root -> 1 (ws-pkg, workspace), 2 (cyc-a)
    // 1: ws-pkg -> 2 (cyc-a)
    // 2: cyc-a -> 3 (cyc-b)
    // 3: cyc-b -> 2 (cyc-a)  <-- cycle!
    const pkgs: [4]IsoPkgInput = [
        IsoPkgInput{ name: "monorepo", version: "0.0.0", is_workspace: true, dep_start: 0, dep_len: 2 },
        IsoPkgInput{ name: "@mono/core", version: "1.0.0", is_workspace: true, workspace_rel_path: "packages/core", dep_start: 2, dep_len: 1 },
        IsoPkgInput{ name: "cyc-a", version: "1.0.0", dep_start: 3, dep_len: 1 },
        IsoPkgInput{ name: "cyc-b", version: "1.0.0", dep_start: 4, dep_len: 1 },
    ];
    const edges: [5]IsoDepEdge = [
        IsoDepEdge.workspace("@mono/core", 1),
        IsoDepEdge.normal("cyc-a", 2),
        IsoDepEdge.normal("cyc-a", 2),
        IsoDepEdge.normal("cyc-b", 3),
        IsoDepEdge.normal("cyc-a", 2),
    ];

    let disc: [4]u32 = [0; 4];
    let low: [4]u32 = [0; 4];
    let on_stack: [4]bool = [false; 4];
    let t_stack: [4]u32 = [0; 4];
    let w_pkg: [4]u32 = [0; 4];
    let w_edge: [4]u32 = [0; 4];
    let scc_id: [4]u32 = [0; 4];
    let leaking: [4]u64 = [0; 4];
    let own_p: [4]u64 = [0; 4];
    let own_np: [4]u64 = [0; 4];
    let dfs_stack: [16]IsoDfsFrame = [IsoDfsFrame.empty(); 16];
    let entries: [8]IsoStoreEntry = [IsoStoreEntry.empty(); 8];
    let links: [16]IsoSymlinkOp = [IsoSymlinkOp.empty(); 16];

    let planner = IsoInstallPlanner.init(entries[0..], links[0..]);
    const summary = planner.build(
        pkgs[0..],
        edges[0..],
        0,
        IsoSccScratch{
            disc: disc[0..],
            low: low[0..],
            on_stack: on_stack[0..],
            tarjan_stack: t_stack[0..],
            work_pkg: w_pkg[0..],
            work_edge: w_edge[0..],
            scc_id: scc_id[0..],
        },
        leaking[0..],
        own_p[0..],
        own_np[0..],
        dfs_stack[0..],
    ) catch unreachable;

    assertEq(summary.entry_count, 3); // @mono/core, cyc-a, cyc-b
    assertEq(summary.cyclic_scc_count, 1);
    assertEq(summary.workspace_link_count, 1);
    assert(summary.deduped_hits >= 2);

    // Verify deterministic symlink ordering: root links come first, alphabetically by dep_name
    assertEq(links[0].from_entry_id, ISO_ROOT_ENTRY);
    assert(strEql(links[0].dep_name, "@mono/core"));
    assertEq(links[1].from_entry_id, ISO_ROOT_ENTRY);
    assert(strEql(links[1].dep_name, "cyc-a"));

    // Format store paths (workspace returns workspace_rel_path; registry pkg returns node_modules/.bun/...)
    let pbuf: [160]u8 = [0; 160];
    const ws_len = planner.formatEntryStorePath(pkgs[0..], 0, ISO_DEFAULT_STORE_DIR, pbuf[0..]) catch unreachable;
    assert(strEql(pbuf[0..ws_len], "packages/core"));

    const pkg_len = planner.formatEntryStorePath(pkgs[0..], 1, ISO_DEFAULT_STORE_DIR, pbuf[0..]) catch unreachable;
    assert(strEql(pbuf[0..pkg_len], "node_modules/.bun/cyc-a@1.0.0/node_modules/cyc-a"));

    // Lifecycle state transitions: pending -> extracting -> extracted -> linking -> linked
    planner.transitionEntryState(1, IsoInstallState.extracting) catch unreachable;
    planner.transitionEntryState(1, IsoInstallState.extracted) catch unreachable;
    planner.transitionEntryState(1, IsoInstallState.linking) catch unreachable;
    planner.transitionEntryState(1, IsoInstallState.linked) catch unreachable;
    assert((planner.getEntry(1) catch unreachable).state == IsoInstallState.linked);

    // Illegal backward transition from linked -> pending rejected
    let bad_trans = false;
    * = planner.transitionEntryState(1, IsoInstallState.pending) catch |e| {
        bad_trans = (e == IsolatedPlanError.InvalidStateTransition);
    };
    assert(bad_trans);

    // Plan report formatting
    let rep: [512]u8 = [0; 512];
    const rn = planner.formatPlanReport(pkgs[0..], rep[0..]) catch unreachable;
    assert(rn > 40);
});

test("IsoInstallPlanner capacity & boundary error guards", () => {
    const pkgs: [2]IsoPkgInput = [
        IsoPkgInput{ name: "root", version: "1.0.0", dep_start: 0, dep_len: 1 },
        IsoPkgInput{ name: "dep", version: "1.0.0", dep_start: 1, dep_len: 0 },
    ];
    const edges: [1]IsoDepEdge = [IsoDepEdge.normal("dep", 1)];

    let disc: [2]u32 = [0; 2];
    let low: [2]u32 = [0; 2];
    let on_stack: [2]bool = [false; 2];
    let t_stack: [2]u32 = [0; 2];
    let w_pkg: [2]u32 = [0; 2];
    let w_edge: [2]u32 = [0; 2];
    let scc_id: [2]u32 = [0; 2];
    let leaking: [2]u64 = [0; 2];
    let own_p: [2]u64 = [0; 2];
    let own_np: [2]u64 = [0; 2];
    let dfs_stack: [4]IsoDfsFrame = [IsoDfsFrame.empty(); 4];

    // StoreFull when entries slice is 0
    let zero_entries: [0]IsoStoreEntry = [];
    let links: [4]IsoSymlinkOp = [IsoSymlinkOp.empty(); 4];
    let p1 = IsoInstallPlanner.init(zero_entries[0..], links[0..]);
    let store_full = false;
    * = p1.build(
        pkgs[0..],
        edges[0..],
        0,
        IsoSccScratch{
            disc: disc[0..],
            low: low[0..],
            on_stack: on_stack[0..],
            tarjan_stack: t_stack[0..],
            work_pkg: w_pkg[0..],
            work_edge: w_edge[0..],
            scc_id: scc_id[0..],
        },
        leaking[0..],
        own_p[0..],
        own_np[0..],
        dfs_stack[0..],
    ) catch |e| { store_full = (e == IsolatedPlanError.StoreFull); };
    assert(store_full);

    // InvalidEntryId guard
    let bad_entry = false;
    * = p1.getEntry(ISO_INVALID_ID) catch |e| { bad_entry = (e == IsolatedPlanError.InvalidEntryId); };
    assert(bad_entry);

    // SymlinkListFull when symlinks slice is 0
    let ok_entries: [4]IsoStoreEntry = [IsoStoreEntry.empty(); 4];
    let zero_links: [0]IsoSymlinkOp = [];
    let p2 = IsoInstallPlanner.init(ok_entries[0..], zero_links[0..]);
    let link_full = false;
    * = p2.build(
        pkgs[0..],
        edges[0..],
        0,
        IsoSccScratch{
            disc: disc[0..],
            low: low[0..],
            on_stack: on_stack[0..],
            tarjan_stack: t_stack[0..],
            work_pkg: w_pkg[0..],
            work_edge: w_edge[0..],
            scc_id: scc_id[0..],
        },
        leaking[0..],
        own_p[0..],
        own_np[0..],
        dfs_stack[0..],
    ) catch |e| { link_full = (e == IsolatedPlanError.SymlinkListFull); };
    assert(link_full);
});
