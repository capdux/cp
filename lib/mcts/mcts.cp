// Origin: Pure-CP Stdlib (Zero-Alloc Monte Carlo Tree Search MCTS with UCT & PUCT over Fixed Node Pool) | Created: 2026-11-22

import std from "std";

/// Sentinel node index indicating no parent / no child / no sibling.
pub const MCTS_NO_NODE: u32 = 0xFFFFFFFF;

const SQRT2: f64 = 1.41421356237309504880;
const LN2: f64 = 0.69314718055994530942;

fn isFiniteF64(x: f64): bool {
    return (x == x) && ((x - x) == 0.0);
}

/// Pure-CP natural logarithm for `x > 0` via range reduction and atanh series.
fn lnF64(x: f64): f64 {
    if (x <= 0.0) {
        return 0.0;
    }
    let m = x;
    let k: i32 = 0;
    while (m >= 1.5) {
        m = m * 0.5;
        k += 1;
    }
    while (m < 0.75) {
        m = m * 2.0;
        k -= 1;
    }
    const t = (m - 1.0) / (m + 1.0);
    const t2 = t * t;
    let term = t;
    let sum: f64 = 0.0;
    let denom: f64 = 1.0;
    let iter: usize = 0;
    while (iter < 10) {
        sum = sum + term / denom;
        term = term * t2;
        denom = denom + 2.0;
        iter += 1;
    }
    return 2.0 * sum + (k as f64) * LN2;
}

/// Single node inside the fixed-capacity MCTS tree pool using left-child / right-sibling links.
pub struct MctsNode {
    parent: u32;
    first_child: u32;
    next_sibling: u32;
    action: i32;
    player: i8;
    visits: u32;
    total_value: f64;
    prior: f64;
    is_expanded: bool;
    is_terminal: bool;
}

/// Configuration for UCT (UCB1) or AlphaZero-style PUCT selection policy.
pub struct MctsConfig {
    exploration_c: f64;
    use_puct: bool;
    c_puct: f64;

    /// Returns default UCT configuration (`exploration_c = sqrt(2)`).
    static fn defaultUct(): MctsConfig {
        return MctsConfig {
            exploration_c: SQRT2,
            use_puct: false,
            c_puct: 1.5,
        };
    }

    /// Returns default PUCT configuration (`c_puct = 1.5`).
    static fn defaultPuct(): MctsConfig {
        return MctsConfig {
            exploration_c: SQRT2,
            use_puct: true,
            c_puct: 1.5,
        };
    }
}

/// Zero-allocation Monte Carlo Tree Search (MCTS) tree backed by a caller-owned `MctsNode` pool.
pub struct MctsTree {
    nodes: []MctsNode;
    count: usize;
    root_player: i8;
    cfg: MctsConfig;

    /// Initializes a new MCTS tree with a single root node at index `0`.
    /// Returns `null` if `nodes.len == 0` or `cfg` contains negative/non-finite exploration constants.
    static fn init(nodes: []MctsNode, root_player: i8, cfg: MctsConfig): MctsTree? {
        if (nodes.len == 0) {
            return null;
        }
        if (!isFiniteF64(cfg.exploration_c) || cfg.exploration_c < 0.0) {
            return null;
        }
        if (!isFiniteF64(cfg.c_puct) || cfg.c_puct < 0.0) {
            return null;
        }

        nodes[0] = MctsNode {
            parent: MCTS_NO_NODE,
            first_child: MCTS_NO_NODE,
            next_sibling: MCTS_NO_NODE,
            action: -1,
            player: root_player,
            visits: 0,
            total_value: 0.0,
            prior: 1.0,
            is_expanded: false,
            is_terminal: false,
        };

        return MctsTree {
            nodes: nodes,
            count: 1,
            root_player: root_player,
            cfg: cfg,
        };
    }

    /// Returns the number of allocated nodes currently in the tree (including root).
    fn nodeCount(): usize {
        return _.count;
    }

    /// Returns the maximum node capacity of the backing pool.
    fn capacity(): usize {
        return _.nodes.len;
    }

    /// Returns the total visit count of the root node.
    fn rootVisits(): u32 {
        return _.nodes[0].visits;
    }

