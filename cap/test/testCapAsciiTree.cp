// cap/test/testCapAsciiTree.cp
// Comprehensive unit tests for @cap/ascii-tree

import { test, expect_eq } from "cap:test";
import {
    TreeStyle,
    TreeBuilder,
    renderTree,
    renderTreeWith,
} from "../../packages/archy/src/index.cp";

fn main() {
    test("1. @cap/ascii-tree — Unicode tree rendering with nested children", () => {
        let t = TreeBuilder.init("capdux@1.0.0");
        const n1 = t.addChild(0 as usize, "@cap/lru@1.0.0") ?? (0 as usize);
        * = t.addChild(n1, "src/index.cp");
        * = t.addChild(n1, "cap.json");
        * = t.addChild(0 as usize, "@cap/redact@1.0.0");
        expect_eq(t.nodeCount(), 5 as usize);

        let out: [512]u8 = undefined;
        const rendered = renderTree(&t, out[0..512]);
        const expected = "capdux@1.0.0\n├── @cap/lru@1.0.0\n│   ├── src/index.cp\n│   └── cap.json\n└── @cap/redact@1.0.0";
        expect_eq(rendered, expected);
    });

    test("2. @cap/ascii-tree — ASCII style rendering", () => {
        let t = TreeBuilder.init("root");
        const a = t.addChild(0 as usize, "a") ?? (0 as usize);
        * = t.addChild(a, "a1");
        * = t.addChild(0 as usize, "b");

        let out: [512]u8 = undefined;
        const rendered = renderTreeWith(&t, TreeStyle.Ascii, out[0..512]);
        expect_eq(rendered, "root\n|-- a\n|   `-- a1\n`-- b");
    });

    test("3. @cap/ascii-tree — Rounded style & deep last-branch spacing", () => {
        let t = TreeBuilder.init("app");
        const only = t.addChild(0 as usize, "only") ?? (0 as usize);
        * = t.addChild(only, "leaf");

        let out: [512]u8 = undefined;
        const rendered = renderTreeWith(&t, TreeStyle.Rounded, out[0..512]);
        expect_eq(rendered, "app\n╰── only\n    ╰── leaf");
    });

    test("4. @cap/ascii-tree — Root-only tree & invalid parent guard", () => {
        let t = TreeBuilder.init("solo");
        expect_eq(t.addChild(99 as usize, "bad") == null, true);

        let out: [128]u8 = undefined;
        expect_eq(renderTree(&t, out[0..128]), "solo");
    });
}
