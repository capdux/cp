import { test, expect_eq } from "cap:test";
import {
    GlyphPalette,
    TermTreeArena,
} from "../../packages/@cap/termtree/src/index.cp";

fn strEqSlice(a: []const u8, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

fn main() {
    test("1. @cap/termtree — single root node render & depth", () => {
        let tree = TermTreeArena.init();
        const root = tree.newRoot("root");
        expect_eq(root, 0);
        expect_eq(tree.nodeCount(), 1);
        expect_eq(tree.maxDepth(root), 1);

        let buf: [128]u8 = undefined;
        const n = tree.render(root, buf[0..128]);
        expect_eq(strEqSlice(buf[0..n], "root\n"), true);
    });

    test("2. @cap/termtree — Unicode multi-level tree rendering", () => {
        let tree = TermTreeArena.init();
        const root = tree.newRoot("pkg");
        const src = tree.addChild(root, "src");
        * = tree.addChild(usize(src), "main.cp");
        * = tree.addChild(usize(src), "lib.cp");
        const tests = tree.addChild(root, "tests");
        * = tree.addChild(usize(tests), "unit.cp");

        expect_eq(tree.nodeCount(), 6);
        expect_eq(tree.maxDepth(root), 3);

        let buf: [512]u8 = undefined;
        const n = tree.render(root, buf[0..512]);
        const expected =
            "pkg\n" ++
            "├── src\n" ++
            "│   ├── main.cp\n" ++
            "│   └── lib.cp\n" ++
            "└── tests\n" ++
            "    └── unit.cp\n";
        expect_eq(strEqSlice(buf[0..n], expected), true);
    });

    test("3. @cap/termtree — ASCII glyph palette rendering", () => {
        let tree = TermTreeArena.init();
        const root = tree.newRoot("app");
        const a = tree.addChild(root, "a");
        * = tree.addChild(usize(a), "a1");
        * = tree.addChild(root, "b");

        let buf: [256]u8 = undefined;
        const n = tree.renderWithGlyphs(root, GlyphPalette.ascii(), buf[0..256]);
        const expected =
            "app\n" ++
            "|-- a\n" ++
            "|   `-- a1\n" ++
            "`-- b\n";
        expect_eq(strEqSlice(buf[0..n], expected), true);
    });

    test("4. @cap/termtree — deep chain maxDepth & last-skip indentation", () => {
        let tree = TermTreeArena.init();
        const r = tree.newRoot("L1");
        const l2 = tree.addChild(r, "L2");
        const l3 = tree.addChild(usize(l2), "L3");
        * = tree.addChild(usize(l3), "L4");

        expect_eq(tree.maxDepth(r), 4);
        let buf: [256]u8 = undefined;
        const n = tree.renderWithGlyphs(r, GlyphPalette.ascii(), buf[0..256]);
        const expected =
            "L1\n" ++
            "`-- L2\n" ++
            "    `-- L3\n" ++
            "        `-- L4\n";
        expect_eq(strEqSlice(buf[0..n], expected), true);
    });

    test("5. @cap/termtree — invalid parent / out-of-range root & buffer guard", () => {
        let tree = TermTreeArena.init();
        expect_eq(tree.addChild(99, "orphan"), -1);
        expect_eq(tree.maxDepth(0), 0);

        const r = tree.newRoot("long_root_name");
        let tiny: [4]u8 = undefined;
        expect_eq(tree.render(r, tiny[0..4]), 0);
        expect_eq(tree.render(10, tiny[0..4]), 0);
    });

    test("6. @cap/termtree — multiple independent trees in one tree", () => {
        let tree = TermTreeArena.init();
        const r1 = tree.newRoot("tree1");
        * = tree.addChild(r1, "c1");
        const r2 = tree.newRoot("tree2");
        * = tree.addChild(r2, "c2");
        * = tree.addChild(r2, "c3");

        expect_eq(tree.nodeCount(), 5);
        expect_eq(tree.maxDepth(r1), 2);
        expect_eq(tree.maxDepth(r2), 2);

        let b1: [128]u8 = undefined;
        const n1 = tree.renderWithGlyphs(r1, GlyphPalette.ascii(), b1[0..128]);
        expect_eq(strEqSlice(b1[0..n1], "tree1\n`-- c1\n"), true);

        let b2: [128]u8 = undefined;
        const n2 = tree.renderWithGlyphs(r2, GlyphPalette.ascii(), b2[0..128]);
        expect_eq(strEqSlice(b2[0..n2], "tree2\n|-- c2\n`-- c3\n"), true);
    });
}
