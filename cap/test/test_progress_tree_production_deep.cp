import { test, expect } from "cap:test";
import { ProgressTree, ProgressUnit } from "cap:obs";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("progress_tree: hierarchical tracking, formatting and end propagation", () => {
    let buf: [128]u8 = [0; 128];
    let pt = ProgressTree.init(0);
    const root = pt.startRoot("Installing", 3) ?? unreachable;
    const dl = pt.startChild(root, "Downloading", 1024 * 1024, ProgressUnit.bytes) ?? unreachable;

    pt.addCompleted(dl, 512 * 1024);
    expect(pt.percent(dl) == 50);
    expect(strEq(pt.formatLine(buf[0..]), "[0/3] Installing > Downloading (512KB/1MB)"));

    // Ending child completes one item on parent and clears active child
    pt.end(dl);
    expect(pt.nodes[root].completed == 1);
    expect(strEq(pt.formatLine(buf[0..]), "[1/3] Installing"));

    const comp = pt.startChild(root, "Compiling", 10, ProgressUnit.files) ?? unreachable;
    pt.addCompleted(comp, 4);
    expect(strEq(pt.formatLine(buf[0..]), "[1/3] Installing > Compiling (4/10 files)"));

    pt.end(comp);
    pt.end(root);
    expect(pt.done);
});

test("progress_tree: initial delay and refresh rate throttling", () => {
    let pt = ProgressTree.init(1_000_000_000);
    expect(pt.startRoot("Build", 10) != null);

    // Before 500ms initial delay -> false
    expect(!pt.shouldRefresh(1_200_000_000));
    // At 500ms -> true
    expect(pt.shouldRefresh(1_500_000_000));
    // 20ms later (< 50ms rate) -> false
    expect(!pt.shouldRefresh(1_520_000_000));
    // 50ms later -> true
    expect(pt.shouldRefresh(1_550_000_000));
});
