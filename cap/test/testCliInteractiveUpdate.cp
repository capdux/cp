import { test, assert, assertEq } from "cap:test";
import {
    InteractiveUpdateError,
    parseIuSemver,
    IuUpgradeKind,
    classifySemverUpgrade,
    formatVersionDiffHighlight,
    IuDepGroup,
    IuPackageEntry,
    IuKeyEvent,
    IuSessionOutcome,
    IuSelectedUpgrade,
    InteractiveUpdateState,
    IuRenderOptions,
    renderInteractiveUpdateTable,
} from "cap:cli";

fn strEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn strContains(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    while (i + needle.len <= haystack.len) {
        if (strEql(haystack[i..i + needle.len], needle)) return true;
        i += 1;
    }
    return false;
}

test("classifySemverUpgrade and formatVersionDiffHighlight accurately pinpoint changed version segments", () => {
    const sv = parseIuSemver("^1.2.3-beta.1");
    assert(sv.valid && sv.major == 1 && sv.minor == 2 && sv.patch == 3 && sv.has_prerelease);

    // Patch: 1.2.3 -> 1.2.4 (only "4" highlighted in green)
    const p = classifySemverUpgrade("1.2.3", "1.2.4");
    assert(p.kind == IuUpgradeKind.Patch && !p.is_breaking);
    assertEq(p.diff_start_offset, @as(usize, 4));

    // Minor: 1.2.3 -> 1.4.0 (only "4.0" highlighted in yellow)
    const m = classifySemverUpgrade("1.2.3", "1.4.0");
    assert(m.kind == IuUpgradeKind.Minor && !m.is_breaking);
    assertEq(m.diff_start_offset, @as(usize, 2));

    // Major: 1.2.3 -> 2.0.0 (entire "2.0.0" highlighted in red, breaking)
    const M = classifySemverUpgrade("1.2.3", "2.0.0");
    assert(M.kind == IuUpgradeKind.Major && M.is_breaking);
    assertEq(M.diff_start_offset, @as(usize, 0));

    // Prerelease: 1.2.3-alpha.1 -> 1.2.3-beta.2
    const pre = classifySemverUpgrade("1.2.3-alpha.1", "1.2.3-beta.2");
    assert(pre.kind == IuUpgradeKind.Prerelease);

    // UpToDate
    const same = classifySemverUpgrade("1.2.3", "1.2.3");
    assert(same.kind == IuUpgradeKind.UpToDate);

    // Highlight formatting: "1." plain + yellow "4.0" + reset
    let hbuf: [64]u8 = [0; 64];
    const hlen = formatVersionDiffHighlight("1.2.3", "1.4.0", true, hbuf[0..]) catch unreachable;
    assert(strEql(hbuf[0..hlen], "1.\x1b[33m4.0\x1b[0m"));

    // Plain-text fallback
    const plen = formatVersionDiffHighlight("1.2.3", "1.4.0", false, hbuf[0..]) catch unreachable;
    assert(strEql(hbuf[0..plen], "1.4.0"));
});

