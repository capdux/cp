import { test, expect_eq } from "cap:test";
import {
    SPDX_NODE_LICENSE,
    SPDX_NODE_AND,
    SPDX_NODE_OR,
    SpdxAstNode,
    SpdxAst,
    parseSpdxExpression,
    isValidSpdxExpression,
    satisfiesSpdx,
} from "../../packages/@cap/spdx-expression-parse/src/index.cp";

fn eqStr(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/spdx-expression-parse — simple license, plus suffix, and LicenseRef", () => {
        let a1: SpdxAst = parseSpdxExpression("MIT+");
        expect_eq(a1.valid, true);
        expect_eq(a1.node_count, 1);
        let n1: SpdxAstNode = a1.nodes[a1.root as usize];
        expect_eq(n1.kind, SPDX_NODE_LICENSE);
        expect_eq(eqStr(n1.license, "MIT"), true);
        expect_eq(n1.plus, true);
        expect_eq(eqStr(n1.exception, ""), true);

        let a2: SpdxAst = parseSpdxExpression("LicenseRef-Custom");
        expect_eq(a2.valid, true);
        let n2: SpdxAstNode = a2.nodes[a2.root as usize];
        expect_eq(eqStr(n2.license, "LicenseRef-Custom"), true);
        expect_eq(n2.plus, false);
    });

    test("2. @cap/spdx-expression-parse — operator precedence: OR < AND", () => {
        // `MIT OR Apache-2.0 AND BSD-3-Clause` parses as `MIT OR (Apache-2.0 AND BSD-3-Clause)`.
        let ast: SpdxAst = parseSpdxExpression("MIT OR Apache-2.0 AND BSD-3-Clause");
        expect_eq(ast.valid, true);
        expect_eq(ast.node_count, 5);
        let root_node: SpdxAstNode = ast.nodes[ast.root as usize];
        expect_eq(root_node.kind, SPDX_NODE_OR);

        let left_node: SpdxAstNode = ast.nodes[root_node.left as usize];
        let right_node: SpdxAstNode = ast.nodes[root_node.right as usize];
        expect_eq(left_node.kind, SPDX_NODE_LICENSE);
        expect_eq(eqStr(left_node.license, "MIT"), true);
        expect_eq(right_node.kind, SPDX_NODE_AND);
    });

    test("3. @cap/spdx-expression-parse — parentheses override precedence", () => {
        let ast: SpdxAst = parseSpdxExpression("(MIT OR Apache-2.0) AND BSD-3-Clause");
        expect_eq(ast.valid, true);
        let root_node: SpdxAstNode = ast.nodes[ast.root as usize];
        expect_eq(root_node.kind, SPDX_NODE_AND);

        let left_node: SpdxAstNode = ast.nodes[root_node.left as usize];
        let right_node: SpdxAstNode = ast.nodes[root_node.right as usize];
        expect_eq(left_node.kind, SPDX_NODE_OR);
        expect_eq(right_node.kind, SPDX_NODE_LICENSE);
        expect_eq(eqStr(right_node.license, "BSD-3-Clause"), true);
    });

    test("4. @cap/spdx-expression-parse — WITH exception binding", () => {
        let ast: SpdxAst = parseSpdxExpression("GPL-2.0-only WITH Classpath-exception-2.0");
        expect_eq(ast.valid, true);
        expect_eq(ast.node_count, 1);
        let node: SpdxAstNode = ast.nodes[ast.root as usize];
        expect_eq(node.kind, SPDX_NODE_LICENSE);
        expect_eq(eqStr(node.license, "GPL-2.0-only"), true);
        expect_eq(eqStr(node.exception, "Classpath-exception-2.0"), true);
        expect_eq(satisfiesSpdx(ast, "GPL-2.0-only WITH Classpath-exception-2.0"), true);
        expect_eq(satisfiesSpdx(ast, "GPL-2.0-only"), false);
    });

    test("5. @cap/spdx-expression-parse — rejects invalid expressions and unknown licenses", () => {
        expect_eq(isValidSpdxExpression(""), false);
        expect_eq(isValidSpdxExpression("   "), false);
        expect_eq(isValidSpdxExpression("(MIT OR Apache-2.0"), false);
        expect_eq(isValidSpdxExpression("MIT AND OR Apache-2.0"), false);
        expect_eq(isValidSpdxExpression("MIT OR"), false);
        expect_eq(isValidSpdxExpression("UnknownLicense-9.9"), false);
        expect_eq(isValidSpdxExpression("MIT or Apache-2.0"), false);
    });

    test("6. @cap/spdx-expression-parse — satisfiesSpdx evaluates AND/OR semantics", () => {
        let or_ast: SpdxAst = parseSpdxExpression("MIT OR Apache-2.0");
        expect_eq(satisfiesSpdx(or_ast, "MIT"), true);
        expect_eq(satisfiesSpdx(or_ast, "Apache-2.0"), true);
        expect_eq(satisfiesSpdx(or_ast, "ISC"), false);

        let and_ast: SpdxAst = parseSpdxExpression("MIT AND Apache-2.0");
        expect_eq(satisfiesSpdx(and_ast, "MIT"), false);

        let same_and: SpdxAst = parseSpdxExpression("MIT AND MIT+");
        expect_eq(satisfiesSpdx(same_and, "MIT"), true);

        let bad_ast: SpdxAst = parseSpdxExpression("NotALicense");
        expect_eq(satisfiesSpdx(bad_ast, "NotALicense"), false);
    });
}