    /// Computes the UCB1 score `Q + c * sqrt(ln(N_parent) / N_child)` for `child_idx`.
    /// Unvisited children return `+inf` to guarantee exploration.
    fn ucb1Score(child_idx: u32, parent_visits: u32): f64 {
        if ((child_idx as usize) >= _.count) {
            return 0.0 - std.math.inf(f64);
        }
        const child = _.nodes[child_idx as usize];
        if (child.visits == 0) {
            return std.math.inf(f64);
        }
        const q = child.total_value / (child.visits as f64);
        const pv: f64 = if (parent_visits > 1) (parent_visits as f64) else 1.0;
        const explore = _.cfg.exploration_c * std.math.sqrt(lnF64(pv) / (child.visits as f64));
        return q + explore;
    }

    /// Computes the AlphaZero-style PUCT score `Q + c_puct * P * sqrt(N_parent) / (1 + N_child)`.
    fn puctScore(child_idx: u32, parent_visits: u32): f64 {
        if ((child_idx as usize) >= _.count) {
            return 0.0 - std.math.inf(f64);
        }
        const child = _.nodes[child_idx as usize];
        const q: f64 = if (child.visits > 0) child.total_value / (child.visits as f64) else 0.0;
        const pv: f64 = if (parent_visits > 0) (parent_visits as f64) else 1.0;
        const explore = _.cfg.c_puct * child.prior * std.math.sqrt(pv) / ((1 + child.visits) as f64);
        return q + explore;
    }

    /// Descends from root (`0`) along highest UCB1/PUCT child scores until reaching
    /// an unexpanded or terminal leaf node.
    fn selectLeaf(): u32 {
        let cur: u32 = 0;
        let depth: usize = 0;
        while (depth < _.count) {
            const node = _.nodes[cur as usize];
            if (node.is_terminal || !node.is_expanded || node.first_child == MCTS_NO_NODE) {
                return cur;
            }
            let best_child = MCTS_NO_NODE;
            let best_score = 0.0 - std.math.inf(f64);
            let ch = node.first_child;
            let guard: usize = 0;
            while (ch != MCTS_NO_NODE && guard < _.count) {
                const sc = if (_.cfg.use_puct) _.puctScore(ch, node.visits) else _.ucb1Score(ch, node.visits);
                if (best_child == MCTS_NO_NODE || sc > best_score) {
                    best_score = sc;
                    best_child = ch;
                }
                ch = _.nodes[ch as usize].next_sibling;
                guard += 1;
            }
            if (best_child == MCTS_NO_NODE) {
                return cur;
            }
            cur = best_child;
            depth += 1;
        }
        return cur;
    }

    /// Expands `node_idx` by attaching child nodes for each action in `actions`.
    ///
    /// - If `priors.len == 0`, assigns uniform prior `1 / actions.len`.
    /// - If `priors.len >= actions.len`, uses normalized non-negative priors.
    /// - Stops safely without out-of-bounds access if the fixed node pool becomes full.
    /// Returns the number of child nodes actually allocated and attached.
    fn expandNode(
        *_,
        node_idx: u32,
        actions: []const i32,
        priors: []const f64,
        next_player: i8,
    ): usize {
        if ((node_idx as usize) >= _.count) {
            return 0;
        }
        if (_.nodes[node_idx as usize].is_terminal || _.nodes[node_idx as usize].is_expanded) {
            return 0;
        }
        if (actions.len == 0) {
            _.nodes[node_idx as usize].is_expanded = true;
            return 0;
        }
        if (priors.len > 0 && priors.len < actions.len) {
            return 0;
        }

        // Compute prior normalization factor if priors are supplied
        let prior_sum: f64 = 0.0;
        if (priors.len >= actions.len) {
            let pi: usize = 0;
            while (pi < actions.len) {
                const pv = priors[pi];
                if (isFiniteF64(pv) && pv > 0.0) {
                    prior_sum = prior_sum + pv;
                }
                pi += 1;
            }
        }
        const uniform_prior = 1.0 / (actions.len as f64);

        let attached: usize = 0;
        let tail_child = MCTS_NO_NODE;
        let i: usize = 0;
        while (i < actions.len && _.count < _.nodes.len) {
            const child_idx = _.count as u32;
            _.count += 1;

            let p_val = uniform_prior;
            if (priors.len >= actions.len && prior_sum > 0.0) {
                const raw = priors[i];
                p_val = if (isFiniteF64(raw) && raw > 0.0) raw / prior_sum else 0.0;
            }

            _.nodes[child_idx as usize] = MctsNode {
                parent: node_idx,
                first_child: MCTS_NO_NODE,
                next_sibling: MCTS_NO_NODE,
                action: actions[i],
                player: next_player,
                visits: 0,
                total_value: 0.0,
                prior: p_val,
                is_expanded: false,
                is_terminal: false,
            };

            if (tail_child == MCTS_NO_NODE) {
                _.nodes[node_idx as usize].first_child = child_idx;
            } else {
                _.nodes[tail_child as usize].next_sibling = child_idx;
            }
            tail_child = child_idx;
            attached += 1;
            i += 1;
        }

        if (attached > 0) {
            _.nodes[node_idx as usize].is_expanded = true;
        }
        return attached;
    }