test("InteractiveUpdateState handles group ordering, Space/a/n/l toggles, and selected upgrade export", () => {
    let entries: [3]IuPackageEntry = [
        IuPackageEntry{ name: "vitest", group: IuDepGroup.DevDependencies, current_version: "1.5.0", target_version: "1.6.1", latest_version: "2.1.0" },
        IuPackageEntry{ name: "zod", group: IuDepGroup.Dependencies, current_version: "3.22.0", target_version: "3.23.8", latest_version: "4.0.0" },
        IuPackageEntry{ name: "react", group: IuDepGroup.PeerDependencies, current_version: "18.2.0", target_version: "18.3.1", latest_version: "19.0.0" },
    ];
    let vis: [3]usize = [0; 3];
    let st = InteractiveUpdateState.init(entries[0..], vis[0..], 5) catch unreachable;

    // Group order: zod (prod, idx 1) -> vitest (dev, idx 0) -> react (peer, idx 2)
    assertEq(st.visible_count, @as(usize, 3));
    assertEq(vis[0], @as(usize, 1));
    assertEq(vis[1], @as(usize, 0));
    assertEq(vis[2], @as(usize, 2));

    // Select zod and toggle latest -> 4.0.0
    st.handleKey(IuKeyEvent.ToggleSelect);
    st.handleKey(IuKeyEvent.ToggleLatest);
    // Move down to vitest and select target -> 1.6.1
    st.handleKey(IuKeyEvent.Down);
    st.handleKey(IuKeyEvent.ToggleSelect);
    st.handleKey(IuKeyEvent.Confirm);

    assert(st.outcome == IuSessionOutcome.Confirmed);
    assertEq(st.selectedCount(), @as(usize, 2));

    let exported: [4]IuSelectedUpgrade = [IuSelectedUpgrade.empty(); 4];
    const n = st.exportSelectedUpgrades(exported[0..]) catch unreachable;
    assertEq(n, @as(usize, 2));
    assert(strEql(exported[0].name, "vitest") && strEql(exported[0].to_version, "1.6.1") && exported[0].upgrade_kind == IuUpgradeKind.Minor);
    assert(strEql(exported[1].name, "zod") && strEql(exported[1].to_version, "4.0.0") && exported[1].upgrade_kind == IuUpgradeKind.Major);
});

test("InteractiveUpdateState SelectAll / SelectNone / Cancel and SIMD live case-insensitive search filtering", () => {
    let entries: [4]IuPackageEntry = [
        IuPackageEntry{ name: "@cap/http", workspace: "packages/net", group: IuDepGroup.Dependencies, current_version: "1.0.0", target_version: "1.0.1", latest_version: "1.0.1" },
        IuPackageEntry{ name: "@cap/json", workspace: "packages/core", group: IuDepGroup.Dependencies, current_version: "1.0.0", target_version: "1.1.0", latest_version: "1.1.0" },
        IuPackageEntry{ name: "lodash", workspace: "apps/web", group: IuDepGroup.Dependencies, current_version: "4.17.20", target_version: "4.17.21", latest_version: "4.17.21" },
        IuPackageEntry{ name: "esbuild", workspace: "apps/web", group: IuDepGroup.DevDependencies, current_version: "0.20.0", target_version: "0.24.0", latest_version: "0.24.0" },
    ];
    let vis: [4]usize = [0; 4];
    let st = InteractiveUpdateState.init(entries[0..], vis[0..], 2) catch unreachable;

    // Filter by uppercase "CAP" -> matches @cap/http and @cap/json
    st.setFilter("CAP");
    assertEq(st.visible_count, @as(usize, 2));
    st.handleKey(IuKeyEvent.SelectAll);
    assertEq(st.selectedCount(), @as(usize, 2));

    // Clear filter -> all 4 visible, only the 2 previously selected remain checked
    st.handleKey(IuKeyEvent.ClearFilter);
    assertEq(st.visible_count, @as(usize, 4));
    assertEq(st.selectedCount(), @as(usize, 2));

    st.handleKey(IuKeyEvent.SelectNone);
    assertEq(st.selectedCount(), @as(usize, 0));

    // Filter with no matches -> visible_count == 0, cursor/viewport safely 0
    st.setFilter("zzzz-nonexistent");
    assertEq(st.visible_count, @as(usize, 0));
    assertEq(st.cursor, @as(usize, 0));
    st.handleKey(IuKeyEvent.Down);
    assertEq(st.cursor, @as(usize, 0));

    st.handleKey(IuKeyEvent.Cancel);
    assert(st.outcome == IuSessionOutcome.Cancelled);
});

