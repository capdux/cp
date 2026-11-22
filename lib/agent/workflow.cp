// Origin: Pure-CP Stdlib (AI Agent Deterministic Workflow FSM & Transition Audit Log) | Created: 2026-11-22
// Zero-heap-allocation deterministic finite state machine for multi-step AI agent orchestration,
// visit-cap cycle protection, human-in-the-loop gating, transition audit logging, and step rollback.

const MAX_REACHABILITY_NODES: usize = 256;

/// Semantic role of a node inside an `AgentWorkflowFsm`.
pub enum WorkflowNodeKind {
    Start,
    LlmStep,
    ToolStep,
    Decision,
    HumanGate,
    TerminalOk,
    TerminalFail,
}

/// Node descriptor stored in the caller-owned node slice.
/// `max_visits == 0` disables the cycle guard for this node.
pub struct WorkflowNode {
    id: u16 = 0;
    kind: WorkflowNodeKind = WorkflowNodeKind.LlmStep;
    max_visits: u16 = 0;
    visit_count: u16 = 0;
}

/// Directed transition rule triggered by `event_code`. Higher `priority` wins on ties.
pub struct WorkflowEdge {
    from_id: u16 = 0;
    to_id: u16 = 0;
    event_code: u16 = 0;
    priority: i16 = 0;
}

/// Immutable audit log entry recorded on every successful transition.
pub struct TransitionRecord {
    step_seq: u32 = 0;
    from_id: u16 = 0;
    to_id: u16 = 0;
    event_code: u16 = 0;
    timestamp_ms: u64 = 0;
}

/// Outcome returned by `AgentWorkflowFsm.trigger`.
pub enum WorkflowStepResult {
    Transitioned,
    TerminalReached,
    NoMatchingEdge,
    VisitLimitExceeded,
    InvalidState,
}

/// Static graph reachability and integrity summary returned by `validateReachability`.
pub struct ReachabilityReport {
    has_start: bool = false;
    has_invalid_edge: bool = false;
    reachable_nodes: usize = 0;
    unreachable_nodes: usize = 0;
    reachable_terminals: usize = 0;
    total_terminals: usize = 0;

    /// Returns `true` when the graph has a start node, no dangling edges, no unreachable nodes,
    /// and at least one reachable terminal node.
    fn isValid(): bool {
        return _.has_start &&
            !_.has_invalid_edge &&
            _.unreachable_nodes == 0 &&
            _.reachable_terminals > 0;
    }
}

fn isTerminalKind(kind: WorkflowNodeKind): bool {
    return kind == WorkflowNodeKind.TerminalOk || kind == WorkflowNodeKind.TerminalFail;
}

/// Zero-heap-allocation deterministic workflow FSM with audit log and rollback support.
pub struct AgentWorkflowFsm {
    nodes: []WorkflowNode;
    edges: []const WorkflowEdge;
    history_buf: []TransitionRecord;
    history_len: usize = 0;
    started: bool = false;
    current_idx: usize = 0;
    step_seq: u32 = 0;

    /// Initializes a workflow FSM using caller-owned `nodes`, `edges`, and `history_buf`.
    static fn init(
        nodes: *[]WorkflowNode,
        edges: []const WorkflowEdge,
        history_buf: *[]TransitionRecord,
    ): AgentWorkflowFsm {
        let i: usize = 0;
        while (i < nodes.len) : (i += 1) {
            nodes[i].visit_count = 0;
        }
        return AgentWorkflowFsm{
            nodes: nodes,
            edges: edges,
            history_buf: history_buf,
            history_len: 0,
            started: false,
            current_idx: 0,
            step_seq: 0,
        };
    }

    fn findNodeIndex(id: u16): usize? {
        let i: usize = 0;
        while (i < _.nodes.len) : (i += 1) {
            if (_.nodes[i].id == id) return i;
        }
        return null;
    }

    /// Resets visit counts and audit history and positions the FSM at the `Start` node.
    /// Returns `false` if no `Start` node exists or duplicate node IDs are present.
    fn start(*_): bool {
        // Reject duplicate node IDs
        let a: usize = 0;
        while (a < _.nodes.len) : (a += 1) {
            let b = a + 1;
            while (b < _.nodes.len) : (b += 1) {
                if (_.nodes[a].id == _.nodes[b].id) return false;
            }
        }

        let start_idx: usize? = null;
        let i: usize = 0;
        while (i < _.nodes.len) : (i += 1) {
            _.nodes[i].visit_count = 0;
            if (_.nodes[i].kind == WorkflowNodeKind.Start && start_idx == null) {
                start_idx = i;
            }
        }
        if (s_idx := start_idx) {
            _.current_idx = s_idx;
            _.nodes[s_idx].visit_count = 1;
            _.history_len = 0;
            _.step_seq = 0;
            _.started = true;
            return true;
        }
        _.started = false;
        return false;
    }

    /// Returns the ID of the active node, or `null` if the FSM has not been started.
    fn currentNodeId(): u16? {
        if (!_.started || _.current_idx >= _.nodes.len) return null;
        return _.nodes[_.current_idx].id;
    }

    /// Returns the kind of the active node, or `null` if the FSM has not been started.
    fn currentNodeKind(): WorkflowNodeKind? {
        if (!_.started || _.current_idx >= _.nodes.len) return null;
        return _.nodes[_.current_idx].kind;
    }

    /// Returns `true` if started and currently positioned on `TerminalOk` or `TerminalFail`.
    fn isTerminal(): bool {
        if (!_.started || _.current_idx >= _.nodes.len) return false;
        return isTerminalKind(_.nodes[_.current_idx].kind);
    }

