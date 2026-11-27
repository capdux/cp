import { test, expect_eq } from "cap:test";
import {
    UNIST_NONE,
    newUnistTree,
} from "../../packages/unist-util-visit/src/index.cp";

fn main() {
    test("1. @cap/unist-util-visit — builds tree and visits all nodes in pre-order DFS", () => {
        let tree = newUnistTree();
        const root = tree.addNode("root", "", UNIST_NONE);
        const para = tree.addNode("paragraph", "", root);
        const t1 = tree.addNode("text", "hello ", para);
        const em = tree.addNode("emphasis", "", para);
        const t2 = tree.addNode("text", "world", em);

        let visited: [16]u32 = [0; 16];
        const n = tree.visitPreOrder(root, "", visited[0..16]);
        expect_eq(n, 5 as usize);
        expect_eq(visited[0], root);
        expect_eq(visited[1], para);
        expect_eq(visited[2], t1);
        expect_eq(visited[3], em);
        expect_eq(visited[4], t2);
    });

    test("2. @cap/unist-util-visit — filters visited nodes by kind", () => {
        let tree = newUnistTree();
        const root = tree.addNode("root", "", UNIST_NONE);
        const h1 = tree.addNode("heading", "", root);
        const ht = tree.addNode("text", "Title", h1);
        const p = tree.addNode("paragraph", "", root);
        const pt = tree.addNode("text", "Body", p);

        let texts: [8]u32 = [0; 8];
        const n = tree.visitPreOrder(root, "text", texts[0..8]);
        expect_eq(n, 2 as usize);
        expect_eq(texts[0], ht);
        expect_eq(texts[1], pt);
        expect_eq(tree.nodes[texts[0] as usize].value, "Title");
        expect_eq(tree.nodes[texts[1] as usize].value, "Body");
    });

    test("3. @cap/unist-util-visit — getAncestors returns root-to-parent chain", () => {
        let tree = newUnistTree();
        const root = tree.addNode("root", "", UNIST_NONE);
        const list = tree.addNode("list", "", root);
        const item = tree.addNode("listItem", "", list);
        const txt = tree.addNode("text", "one", item);

        let anc: [8]u32 = [0; 8];
        const n = tree.getAncestors(txt, anc[0..8]);
        expect_eq(n, 3 as usize);
        expect_eq(anc[0], root);
        expect_eq(anc[1], list);
        expect_eq(anc[2], item);
    });

    test("4. @cap/unist-util-visit — isNode matches kind and wildcard empty string", () => {
        let tree = newUnistTree();
        const r = tree.addNode("code", "fn main() {}", UNIST_NONE);
        expect_eq(tree.isNode(r, "code"), true);
        expect_eq(tree.isNode(r, "text"), false);
        expect_eq(tree.isNode(r, ""), true);
        expect_eq(tree.isNode(99, "code"), false);
    });

    test("5. @cap/unist-util-visit — visits subtree rooted at non-root node", () => {
        let tree = newUnistTree();
        const root = tree.addNode("root", "", UNIST_NONE);
        const b1 = tree.addNode("blockquote", "", root);
        const t1 = tree.addNode("text", "quoted", b1);
        const b2 = tree.addNode("paragraph", "", root);
        * = tree.addNode("text", "outside", b2);

        let out: [8]u32 = [0; 8];
        const n = tree.visitPreOrder(b1, "text", out[0..8]);
        expect_eq(n, 1 as usize);
        expect_eq(out[0], t1);
    });

    test("6. @cap/unist-util-visit — handles empty tree and out-of-bounds root safely", () => {
        let tree = newUnistTree();
        let out: [4]u32 = [0; 4];
        expect_eq(tree.visitPreOrder(0, "", out[0..4]), 0 as usize);
        expect_eq(tree.getAncestors(0, out[0..4]), 0 as usize);
    });
}
