// Source: cp/cap/test/test_mcts_production_deep.cp (Production deep tests for cap:mcts)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    MCTS_NO_NODE,
    MctsNode,
    MctsConfig,
    MctsTree,
} from "cap:ml";

// --- Tic-Tac-Toe helper for two-player zero-sum MCTS verification ---
// Board cells: 0 = empty, 1 = X, -1 = O
fn tttWinner(b: []const i8): i8 {
    const lines: [24]usize = [
        0, 1, 2,
        3, 4, 5,
        6, 7, 8,
        0, 3, 6,
        1, 4, 7,
        2, 5, 8,
        0, 4, 8,
        2, 4, 6,
    ];
    let i: usize = 0;
    while (i < 8) {
        const a = b[lines[i * 3 + 0]];
        if (a != 0 && a == b[lines[i * 3 + 1]] && a == b[lines[i * 3 + 2]]) {
            return a;
        }
        i += 1;
    }
    return 0;
}

fn tttIsFull(b: []const i8): bool {
    let i: usize = 0;
    while (i < 9) {
        if (b[i] == 0) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn nextRng(state: *u64): u64 {
    state.* = state.* +% 0x9e3779b97f4a7c15;
    let z = state.*;
    z = (z ^ (z >> 30)) *% 0xbf58476d1ce4e5b9;
    z = (z ^ (z >> 27)) *% 0x94d049bb133111eb;
    return z ^ (z >> 31);
}

fn flipPlayer(p: i8): i8 {
    return if (p == 1) (-1 as i8) else (1 as i8);
}

fn tttRandomPlayout(board_in: []const i8, turn_in: i8, root_player: i8, rng: *u64): f64 {
    let b: [9]i8 = std.mem.zeroes([9]i8);
    let i: usize = 0;
    while (i < 9) {
        b[i] = board_in[i];
        i += 1;
    }
    let turn = turn_in;
    while (true) {
        const w = tttWinner(b[0..]);
        if (w != 0) {
            return if (w == root_player) 1.0 else -1.0;
        }
        if (tttIsFull(b[0..])) {
            return 0.0;
        }
        let legal: [9]usize = std.mem.zeroes([9]usize);
        let cnt: usize = 0;
        i = 0;
        while (i < 9) {
            if (b[i] == 0) {
                legal[cnt] = i;
                cnt += 1;
            }
            i += 1;
        }
        const pick = legal[(nextRng(rng) as usize) % cnt];
        b[pick] = turn;
        turn = flipPlayer(turn);
    }
    return 0.0;
}

fn runTttMcts(root_board: []const i8, root_player: i8, sims: usize, seed: u64): i32 {
    let pool: [1024]MctsNode = std.mem.zeroes([1024]MctsNode);
    let tree = MctsTree.init(pool[0..], root_player, MctsConfig.defaultUct()) ?? unreachable;
    let rng = seed;
    const no_priors: [0]f64 = [];

    let sim: usize = 0;
    while (sim < sims) {
        const leaf = tree.selectLeaf();

        // Reconstruct path from root to leaf
        let path: [16]u32 = std.mem.zeroes([16]u32);
        let plen: usize = 0;
        let cur = leaf;
        while (cur != MCTS_NO_NODE && cur != 0 && plen < 16) {
            path[plen] = cur;
            plen += 1;
            cur = tree.nodes[cur as usize].parent;
        }

        let b: [9]i8 = std.mem.zeroes([9]i8);
        let k: usize = 0;
        while (k < 9) {
            b[k] = root_board[k];
            k += 1;
        }
        let turn = root_player;
        let rev = plen;
        while (rev > 0) {
            rev -= 1;
            const act = tree.nodes[path[rev] as usize].action;
            b[act as usize] = turn;
            turn = flipPlayer(turn);
        }

        const w = tttWinner(b[0..]);
        if (w != 0 || tttIsFull(b[0..])) {
            tree.markTerminal(leaf);
            const val: f64 = if (w == root_player) 1.0 else if (w == 0) 0.0 else -1.0;
            tree.backpropagate(leaf, val);
        } else {
            let acts: [9]i32 = std.mem.zeroes([9]i32);
            let acnt: usize = 0;
            k = 0;
            while (k < 9) {
                if (b[k] == 0) {
                    acts[acnt] = k as i32;
                    acnt += 1;
                }
                k += 1;
            }
            const added = tree.expandNode(leaf, acts[0..acnt], no_priors[0..], flipPlayer(turn));
            if (added > 0) {
                const first_ch = tree.nodes[leaf as usize].first_child;
                const ch_act = tree.nodes[first_ch as usize].action;
                b[ch_act as usize] = turn;
                const val = tttRandomPlayout(b[0..], flipPlayer(turn), root_player, &rng);
                tree.backpropagate(first_ch, val);
            } else {
                const val = tttRandomPlayout(b[0..], turn, root_player, &rng);
                tree.backpropagate(leaf, val);
            }
        }
        sim += 1;
    }

    return tree.bestActionByVisits() ?? -1;
}

test("1. Single-agent 2-level decision tree UCB1 converges to optimal action & rootActionStats conserves visits", () => {
    let pool: [64]MctsNode = std.mem.zeroes([64]MctsNode);
    let tree = MctsTree.init(pool[0..], 1, MctsConfig.defaultUct()) ?? unreachable;
    const root_acts: [3]i32 = [0, 1, 2];
    const sub_acts: [2]i32 = [10, 11];
    const empty_p: [0]f64 = [];

    // First simulation expands root
    expect_eq(tree.selectLeaf(), 0 as u32);
    expect_eq(tree.expandNode(0, root_acts[0..], empty_p[0..], 1), 3 as usize);
    tree.backpropagate(0, 0.0);

    // Rewards:
    // action 0 -> sub 10: 0.2, sub 11: 0.25
    // action 1 -> sub 10: 0.95 (optimal!), sub 11: 0.4
    // action 2 -> sub 10: 0.5, sub 11: 0.55
    let sim: usize = 0;
    while (sim < 200) {
        const leaf = tree.selectLeaf();
        const node = tree.nodes[leaf as usize];
        if (node.parent == 0) {
            // Depth-1 node: expand its 2 sub-actions
            * = tree.expandNode(leaf, sub_acts[0..], empty_p[0..], 1);
            const ch = tree.nodes[leaf as usize].first_child;
            const r: f64 = if (node.action == 0) 0.2 else if (node.action == 1) 0.95 else 0.5;
            tree.backpropagate(ch, r);
        } else {
            // Depth-2 leaf
            tree.markTerminal(leaf);
            const par_act = tree.nodes[node.parent as usize].action;
            let r: f64 = 0.0;
            if (par_act == 0) {
                r = if (node.action == 10) 0.2 else 0.25;
            } else if (par_act == 1) {
                r = if (node.action == 10) 0.95 else 0.4;
            } else {
                r = if (node.action == 10) 0.5 else 0.55;
            }
            tree.backpropagate(leaf, r);
        }
        sim += 1;
    }

    expect_eq(tree.bestActionByVisits() ?? unreachable, 1 as i32);
    expect_eq(tree.bestActionByMeanValue() ?? unreachable, 1 as i32);
    expect_eq(tree.rootVisits(), 201 as u32);

    // Verify visit conservation: sum(child_visits) == root_visits - 1
    let a_out: [4]i32 = std.mem.zeroes([4]i32);
    let v_out: [4]u32 = std.mem.zeroes([4]u32);
    let m_out: [4]f64 = std.mem.zeroes([4]f64);
    const n_stats = tree.rootActionStats(a_out[0..], v_out[0..], m_out[0..]);
    expect_eq(n_stats, 3 as usize);
    const child_visit_sum: u32 = v_out[0] + v_out[1] + v_out[2];
    expect_eq(child_visit_sum, tree.rootVisits() - 1);
    expect_eq(v_out[1] > v_out[0] && v_out[1] > v_out[2], true);
});

test("2. Tic-Tac-Toe two-player zero-sum endgame: 100% finds immediate winning move", () => {
    // X X .
    // O O .
    // . . .
    // X (player +1) to move: playing cell 2 wins immediately!
    const board: [9]i8 = [
        1, 1, 0,
        -1, -1, 0,
        0, 0, 0,
    ];
    const best = runTttMcts(board[0..], 1, 300, 42);
    expect_eq(best, 2 as i32);
});

test("3. Tic-Tac-Toe two-player zero-sum endgame: 100% blocks opponent's immediate winning threat", () => {
    // O . X
    // . O .
    // X . .
    // X (player +1) to move: O threatens diagonal 0-4-8; X MUST play cell 8 to block!
    const board: [9]i8 = [
        -1, 0, 1,
        0, -1, 0,
        1, 0, 0,
    ];
    const best = runTttMcts(board[0..], 1, 300, 1337);
    expect_eq(best, 8 as i32);
});

test("4. PUCT prior-guided search prioritizes high-prior branch under equal rewards", () => {
    let pool: [32]MctsNode = std.mem.zeroes([32]MctsNode);
    let tree = MctsTree.init(pool[0..], 1, MctsConfig.defaultPuct()) ?? unreachable;
    const actions: [3]i32 = [0, 1, 2];
    const priors: [3]f64 = [0.05, 0.10, 0.85];

    expect_eq(tree.expandNode(0, actions[0..], priors[0..], 1), 3 as usize);

    // First leaf selected must be action 2 (highest prior 0.85)
    const first_leaf = tree.selectLeaf();
    expect_eq(tree.nodes[first_leaf as usize].action, 2 as i32);

    // Under identical neutral reward 0.5, PUCT concentrates visits on high-prior action 2
    let sim: usize = 0;
    while (sim < 40) {
        const leaf = tree.selectLeaf();
        tree.backpropagate(leaf, 0.5);
        sim += 1;
    }

    expect_eq(tree.bestActionByVisits() ?? unreachable, 2 as i32);
    let a_out: [3]i32 = std.mem.zeroes([3]i32);
    let v_out: [3]u32 = std.mem.zeroes([3]u32);
    let m_out: [3]f64 = std.mem.zeroes([3]f64);
    expect_eq(tree.rootActionStats(a_out[0..], v_out[0..], m_out[0..]), 3 as usize);
    expect_eq(v_out[2] > v_out[1] && v_out[1] >= v_out[0], true);
});

test("5. Fixed node pool exhaustion degrades gracefully without out-of-bounds access", () => {
    let pool: [5]MctsNode = std.mem.zeroes([5]MctsNode);
    let tree = MctsTree.init(pool[0..], 1, MctsConfig.defaultUct()) ?? unreachable;
    expect_eq(tree.capacity(), 5 as usize);
    expect_eq(tree.nodeCount(), 1 as usize);

    const actions: [6]i32 = [10, 20, 30, 40, 50, 60];
    const empty_p: [0]f64 = [];

    // Pool only has 4 free slots left -> attaches exactly 4 children safely
    const attached = tree.expandNode(0, actions[0..], empty_p[0..], 1);
    expect_eq(attached, 4 as usize);
    expect_eq(tree.nodeCount(), 5 as usize);

    // Further expansion on child returns 0 without panic
    const child = tree.selectLeaf();
    expect_eq(tree.expandNode(child, actions[0..], empty_p[0..], 1), 0 as usize);
    tree.backpropagate(child, 1.0);
    expect_eq(tree.bestActionByVisits() ?? unreachable, 10 as i32);
});

test("6. Boundary guards: empty pool, invalid config, empty actions, mismatched priors & unexpanded tree", () => {
    let empty_pool: [0]MctsNode = [];
    let ok_pool: [8]MctsNode = std.mem.zeroes([8]MctsNode);

    expect_eq(MctsTree.init(empty_pool[0..], 1, MctsConfig.defaultUct()) == null, true);

    const bad_c = MctsConfig { exploration_c: -1.0, use_puct: false, c_puct: 1.5 };
    const bad_puct = MctsConfig { exploration_c: 1.4, use_puct: true, c_puct: std.math.nan(f64) };
    expect_eq(MctsTree.init(ok_pool[0..], 1, bad_c) == null, true);
    expect_eq(MctsTree.init(ok_pool[0..], 1, bad_puct) == null, true);

    let tree = MctsTree.init(ok_pool[0..], 1, MctsConfig.defaultUct()) ?? unreachable;
    expect_eq(tree.bestActionByVisits() == null, true);
    expect_eq(tree.bestActionByMeanValue() == null, true);

    // Mismatched priors length (0 < priors.len < actions.len) rejected
    const acts: [3]i32 = [1, 2, 3];
    const short_p: [2]f64 = [0.5, 0.5];
    expect_eq(tree.expandNode(0, acts[0..], short_p[0..], 1), 0 as usize);

    // Empty actions list marks expanded cleanly with 0 children
    const empty_a: [0]i32 = [];
    const empty_p: [0]f64 = [];
    expect_eq(tree.expandNode(0, empty_a[0..], empty_p[0..], 1), 0 as usize);
    expect_eq(tree.selectLeaf(), 0 as u32);
    expect_eq(MCTS_NO_NODE, 0xFFFFFFFF as u32);
});