    /// Returns `true` if started and currently paused on a `HumanGate` node.
    fn isWaitingHuman(): bool {
        if (!_.started || _.current_idx >= _.nodes.len) return false;
        return _.nodes[_.current_idx].kind == WorkflowNodeKind.HumanGate;
    }

    /// Returns a zero-copy slice of recorded transition audit entries.
    fn history(): []const TransitionRecord {
        return _.history_buf[0.._.history_len];
    }

    /// Triggers an event from the current node, selecting the highest-priority matching edge.
    /// Enforces `max_visits` cycle guards and appends to the transition audit log.
    fn trigger(*_, event_code: u16, now_ms: u64): WorkflowStepResult {
        if (!_.started || _.current_idx >= _.nodes.len || _.isTerminal()) {
            return WorkflowStepResult.InvalidState;
        }

        const from_id = _.nodes[_.current_idx].id;
        let best_edge_idx: usize? = null;
        let best_priority: i16 = 0;

        let i: usize = 0;
        while (i < _.edges.len) : (i += 1) {
            const e = _.edges[i];
            if (e.from_id != from_id || e.event_code != event_code) continue;
            if (_.findNodeIndex(e.to_id) == null) continue;
            if (best_edge_idx == null || e.priority > best_priority) {
                best_edge_idx = i;
                best_priority = e.priority;
            }
        }

        if (best_edge_idx == null) {
            return WorkflowStepResult.NoMatchingEdge;
        }
        const edge = _.edges[best_edge_idx ?? 0];
        const target_idx_opt = _.findNodeIndex(edge.to_id);
        if (target_idx_opt == null) {
            return WorkflowStepResult.InvalidState;
        }
        const target_idx = target_idx_opt ?? 0;

        // Enforce per-node visit cap cycle guard
        const cap = _.nodes[target_idx].max_visits;
        if (cap > 0 && _.nodes[target_idx].visit_count >= cap) {
            return WorkflowStepResult.VisitLimitExceeded;
        }

        _.nodes[target_idx].visit_count += 1;
        _.step_seq += 1;
        if (_.history_len < _.history_buf.len) {
            _.history_buf[_.history_len] = TransitionRecord{
                step_seq: _.step_seq,
                from_id: from_id,
                to_id: edge.to_id,
                event_code: event_code,
                timestamp_ms: now_ms,
            };
            _.history_len += 1;
        }
        _.current_idx = target_idx;

        if (isTerminalKind(_.nodes[target_idx].kind)) {
            return WorkflowStepResult.TerminalReached;
        }
        return WorkflowStepResult.Transitioned;
    }

    /// Reverts the most recent recorded transition, decrementing the target node's `visit_count`
    /// and restoring `currentNodeId()` to the previous node. Returns `false` if history is empty.
    fn rollbackLastStep(*_): bool {
        if (!_.started || _.history_len == 0 || _.current_idx >= _.nodes.len) {
            return false;
        }
        const last = _.history_buf[_.history_len - 1];
        if (last.to_id != _.nodes[_.current_idx].id) {
            return false;
        }
        const prev_idx_opt = _.findNodeIndex(last.from_id);
        if (prev_idx_opt == null) {
            return false;
        }
        const prev_idx = prev_idx_opt ?? 0;
        if (_.nodes[_.current_idx].visit_count > 0) {
            _.nodes[_.current_idx].visit_count -= 1;
        }
        _.current_idx = prev_idx;
        _.history_len -= 1;
        return true;
    }

    /// Statically validates graph integrity and reachability from the `Start` node
    /// without heap allocation (supports up to 256 nodes).
    fn validateReachability(): ReachabilityReport {
        let rep = ReachabilityReport{};
        const n = _.nodes.len;
        if (n == 0 || n > MAX_REACHABILITY_NODES) {
            return rep;
        }

        let start_idx: usize? = null;
        let i: usize = 0;
        while (i < n) : (i += 1) {
            if (_.nodes[i].kind == WorkflowNodeKind.Start && start_idx == null) {
                start_idx = i;
                rep.has_start = true;
            }
            if (isTerminalKind(_.nodes[i].kind)) {
                rep.total_terminals += 1;
            }
        }

        // Validate that every edge references existing source and destination nodes
        let e_i: usize = 0;
        while (e_i < _.edges.len) : (e_i += 1) {
            if (_.findNodeIndex(_.edges[e_i].from_id) == null || _.findNodeIndex(_.edges[e_i].to_id) == null) {
                rep.has_invalid_edge = true;
            }
        }

        if (!rep.has_start) {
            rep.unreachable_nodes = n;
            return rep;
        }

        let visited: [256]bool = [false; 256];
        if (s_idx := start_idx) {
            visited[s_idx] = true;
        }

        // Fixed-point reachability propagation
        let changed = true;
        while (changed) {
            changed = false;
            let k: usize = 0;
            while (k < _.edges.len) : (k += 1) {
                if (u := _.findNodeIndex(_.edges[k].from_id)) {
                    if (v := _.findNodeIndex(_.edges[k].to_id)) {
                        if (visited[u] && !visited[v]) {
                            visited[v] = true;
                            changed = true;
                        }
                    }
                }
            }
        }

        let j: usize = 0;
        while (j < n) : (j += 1) {
            if (visited[j]) {
                rep.reachable_nodes += 1;
                if (isTerminalKind(_.nodes[j].kind)) {
                    rep.reachable_terminals += 1;
                }
            } else {
                rep.unreachable_nodes += 1;
            }
        }
        return rep;
    }
}
