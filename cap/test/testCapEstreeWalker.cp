// cap/test/testCapEstreeWalker.cp
// Comprehensive unit tests for @cap/estree-walker

import { test, expect_eq } from "cap:test";
import {
    ESTW_NONE,
    EstreeNodeKind,
    WalkPhase,
    EstreeArena,
    EstreeWalker,
} from "../../packages/estree-walker/src/index.cp";

fn main() {
    test("1. @cap/estree-walker — DFS Enter/Leave ordering and depths", () => {
        let tree = EstreeArena.init();
        const prog = tree.addNode(.Program, "root");
        const fn_decl = tree.addNode(.FunctionDeclaration, "add");
        const block = tree.addNode(.BlockStatement, "body");
        const ret = tree.addNode(.ReturnStatement, "ret");
        const id_a = tree.addNode(.Identifier, "a");

        expect_eq(tree.appendChild(prog, fn_decl), true);
        expect_eq(tree.appendChild(fn_decl, block), true);
        expect_eq(tree.appendChild(block, ret), true);
        expect_eq(tree.appendChild(ret, id_a), true);

        let walker = EstreeWalker.init(prog);
        let enters: usize = 0;
        let leaves: usize = 0;
        let max_depth: usize = 0;

        while (ev := walker.nextEvent(tree)) {
            if (ev.phase == .Enter) {
                enters += 1;
                if (ev.depth > max_depth) max_depth = ev.depth;
            } else {
                leaves += 1;
            }
        }
        expect_eq(enters, 5 as usize);
        expect_eq(leaves, 5 as usize);
        expect_eq(max_depth, 4 as usize);
    });

    test("2. @cap/estree-walker — multi-child sibling traversal order", () => {
        let tree = EstreeArena.init();
        const prog = tree.addNode(.Program, "root");
        const call = tree.addNode(.CallExpression, "fn");
        const arg1 = tree.addNode(.Literal, "1");
        const arg2 = tree.addNode(.Literal, "2");
        const arg3 = tree.addNode(.Literal, "3");

        * = tree.appendChild(prog, call);
        * = tree.appendChild(call, arg1);
        * = tree.appendChild(call, arg2);
        * = tree.appendChild(call, arg3);

        let walker = EstreeWalker.init(prog);
        let order: [8]i32 = undefined;
        let idx: usize = 0;
        while (ev := walker.nextEvent(tree)) {
            if (ev.phase == .Enter) {
                order[idx] = ev.node_id;
                idx += 1;
            }
        }
        expect_eq(idx, 5 as usize);
        expect_eq(order[0], prog);
        expect_eq(order[1], call);
        expect_eq(order[2], arg1);
        expect_eq(order[3], arg2);
        expect_eq(order[4], arg3);
    });

    test("3. @cap/estree-walker — skip() prunes subtree while preserving balanced Leave", () => {
        let tree = EstreeArena.init();
        const prog = tree.addNode(.Program, "root");
        const b1 = tree.addNode(.BlockStatement, "skip_me");
        const child = tree.addNode(.Identifier, "hidden");
        const b2 = tree.addNode(.VariableDeclaration, "kept");

        * = tree.appendChild(prog, b1);
        * = tree.appendChild(b1, child);
        * = tree.appendChild(prog, b2);

        let walker = EstreeWalker.init(prog);
        let visited_hidden = false;
        let visited_kept = false;
        let enters: usize = 0;
        let leaves: usize = 0;

        while (ev := walker.nextEvent(tree)) {
            if (ev.phase == .Enter) {
                enters += 1;
                if (ev.node_id == b1) {
                    walker.skip();
                }
                if (ev.node_id == child) visited_hidden = true;
                if (ev.node_id == b2) visited_kept = true;
            } else {
                leaves += 1;
            }
        }
        expect_eq(visited_hidden, false);
        expect_eq(visited_kept, true);
        expect_eq(enters, 3 as usize);
        expect_eq(leaves, 3 as usize);
    });

    test("4. @cap/estree-walker — removeNode detaches node from parent chain", () => {
        let tree = EstreeArena.init();
        const prog = tree.addNode(.Program, "root");
        const n1 = tree.addNode(.Identifier, "x");
        const n2 = tree.addNode(.Identifier, "y");
        const n3 = tree.addNode(.Identifier, "z");

        * = tree.appendChild(prog, n1);
        * = tree.appendChild(prog, n2);
        * = tree.appendChild(prog, n3);

        expect_eq(tree.removeNode(n2), true);
        expect_eq(tree.removeNode(n2), false);

        let walker = EstreeWalker.init(prog);
        let seen_y = false;
        let enters: usize = 0;
        while (ev := walker.nextEvent(tree)) {
            if (ev.phase == .Enter) {
                enters += 1;
                if (ev.node_id == n2) seen_y = true;
            }
        }
        expect_eq(seen_y, false);
        expect_eq(enters, 3 as usize);
    });

    test("5. @cap/estree-walker — replaceNode swaps node in-place preserving sibling order", () => {
        let tree = EstreeArena.init();
        const prog = tree.addNode(.Program, "root");
        const bin = tree.addNode(.BinaryExpression, "+");
        const left = tree.addNode(.Literal, "1");
        const right = tree.addNode(.Literal, "2");
        const folded = tree.addNode(.Literal, "3");

        * = tree.appendChild(prog, bin);
        * = tree.appendChild(bin, left);
        * = tree.appendChild(bin, right);

        expect_eq(tree.replaceNode(bin, folded), true);

        let walker = EstreeWalker.init(prog);
        let ids: [4]i32 = undefined;
        let k: usize = 0;
        while (ev := walker.nextEvent(tree)) {
            if (ev.phase == .Enter) {
                ids[k] = ev.node_id;
                k += 1;
            }
        }
        expect_eq(k, 2 as usize);
        expect_eq(ids[0], prog);
        expect_eq(ids[1], folded);
    });

    test("6. @cap/estree-walker — arena capacity & invalid id guards", () => {
        let tree = EstreeArena.init();
        let i: usize = 0;
        while (i < 64) : (i += 1) {
            const id = tree.addNode(.Identifier, "v");
            expect_eq(id, i32(i));
        }
        // 65th node exceeds 64-node fixed pool
        expect_eq(tree.addNode(.Identifier, "overflow"), ESTW_NONE);
        expect_eq(tree.getNode(-1) == null, true);
        expect_eq(tree.getNode(64) == null, true);
        expect_eq(tree.appendChild(0, 0), false);
    });
}