test("InteractiveUpdateState viewport scrolling keeps cursor inside [viewport_start, viewport_start + height)", () => {
    let entries: [6]IuPackageEntry = [
        IuPackageEntry{ name: "p0", current_version: "1.0.0", target_version: "1.0.1", latest_version: "1.0.1" },
        IuPackageEntry{ name: "p1", current_version: "1.0.0", target_version: "1.0.1", latest_version: "1.0.1" },
        IuPackageEntry{ name: "p2", current_version: "1.0.0", target_version: "1.0.1", latest_version: "1.0.1" },
        IuPackageEntry{ name: "p3", current_version: "1.0.0", target_version: "1.0.1", latest_version: "1.0.1" },
        IuPackageEntry{ name: "p4", current_version: "1.0.0", target_version: "1.0.1", latest_version: "1.0.1" },
        IuPackageEntry{ name: "p5", current_version: "1.0.0", target_version: "1.0.1", latest_version: "1.0.1" },
    ];
    let vis: [6]usize = [0; 6];
    let st = InteractiveUpdateState.init(entries[0..], vis[0..], 2) catch unreachable;

    st.handleKey(IuKeyEvent.End);
    assertEq(st.cursor, @as(usize, 5));
    assertEq(st.viewport_start, @as(usize, 4));

    st.handleKey(IuKeyEvent.PageUp);
    assertEq(st.cursor, @as(usize, 3));
    assert(st.cursor >= st.viewport_start && st.cursor < st.viewport_start + st.viewport_height);

    st.handleKey(IuKeyEvent.Home);
    assertEq(st.cursor, @as(usize, 0));
    assertEq(st.viewport_start, @as(usize, 0));
});

test("renderInteractiveUpdateTable renders aligned ANSI and plain-text tables with workspace column", () => {
    let entries: [2]IuPackageEntry = [
        IuPackageEntry{ name: "zod", workspace: "packages/api", group: IuDepGroup.Dependencies, current_version: "3.22.0", target_version: "3.23.8", latest_version: "4.0.0", selected: true, use_latest: true },
        IuPackageEntry{ name: "esbuild", workspace: "packages/cli", group: IuDepGroup.DevDependencies, current_version: "0.20.0", target_version: "0.24.0", latest_version: "0.24.0" },
    ];
    let vis: [2]usize = [0; 2];
    let st = InteractiveUpdateState.init(entries[0..], vis[0..], 10) catch unreachable;

    let ansi_buf: [2048]u8 = [0; 2048];
    const alen = renderInteractiveUpdateTable(st, IuRenderOptions{ ansi_colors: true, show_workspace: true }, ansi_buf[0..]) catch unreachable;
    const ansi_out = ansi_buf[0..alen];
    assert(strContains(ansi_out, "(1/2 selected)"));
    assert(strContains(ansi_out, "[dependencies]"));
    assert(strContains(ansi_out, "[devDependencies]"));
    assert(strContains(ansi_out, "> [x] zod"));
    assert(strContains(ansi_out, "packages/api"));
    assert(strContains(ansi_out, "\x1b[31m4.0.0\x1b[0m*"));

    // Plain-text mode must contain zero ESC bytes (0x1b)
    let plain_buf: [2048]u8 = [0; 2048];
    const plen = renderInteractiveUpdateTable(st, IuRenderOptions{ ansi_colors: false, show_workspace: false }, plain_buf[0..]) catch unreachable;
    assert(!strContains(plain_buf[0..plen], "\x1b["));
    assert(strContains(plain_buf[0..plen], "4.0.0*"));
});

test("InteractiveUpdateState validates zero viewport height and small visible_indices slice", () => {
    let entries: [2]IuPackageEntry = [IuPackageEntry.empty(); 2];
    let vis_ok: [2]usize = [0; 2];
    let vis_small: [1]usize = [0; 1];

    let bad_vp = false;
    * = InteractiveUpdateState.init(entries[0..], vis_ok[0..], 0) catch {
        bad_vp = true;
    };
    assert(bad_vp);

    let bad_cap = false;
    * = InteractiveUpdateState.init(entries[0..], vis_small[0..], 4) catch {
        bad_cap = true;
    };
    assert(bad_cap);
});