    /// Marks `node_idx` as a terminal state so selection stops at this node.
    fn markTerminal(*_, node_idx: u32) {
        if ((node_idx as usize) < _.count) {
            _.nodes[node_idx as usize].is_terminal = true;
        }
    }

    /// Backpropagates a rollout/evaluation `value_for_root` from `leaf_idx` up to the root (`0`).
    ///
    /// For each non-root node `cur`, adds `+value_for_root` if its parent's acting player equals
    /// `root_player`, and `-value_for_root` otherwise — seamlessly supporting both single-agent
    /// planning (all nodes share `root_player`) and two-player zero-sum games (alternating players).
    fn backpropagate(*_, leaf_idx: u32, value_for_root: f64) {
        if ((leaf_idx as usize) >= _.count || !isFiniteF64(value_for_root)) {
            return;
        }
        let cur = leaf_idx;
        let guard: usize = 0;
        while (cur != MCTS_NO_NODE && guard < _.count) {
            _.nodes[cur as usize].visits += 1;
            const par = _.nodes[cur as usize].parent;
            if (par == MCTS_NO_NODE) {
                _.nodes[cur as usize].total_value = _.nodes[cur as usize].total_value + value_for_root;
            } else {
                const par_player = _.nodes[par as usize].player;
                if (par_player == _.root_player) {
                    _.nodes[cur as usize].total_value = _.nodes[cur as usize].total_value + value_for_root;
                } else {
                    _.nodes[cur as usize].total_value = _.nodes[cur as usize].total_value - value_for_root;
                }
            }
            cur = par;
            guard += 1;
        }
    }

    /// Returns the root action with the highest visit count (robust child selection), or `null` if unexpanded.
    fn bestActionByVisits(): i32? {
        let ch = _.nodes[0].first_child;
        if (ch == MCTS_NO_NODE) {
            return null;
        }
        let best_action: i32 = _.nodes[ch as usize].action;
        let best_visits: u32 = _.nodes[ch as usize].visits;
        let guard: usize = 0;
        while (ch != MCTS_NO_NODE && guard < _.count) {
            const node = _.nodes[ch as usize];
            if (node.visits > best_visits) {
                best_visits = node.visits;
                best_action = node.action;
            }
            ch = node.next_sibling;
            guard += 1;
        }
        return best_action;
    }

    /// Returns the root action with the highest mean value `total_value / visits` among visited children,
    /// or `null` if no root child has been visited.
    fn bestActionByMeanValue(): i32? {
        let ch = _.nodes[0].first_child;
        let found = false;
        let best_action: i32 = 0;
        let best_mean: f64 = 0.0;
        let guard: usize = 0;
        while (ch != MCTS_NO_NODE && guard < _.count) {
            const node = _.nodes[ch as usize];
            if (node.visits > 0) {
                const mean = node.total_value / (node.visits as f64);
                if (!found || mean > best_mean) {
                    found = true;
                    best_mean = mean;
                    best_action = node.action;
                }
            }
            ch = node.next_sibling;
            guard += 1;
        }
        if (!found) {
            return null;
        }
        return best_action;
    }

    /// Exports action IDs, visit counts, and mean values for all root children into caller buffers.
    /// Returns the number of entries written (clamped to the shortest output slice length).
    fn rootActionStats(
        actions_out: []i32,
        visits_out: []u32,
        mean_vals_out: []f64,
    ): usize {
        let cap = actions_out.len;
        if (visits_out.len < cap) {
            cap = visits_out.len;
        }
        if (mean_vals_out.len < cap) {
            cap = mean_vals_out.len;
        }
        let written: usize = 0;
        let ch = _.nodes[0].first_child;
        let guard: usize = 0;
        while (ch != MCTS_NO_NODE && written < cap && guard < _.count) {
            const node = _.nodes[ch as usize];
            actions_out[written] = node.action;
            visits_out[written] = node.visits;
            mean_vals_out[written] = if (node.visits > 0) node.total_value / (node.visits as f64) else 0.0;
            written += 1;
            ch = node.next_sibling;
            guard += 1;
        }
        return written;
    }
}
