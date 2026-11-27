import { test, expect_eq } from "cap:test";
import {
    SelectorKind,
    AttrOp,
    AstNodeInfo,
    parseSelector,
    matchesSelector,
    queryAll,
} from "../../packages/esquery/src/index.cp";

fn makeSampleAst(): [7]AstNodeInfo {
    let nodes: [7]AstNodeInfo = [
        // 0: Program
        AstNodeInfo{ node_type: "Program", name: "", operator: "", parent_idx: -1, prev_sibling_idx: -1 },
        // 1: FunctionDeclaration (foo)
        AstNodeInfo{ node_type: "FunctionDeclaration", name: "foo", operator: "", parent_idx: 0, prev_sibling_idx: -1 },
        // 2: CallExpression (child of 1)
        AstNodeInfo{ node_type: "CallExpression", name: "", operator: "", parent_idx: 1, prev_sibling_idx: -1 },
        // 3: Identifier (bar, child of 2)
        AstNodeInfo{ node_type: "Identifier", name: "bar", operator: "", parent_idx: 2, prev_sibling_idx: -1 },
        // 4: BinaryExpression (+, child of 1, sibling after 2)
        AstNodeInfo{ node_type: "BinaryExpression", name: "", operator: "+", parent_idx: 1, prev_sibling_idx: 2 },
        // 5: Identifier (x, child of 4)
        AstNodeInfo{ node_type: "Identifier", name: "x", operator: "", parent_idx: 4, prev_sibling_idx: -1 },
        // 6: Literal (42, child of 4, sibling after 5)
        AstNodeInfo{ node_type: "Literal", name: "42", operator: "", parent_idx: 4, prev_sibling_idx: 5 },
    ];
    return nodes;
}

fn main() {
    test("1. @cap/esquery — NodeType and Wildcard selectors", () => {
        const nodes = makeSampleAst();
        let out: [16]usize = undefined;

        const n_id = queryAll("Identifier", nodes[0..7], out[0..16]);
        expect_eq(n_id, 2);
        expect_eq(out[0], 3);
        expect_eq(out[1], 5);

        const n_all = queryAll("*", nodes[0..7], out[0..16]);
        expect_eq(n_all, 7);

        const sel_id = parseSelector("Identifier");
        expect_eq(sel_id.ok, true);
        expect_eq(matchesSelector(sel_id, nodes[0..7], 3), true);
        expect_eq(matchesSelector(sel_id, nodes[0..7], 0), false);
    });

    test("2. @cap/esquery — Child (>) and Descendant (space) combinators", () => {
        const nodes = makeSampleAst();
        let out: [16]usize = undefined;

        const n_child = queryAll("CallExpression > Identifier", nodes[0..7], out[0..16]);
        expect_eq(n_child, 1);
        expect_eq(out[0], 3);

        const n_desc = queryAll("FunctionDeclaration Identifier", nodes[0..7], out[0..16]);
        expect_eq(n_desc, 2);
        expect_eq(out[0], 3);
        expect_eq(out[1], 5);

        const n_direct_miss = queryAll("FunctionDeclaration > Identifier", nodes[0..7], out[0..16]);
        expect_eq(n_direct_miss, 0);
    });

    test("3. @cap/esquery — Attribute selectors ([name], =, !=, <, >) & Compound", () => {
        const nodes = makeSampleAst();
        let out: [16]usize = undefined;

        const n_foo = queryAll("[name=\"foo\"]", nodes[0..7], out[0..16]);
        expect_eq(n_foo, 1);
        expect_eq(out[0], 1);

        const n_bin = queryAll("BinaryExpression[operator=\"+\"]", nodes[0..7], out[0..16]);
        expect_eq(n_bin, 1);
        expect_eq(out[0], 4);

        const n_neq = queryAll("Identifier[name!='x']", nodes[0..7], out[0..16]);
        expect_eq(n_neq, 1);
        expect_eq(out[0], 3);

        const n_exists = queryAll("[operator]", nodes[0..7], out[0..16]);
        expect_eq(n_exists, 1);
        expect_eq(out[0], 4);

        const n_gt = queryAll("Literal[name>10]", nodes[0..7], out[0..16]);
        expect_eq(n_gt, 1);
        expect_eq(out[0], 6);

        const n_lt = queryAll("Literal[name<10]", nodes[0..7], out[0..16]);
        expect_eq(n_lt, 0);
    });

    test("4. @cap/esquery — AdjacentSibling (+) combinator", () => {
        const nodes = makeSampleAst();
        let out: [16]usize = undefined;

        const n_sib1 = queryAll("CallExpression + BinaryExpression", nodes[0..7], out[0..16]);
        expect_eq(n_sib1, 1);
        expect_eq(out[0], 4);

        const n_sib2 = queryAll("Identifier + Literal", nodes[0..7], out[0..16]);
        expect_eq(n_sib2, 1);
        expect_eq(out[0], 6);

        const n_sib_miss = queryAll("Literal + Identifier", nodes[0..7], out[0..16]);
        expect_eq(n_sib_miss, 0);
    });

    test("5. @cap/esquery — Matches (A, B) selector list", () => {
        const nodes = makeSampleAst();
        let out: [16]usize = undefined;

        const n_multi = queryAll("CallExpression, Literal", nodes[0..7], out[0..16]);
        expect_eq(n_multi, 2);
        expect_eq(out[0], 2);
        expect_eq(out[1], 6);

        const n_multi2 = queryAll("FunctionDeclaration[name=\"foo\"], BinaryExpression[operator=\"+\"]", nodes[0..7], out[0..16]);
        expect_eq(n_multi2, 2);
        expect_eq(out[0], 1);
        expect_eq(out[1], 4);
    });

    test("6. @cap/esquery — Invalid selectors rejected gracefully", () => {
        const bad1 = parseSelector("");
        expect_eq(bad1.ok, false);

        const bad2 = parseSelector("Identifier >");
        expect_eq(bad2.ok, false);

        const bad3 = parseSelector("[name=\"unterminated");
        expect_eq(bad3.ok, false);

        const bad4 = parseSelector("Identifier,");
        expect_eq(bad4.ok, false);

        const nodes = makeSampleAst();
        expect_eq(matchesSelector(bad1, nodes[0..7], 0), false);
    });
}