test("deterministic 500-iteration fuzz over random keyboard events and live search filters preserves cursor/viewport invariants", () => {
    let seed: u64 = 0x3243f6a8885a308d;
    let entries: [8]IuPackageEntry = [
        IuPackageEntry{ name: "alpha", workspace: "ws-a", group: IuDepGroup.Dependencies, current_version: "1.0.0", target_version: "1.0.1", latest_version: "2.0.0" },
        IuPackageEntry{ name: "beta", workspace: "ws-a", group: IuDepGroup.Dependencies, current_version: "2.1.0", target_version: "2.2.0", latest_version: "3.0.0" },
        IuPackageEntry{ name: "gamma", workspace: "ws-b", group: IuDepGroup.DevDependencies, current_version: "0.1.0", target_version: "0.2.0", latest_version: "1.0.0" },
        IuPackageEntry{ name: "delta", workspace: "ws-b", group: IuDepGroup.OptionalDependencies, current_version: "1.2.3", target_version: "1.2.4", latest_version: "1.2.4" },
        IuPackageEntry{ name: "epsilon", workspace: "ws-c", group: IuDepGroup.PeerDependencies, current_version: "4.0.0", target_version: "4.1.0", latest_version: "5.0.0" },
        IuPackageEntry{ name: "zeta", workspace: "ws-c", group: IuDepGroup.Dependencies, current_version: "1.0.0-alpha.1", target_version: "1.0.0-beta.1", latest_version: "1.0.0" },
        IuPackageEntry{ name: "eta", workspace: "ws-d", group: IuDepGroup.DevDependencies, current_version: "3.0.0", target_version: "3.0.0", latest_version: "3.1.0" },
        IuPackageEntry{ name: "theta", workspace: "ws-d", group: IuDepGroup.Dependencies, current_version: "5.2.0", target_version: "5.3.0", latest_version: "6.0.0" },
    ];
    let vis: [8]usize = [0; 8];
    let st = InteractiveUpdateState.init(entries[0..], vis[0..], 3) catch unreachable;
    let render_buf: [2048]u8 = [0; 2048];

    const keys: [10]IuKeyEvent = [
        IuKeyEvent.Up,
        IuKeyEvent.Down,
        IuKeyEvent.PageUp,
        IuKeyEvent.PageDown,
        IuKeyEvent.Home,
        IuKeyEvent.End,
        IuKeyEvent.ToggleSelect,
        IuKeyEvent.SelectAll,
        IuKeyEvent.SelectNone,
        IuKeyEvent.ToggleLatest,
    ];
    const chars: [8]u8 = ['a', 'E', 't', 'w', 's', '-', 'z', 'q'];

    let iter: u32 = 0;
    while (iter < 500) {
        seed = seed ^ (seed >> 12);
        seed = seed ^ (seed << 25);
        seed = seed ^ (seed >> 27);
        const r = seed *% 2685821657736338717;

        const action = r % 14;
        if (action < 10) {
            st.handleKey(keys[@as(usize, @intCast(action))]);
        } else if (action == 10) {
            st.pushFilterChar(chars[@as(usize, @intCast((r >> 16) % 8))]);
        } else if (action == 11) {
            st.popFilterChar();
        } else if (action == 12) {
            st.handleKey(IuKeyEvent.ClearFilter);
        } else {
            * = renderInteractiveUpdateTable(st, IuRenderOptions{ ansi_colors: (iter & 1) == 0, show_workspace: (iter & 2) == 0 }, render_buf[0..]) catch unreachable;
        }

        // Strict invariants on every single step
        assert(st.visible_count <= entries.len);
        if (st.visible_count == 0) {
            assertEq(st.cursor, @as(usize, 0));
            assertEq(st.viewport_start, @as(usize, 0));
        } else {
            assert(st.cursor < st.visible_count);
            assert(st.cursor >= st.viewport_start);
            assert(st.cursor < st.viewport_start + st.viewport_height);
        }
        iter += 1;
    }
});
