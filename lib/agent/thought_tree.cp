// Origin: Pure-CP Stdlib (Zero-Heap Tree-of-Thoughts & Multi-Branch Agent Search State Manager) | Created: 2026-11-22
// Pure CP zero-heap-allocation Tree-of-Thoughts (ToT) search & conversation branching manager:
//   1. `THOUGHT_NO_PARENT`, `ThoughtNodeState` (`Open | Expanded | Pruned | Terminal`),
//      `ThoughtNode`, `ThoughtTreeStats`
//   2. `ThoughtTree`:
//      - Fixed-pool zero-heap tree construction (`setRoot`, `addChild`) with strict depth & state guards
//      - Deterministic Best-First frontier selection (`selectBestOpenNode`)
//      - O(N) non-recursive zero-stack subtree pruning (`pruneSubtree`)
//      - Parent-chain reward backpropagation (`backpropagateScore` updating `visit_count` & running-mean `agg_score`)
//      - Root-to-target forward path extraction (`extractPathToRoot`)

const std = @import("std");

/// Sentinel parent ID representing the root node of a `ThoughtTree`.
pub const THOUGHT_NO_PARENT: u32 = 0xFFFF_FFFF;

fn isFiniteF32(x: f32): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

/// Lifecycle state of a reasoning node in a `ThoughtTree`.
pub enum ThoughtNodeState {
    Open,
    Expanded,
    Pruned,
    Terminal,
}

/// Single reasoning node stored in caller-provided fixed-capacity pool.
pub struct ThoughtNode {
    active: bool = false;
    node_id: u32 = 0;
    parent_id: u32 = THOUGHT_NO_PARENT;
    depth: u32 = 0;
    local_score: f32 = 0.0;
    agg_score: f32 = 0.0;
    visit_count: u32 = 0;
    state: ThoughtNodeState = ThoughtNodeState.Open;
    thought_text: string = "";
}

/// Point-in-time telemetry snapshot for `ThoughtTree`.
pub struct ThoughtTreeStats {
    total_nodes: usize = 0;
    open_nodes: usize = 0;
    expanded_nodes: usize = 0;
    pruned_nodes: usize = 0;
    terminal_nodes: usize = 0;
    max_depth_reached: u32 = 0;
}

/// Zero-heap-allocation Tree-of-Thoughts reasoning search and multi-branch state manager.
pub struct ThoughtTree {
    nodes: []ThoughtNode;
    max_depth: u32 = 8;
    node_count: usize = 0;
    root_id: u32? = null;

    /// Initializes an empty thought tree over caller-provided `nodes_buf` with maximum depth `max_depth`.
    /// Returns `null` if `nodes_buf.len == 0`.
    static fn init(nodes_buf: *[]ThoughtNode, max_depth: u32): ThoughtTree? {
        if (nodes_buf.len == 0) return null;
        let i: usize = 0;
        while (i < nodes_buf.len) : (i += 1) {
            nodes_buf[i] = ThoughtNode{};
        }
        return ThoughtTree{
            nodes: nodes_buf,
            max_depth: max_depth,
            node_count: 0,
            root_id: null,
        };
    }

    /// Creates the root reasoning node (ID `0`, depth `0`) with initial `score`.
    /// Returns `null` if a root already exists, `score` is NaN/Inf, or the pool is full.
    fn setRoot(*_, thought_text: string, score: f32): u32? {
        if (_.root_id != null || !isFiniteF32(score) || _.node_count >= _.nodes.len) {
            return null;
        }
        _.nodes[0] = ThoughtNode{
            active: true,
            node_id: 0,
            parent_id: THOUGHT_NO_PARENT,
            depth: 0,
            local_score: score,
            agg_score: score,
            visit_count: 1,
            state: ThoughtNodeState.Open,
            thought_text: thought_text,
        };
        _.root_id = 0;
        _.node_count = 1;
        return 0;
    }

    /// Branches a new child thought under `parent_id`.
    /// Automatically transitions an `Open` parent to `Expanded`.
    /// Returns `null` if `parent_id` is invalid/`Pruned`/`Terminal`, `depth` exceeds `max_depth`,
    /// `local_score` is NaN/Inf, or the node pool is full.
    fn addChild(
        *_,
        parent_id: u32,
        thought_text: string,
        local_score: f32,
        is_terminal: bool,
    ): u32? {
        if (!isFiniteF32(local_score) || _.node_count >= _.nodes.len) return null;
        const p_idx: usize = parent_id as usize;
        if (p_idx >= _.node_count || !_.nodes[p_idx].active) return null;

        const p_state: ThoughtNodeState = _.nodes[p_idx].state;
        if (p_state == ThoughtNodeState.Pruned || p_state == ThoughtNodeState.Terminal) {
            return null;
        }
        if (_.nodes[p_idx].depth >= _.max_depth) return null;

        if (p_state == ThoughtNodeState.Open) {
            _.nodes[p_idx].state = ThoughtNodeState.Expanded;
        }

        const cid: u32 = _.node_count as u32;
        const c_depth: u32 = _.nodes[p_idx].depth + 1;
        _.nodes[_.node_count] = ThoughtNode{
            active: true,
            node_id: cid,
            parent_id: parent_id,
            depth: c_depth,
            local_score: local_score,
            agg_score: local_score,
            visit_count: 1,
            state: if (is_terminal) ThoughtNodeState.Terminal else ThoughtNodeState.Open,
            thought_text: thought_text,
        };
        _.node_count += 1;
        return cid;
    }

