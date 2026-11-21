import {
    TreeStyle,
    TreePrefix,
    TreeNode,
    renderFlatTree,
} from "cap:cli";
import { test, expect } from "cap:test";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("TreePrefix push, pop, and formatPrefix across unicode, ascii, and rounded styles", () => {
    let buf: [128]u8 = [0; 128];

    const root = TreePrefix.init();
    expect(root.depth == 0);
    expect(strEq(root.formatPrefix(TreeStyle.unicode, false, buf[0..]).?, "├── "));
    expect(strEq(root.formatPrefix(TreeStyle.unicode, true, buf[0..]).?, "└── "));
    expect(strEq(root.formatPrefix(TreeStyle.ascii, false, buf[0..]).?, "|-- "));
    expect(strEq(root.formatPrefix(TreeStyle.ascii, true, buf[0..]).?, "+-- "));
    expect(strEq(root.formatPrefix(TreeStyle.rounded, true, buf[0..]).?, "╰── "));

    // Depth 1 (non-last) -> Depth 2 (last) -> Depth 3
    const p1 = root.push(false).?;
    const p2 = p1.push(true).?;
    expect(p2.depth == 2);
    expect(!p2.isAncestorLast(0));
    expect(p2.isAncestorLast(1));

    expect(strEq(p2.formatPrefix(TreeStyle.unicode, false, buf[0..]).?, "│       ├── "));
    expect(strEq(p2.formatPrefix(TreeStyle.ascii, true, buf[0..]).?, "|       +-- "));
    expect(strEq(p2.formatPrefix(TreeStyle.rounded, true, buf[0..]).?, "│       ╰── "));

    // Pop restores previous depth and clears popped bit
    const popped = p2.pop();
    expect(popped.depth == 1);
    expect(!popped.isAncestorLast(0));
    expect(root.pop().depth == 0);
});

test("renderFlatTree renders nested dependency tree in Unicode style", () => {
    const nodes: [7]TreeNode = [
        TreeNode { depth: 0, label: "my-app@1.0.0" },
        TreeNode { depth: 1, label: "cap-http@0.2.0" },
        TreeNode { depth: 2, label: "cap-net@0.1.0" },
        TreeNode { depth: 2, label: "cap-tls@0.1.5" },
        TreeNode { depth: 3, label: "boringssl@1.0.0" },
        TreeNode { depth: 1, label: "cap-json@0.3.0" },
        TreeNode { depth: 2, label: "simd-utf8@0.1.0" },
    ];

    let buf: [512]u8 = [0; 512];
    const rendered = renderFlatTree(nodes[0..], TreeStyle.unicode, buf[0..]).?;
    const expected =
        "my-app@1.0.0\n" ++
        "├── cap-http@0.2.0\n" ++
        "│   ├── cap-net@0.1.0\n" ++
        "│   └── cap-tls@0.1.5\n" ++
        "│       └── boringssl@1.0.0\n" ++
        "└── cap-json@0.3.0\n" ++
        "    └── simd-utf8@0.1.0";
    expect(strEq(rendered, expected));
});

test("renderFlatTree renders ASCII and Rounded styles accurately", () => {
    const nodes: [5]TreeNode = [
        TreeNode { depth: 0, label: "src" },
        TreeNode { depth: 1, label: "lib" },
        TreeNode { depth: 2, label: "index.cp" },
        TreeNode { depth: 1, label: "main.cp" },
        TreeNode { depth: 2, label: "helper.cp" },
    ];

    let buf: [256]u8 = [0; 256];

    const rendered_ascii = renderFlatTree(nodes[0..], TreeStyle.ascii, buf[0..]).?;
    const expected_ascii =
        "src\n" ++
        "|-- lib\n" ++
        "|   +-- index.cp\n" ++
        "+-- main.cp\n" ++
        "    +-- helper.cp";
    expect(strEq(rendered_ascii, expected_ascii));

    const rendered_rounded = renderFlatTree(nodes[0..], TreeStyle.rounded, buf[0..]).?;
    const expected_rounded =
        "src\n" ++
        "├── lib\n" ++
        "│   ╰── index.cp\n" ++
        "╰── main.cp\n" ++
        "    ╰── helper.cp";
    expect(strEq(rendered_rounded, expected_rounded));
});

test("TreePrefix 64-level nesting capacity limit and boundary behavior", () => {
    let p = TreePrefix.init();
    let i: u8 = 0;
    while (i < 64) {
        const next_opt = p.push((i % 2) == 1);
        expect(next_opt != null);
        p = next_opt.?;
        i += 1;
    }
    expect(p.depth == 64);
    expect(!p.isAncestorLast(0));
    expect(p.isAncestorLast(1));
    expect(p.isAncestorLast(63));

    // Pushing 65th level returns null
    expect(p.push(false) == null);

    // Format deep prefix (64 * 4 + 4 = 260 bytes in ASCII)
    let buf: [300]u8 = [0; 300];
    const deep = p.formatPrefix(TreeStyle.ascii, true, buf[0..]).?;
    expect(deep.len == 260);
    // Exact boundary fails when 1 byte short
    expect(p.formatPrefix(TreeStyle.ascii, true, buf[0..259]) == null);
});

test("renderFlatTree handles empty list, multi-root forest, deep invalid depth, and small buffer", () => {
    let buf: [128]u8 = [0; 128];

    // Empty input returns empty slice
    const empty: [0]TreeNode = [];
    expect(strEq(renderFlatTree(empty[0..], TreeStyle.unicode, buf[0..]).?, ""));

    // Multi-root forest resets ancestor state cleanly
    const forest: [4]TreeNode = [
        TreeNode { depth: 0, label: "rootA" },
        TreeNode { depth: 1, label: "childA" },
        TreeNode { depth: 0, label: "rootB" },
        TreeNode { depth: 1, label: "childB" },
    ];
    const expected_forest =
        "rootA\n" ++
        "+-- childA\n" ++
        "rootB\n" ++
        "+-- childB";
    expect(strEq(renderFlatTree(forest[0..], TreeStyle.ascii, buf[0..]).?, expected_forest));

    // Buffer too small returns null
    let tiny: [6]u8 = [0; 6];
    expect(renderFlatTree(forest[0..], TreeStyle.ascii, tiny[0..]) == null);

    // Depth > 64 is rejected safely
    const too_deep: [1]TreeNode = [TreeNode { depth: 65, label: "overflow" }];
    expect(renderFlatTree(too_deep[0..], TreeStyle.ascii, buf[0..]) == null);
});