    /// Best-First frontier selection: returns the `Open` node with the highest `agg_score`,
    /// breaking ties deterministically by ascending `node_id`.
    /// Returns `null` if no `Open` nodes remain.
    fn selectBestOpenNode(): u32? {
        let best_id: u32? = null;
        let best_score: f32 = 0.0;

        let i: usize = 0;
        while (i < _.node_count) : (i += 1) {
            const n: ThoughtNode = _.nodes[i];
            if (!n.active || n.state != ThoughtNodeState.Open) continue;

            if (cur_best := best_id) {
                if (n.agg_score > best_score || (n.agg_score == best_score && n.node_id < cur_best)) {
                    best_id = n.node_id;
                    best_score = n.agg_score;
                }
            } else {
                best_id = n.node_id;
                best_score = n.agg_score;
            }
        }
        return best_id;
    }

    /// Prunes `root_node_id` and all of its descendants in a single O(N) non-recursive pass
    /// (exploiting strict topological creation order `parent_id < child_id`).
    /// Returns the number of nodes newly transitioned to `Pruned`.
    fn pruneSubtree(*_, root_node_id: u32): usize {
        const r_idx: usize = root_node_id as usize;
        if (r_idx >= _.node_count || !_.nodes[r_idx].active) return 0;

        let pruned: usize = 0;
        if (_.nodes[r_idx].state != ThoughtNodeState.Pruned) {
            _.nodes[r_idx].state = ThoughtNodeState.Pruned;
            pruned += 1;
        }

        let i: usize = r_idx + 1;
        while (i < _.node_count) : (i += 1) {
            if (!_.nodes[i].active || _.nodes[i].state == ThoughtNodeState.Pruned) continue;
            const pid: u32 = _.nodes[i].parent_id;
            if (pid != THOUGHT_NO_PARENT && _.nodes[pid as usize].state == ThoughtNodeState.Pruned) {
                _.nodes[i].state = ThoughtNodeState.Pruned;
                pruned += 1;
            }
        }
        return pruned;
    }

    /// Backpropagates a scalar `reward` from `leaf_id` up through its ancestor chain to the root,
    /// incrementing `visit_count` and updating the running-mean `agg_score` at each node.
    /// Returns `false` if `leaf_id` is invalid or `reward` is NaN/Inf.
    fn backpropagateScore(*_, leaf_id: u32, reward: f32): bool {
        if (!isFiniteF32(reward)) return false;
        const start_idx: usize = leaf_id as usize;
        if (start_idx >= _.node_count || !_.nodes[start_idx].active) return false;

        let cur: u32 = leaf_id;
        let hops: usize = 0;
        while (cur != THOUGHT_NO_PARENT && hops <= _.node_count) : (hops += 1) {
            const idx: usize = cur as usize;
            if (idx >= _.node_count || !_.nodes[idx].active) break;

            const old_n: u32 = _.nodes[idx].visit_count;
            const new_n: u32 = old_n + 1;
            _.nodes[idx].agg_score = (_.nodes[idx].agg_score * (old_n as f32) + reward) / (new_n as f32);
            _.nodes[idx].visit_count = new_n;
            cur = _.nodes[idx].parent_id;
        }
        return true;
    }

    /// Extracts the reasoning path from the root down to `node_id` in forward order
    /// `[root_id, ..., node_id]` into `out_path_ids`.
    /// Returns the path length, or `null` if `node_id` is invalid or `out_path_ids` is too small.
    fn extractPathToRoot(node_id: u32, out_path_ids: *[]u32): usize? {
        const idx: usize = node_id as usize;
        if (idx >= _.node_count || !_.nodes[idx].active) return null;

        const path_len: usize = (_.nodes[idx].depth as usize) + 1;
        if (out_path_ids.len < path_len) return null;

        let cur: u32 = node_id;
        let pos: usize = path_len;
        while (pos > 0) {
            pos -= 1;
            out_path_ids[pos] = cur;
            cur = _.nodes[cur as usize].parent_id;
        }
        return path_len;
    }

    /// Looks up a node snapshot by `node_id`.
    fn getNode(node_id: u32): ThoughtNode? {
        const idx: usize = node_id as usize;
        if (idx >= _.node_count || !_.nodes[idx].active) return null;
        return _.nodes[idx];
    }

    /// Returns a point-in-time tree telemetry snapshot.
    fn stats(): ThoughtTreeStats {
        let st = ThoughtTreeStats{
            total_nodes: _.node_count,
            open_nodes: 0,
            expanded_nodes: 0,
            pruned_nodes: 0,
            terminal_nodes: 0,
            max_depth_reached: 0,
        };
        let i: usize = 0;
        while (i < _.node_count) : (i += 1) {
            const n: ThoughtNode = _.nodes[i];
            if (!n.active) continue;
            if (n.depth > st.max_depth_reached) {
                st.max_depth_reached = n.depth;
            }
            if (n.state == ThoughtNodeState.Open) {
                st.open_nodes += 1;
            } else if (n.state == ThoughtNodeState.Expanded) {
                st.expanded_nodes += 1;
            } else if (n.state == ThoughtNodeState.Pruned) {
                st.pruned_nodes += 1;
            } else {
                st.terminal_nodes += 1;
            }
        }
        return st;
    }
}
