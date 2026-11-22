// Origin: Pure-CP Stdlib (Zero-Alloc Process Tree, Subreaper Orphan Adoption & Shutdown Planner) | Created: 2026-11-23
// Pure CP zero-heap-allocation Process Tree, Subreaper Orphan Re-parenting & Bottom-Up Shutdown Orchestrator:
//   - Fixed 64-node process tree table tracking `(pid, ppid, pgid, state, exit_code, is_subreaper,
//     started_at_ms, exited_at_ms)` with duplicate-PID, cycle, and capacity guards
//   - Iterative zero-recursion bottom-up graceful shutdown planner (`planShutdownOrder`):
//       * Deterministic BFS level traversal followed by reversal ("children before parents")
//       * Process-group batch selection (`collectProcessGroup`)
//   - Subreaper orphan adoption (`markExited` + `reapZombies`):
//       * When a parent exits into `zombie`, all direct children are atomically re-parented to the
//         nearest alive `is_subreaper` ancestor (or root init fallback), preventing orphan leaks
//       * `reapZombies` harvests zombie children for a parent/subreaper in deterministic PID order
//   - Zero-heap global telemetry snapshot (`ProcTreeStats`)

// ============================================================================
// 1. Constants, Enums & Data Models
// ============================================================================

/// Fixed capacity of the zero-heap process tree table.
pub const PROC_TREE_MAX_NODES: usize = 64;

/// Sentinel invalid process ID.
pub const PROC_TREE_NO_PID: u32 = 0;

/// Default root init process ID.
pub const PROC_TREE_DEFAULT_INIT_PID: u32 = 1;

/// Lifecycle state of a process node in `ProcessTree`.
pub enum ProcNodeState {
    running,
    stopping,
    zombie,
    reaped,
}

/// Operation status code returned by `ProcessTree` mutation methods.
pub enum ProcTreeOpStatus {
    ok,
    invalid_pid,
    duplicate_pid,
    parent_not_found,
    parent_not_alive,
    cycle_detected,
    capacity_full,
    not_found,
    invalid_state,
}

/// Single process node stored in the fixed-capacity process tree.
pub struct ProcNode {
    active: bool = false;
    pid: u32 = 0;
    ppid: u32 = 0;
    pgid: u32 = 0;
    state: ProcNodeState = ProcNodeState.running;
    exit_code: i32 = 0;
    is_subreaper: bool = false;
    started_at_ms: u64 = 0;
    exited_at_ms: u64 = 0;

    /// Return true if this process is alive (`running` or `stopping`).
    fn isAlive(): bool {
        return active && (state == ProcNodeState.running || state == ProcNodeState.stopping);
    }
}

/// Exit summary for a single reaped zombie child returned by `reapZombies`.
pub struct ReapedProcInfo {
    pid: u32 = 0;
    ppid: u32 = 0;
    pgid: u32 = 0;
    exit_code: i32 = 0;
}

/// Outcome report returned by `ProcessTree.markExited`.
pub struct ProcExitReport {
    status: ProcTreeOpStatus = ProcTreeOpStatus.ok;
    pid: u32 = 0;
    exit_code: i32 = 0;
    reparented_children: usize = 0;
    new_parent_pid: u32 = 0;
}

/// Zero-heap global telemetry snapshot for `ProcessTree`.
pub struct ProcTreeStats {
    root_pid: u32 = 0;
    capacity: usize = PROC_TREE_MAX_NODES;
    total_nodes: usize = 0;
    running_count: usize = 0;
    stopping_count: usize = 0;
    zombie_count: usize = 0;
    reaped_count: usize = 0;
    subreaper_count: usize = 0;
    reparented_total: u64 = 0;
    reaped_total: u64 = 0;
}

/// Format a `ProcNodeState` enum variant as a canonical lowercase ASCII label.
pub fn formatProcNodeState(state: ProcNodeState): []const u8 {
    if (state == ProcNodeState.running) return "running";
    if (state == ProcNodeState.stopping) return "stopping";
    if (state == ProcNodeState.zombie) return "zombie";
    return "reaped";
}

// ============================================================================
// 2. ProcessTree — Zero-Heap Process Tree & Subreaper Orchestrator
// ============================================================================

/// Zero-heap-allocation process tree, subreaper orphan adopter & shutdown planner.
pub struct ProcessTree {
    root_pid: u32 = 0;
    nodes: [PROC_TREE_MAX_NODES]ProcNode = [ProcNode{}; PROC_TREE_MAX_NODES];
    reparented_total: u64 = 0;
    reaped_total: u64 = 0;

    /// Initialize a new `ProcessTree` rooted at `root_pid` (init process, always subreaper).
    /// Returns `null` if `root_pid == 0`.
    static fn init(root_pid: u32, now_ms: u64): ProcessTree? {
        if (root_pid == PROC_TREE_NO_PID) return null;
        let tree = ProcessTree{};
        tree.root_pid = root_pid;
        tree.nodes[0] = ProcNode{
            active: true,
            pid: root_pid,
            ppid: PROC_TREE_NO_PID,
            pgid: root_pid,
            state: ProcNodeState.running,
            exit_code: 0,
            is_subreaper: true,
            started_at_ms: now_ms,
            exited_at_ms: 0,
        };
        return tree;
    }

    fn findActiveSlot(pid: u32): usize? {
        if (pid == PROC_TREE_NO_PID) return null;
        let i: usize = 0;
        while (i < PROC_TREE_MAX_NODES) : (i += 1) {
            if (nodes[i].active && nodes[i].state != ProcNodeState.reaped && nodes[i].pid == pid) {
                return i;
            }
        }
        return null;
    }

    fn findAnySlot(pid: u32): usize? {
        if (pid == PROC_TREE_NO_PID) return null;
        let i: usize = 0;
        while (i < PROC_TREE_MAX_NODES) : (i += 1) {
            if (nodes[i].active && nodes[i].pid == pid) {
                return i;
            }
        }
        return null;
    }

    fn selectFreeSlot(): usize? {
        // Prefer completely unused slots first.
        let i: usize = 0;
        while (i < PROC_TREE_MAX_NODES) : (i += 1) {
            if (!nodes[i].active) return i;
        }
        // Recycle oldest reaped slot if table is full.
        let found_reaped = false;
        let best_idx: usize = 0;
        let best_exit_ms: u64 = 0;
        let j: usize = 0;
        while (j < PROC_TREE_MAX_NODES) : (j += 1) {
            if (nodes[j].active && nodes[j].state == ProcNodeState.reaped) {
                if (!found_reaped || nodes[j].exited_at_ms < best_exit_ms) {
                    found_reaped = true;
                    best_idx = j;
                    best_exit_ms = nodes[j].exited_at_ms;
                }
            }
        }
        if (found_reaped) return best_idx;
        return null;
    }

    /// Retrieve a copy of a process node by `pid` (searches active & reaped records).
    fn getNode(pid: u32): ProcNode? {
        if (idx := findActiveSlot(pid)) {
            return nodes[idx];
        }
        if (idx := findAnySlot(pid)) {
            return nodes[idx];
        }
        return null;
    }

    /// Spawn a new child process under an alive `ppid`.
    /// If `pgid == 0`, inherits the parent's `pgid`.
    fn spawn(
        *_,
        pid: u32,
        ppid: u32,
        pgid: u32,
        is_subreaper: bool,
        now_ms: u64,
    ): ProcTreeOpStatus {
        if (pid == PROC_TREE_NO_PID || ppid == PROC_TREE_NO_PID) {
            return ProcTreeOpStatus.invalid_pid;
        }
        if (pid == ppid) {
            return ProcTreeOpStatus.cycle_detected;
        }
        if (findActiveSlot(pid) != null) {
            return ProcTreeOpStatus.duplicate_pid;
        }
        const parent_idx_opt = findActiveSlot(ppid);
        if (parent_idx_opt == null) {
            return ProcTreeOpStatus.parent_not_found;
        }
        const parent_idx = parent_idx_opt ?? 0;
        if (!nodes[parent_idx].isAlive()) {
            return ProcTreeOpStatus.parent_not_alive;
        }

        const slot_opt = selectFreeSlot();
        if (slot_opt == null) {
            return ProcTreeOpStatus.capacity_full;
        }
        const slot = slot_opt ?? 0;
        const effective_pgid: u32 = if (pgid == 0) nodes[parent_idx].pgid else pgid;

        nodes[slot] = ProcNode{
            active: true,
            pid: pid,
            ppid: ppid,
            pgid: effective_pgid,
            state: ProcNodeState.running,
            exit_code: 0,
            is_subreaper: is_subreaper,
            started_at_ms: now_ms,
            exited_at_ms: 0,
        };
        return ProcTreeOpStatus.ok;
    }

    /// Re-parent an existing process `pid` under `new_ppid` with explicit cycle detection.
    fn reparent(*_, pid: u32, new_ppid: u32): ProcTreeOpStatus {
        if (pid == PROC_TREE_NO_PID || new_ppid == PROC_TREE_NO_PID || pid == root_pid) {
            return ProcTreeOpStatus.invalid_pid;
        }
        if (pid == new_ppid) {
            return ProcTreeOpStatus.cycle_detected;
        }
        const idx_opt = findActiveSlot(pid);
        if (idx_opt == null) return ProcTreeOpStatus.not_found;
        const idx = idx_opt ?? 0;

        const parent_idx_opt = findActiveSlot(new_ppid);
        if (parent_idx_opt == null) return ProcTreeOpStatus.parent_not_found;
        const parent_idx = parent_idx_opt ?? 0;
        if (!nodes[parent_idx].isAlive()) return ProcTreeOpStatus.parent_not_alive;

        // Walk upward from `new_ppid` to root; if `pid` is encountered, reparenting would form a cycle.
        let cursor = new_ppid;
        let hops: usize = 0;
        while (cursor != PROC_TREE_NO_PID && hops < PROC_TREE_MAX_NODES) : (hops += 1) {
            if (cursor == pid) return ProcTreeOpStatus.cycle_detected;
            if (c_idx := findActiveSlot(cursor)) {
                cursor = nodes[c_idx].ppid;
            } else {
                break;
            }
        }

        nodes[idx].ppid = new_ppid;
        return ProcTreeOpStatus.ok;
    }

    /// Toggle the `is_subreaper` flag on an alive non-root process.
    fn setSubreaper(*_, pid: u32, enabled: bool): ProcTreeOpStatus {
        const idx_opt = findActiveSlot(pid);
        if (idx_opt == null) return ProcTreeOpStatus.not_found;
        const idx = idx_opt ?? 0;
        if (!nodes[idx].isAlive()) return ProcTreeOpStatus.invalid_state;
        if (pid == root_pid && !enabled) return ProcTreeOpStatus.invalid_state;
        nodes[idx].is_subreaper = enabled;
        return ProcTreeOpStatus.ok;
    }

    /// Transition a `running` process to `stopping` (e.g., after sending SIGTERM).
    fn markStopping(*_, pid: u32): ProcTreeOpStatus {
        const idx_opt = findActiveSlot(pid);
        if (idx_opt == null) return ProcTreeOpStatus.not_found;
        const idx = idx_opt ?? 0;
        if (nodes[idx].state != ProcNodeState.running) return ProcTreeOpStatus.invalid_state;
        nodes[idx].state = ProcNodeState.stopping;
        return ProcTreeOpStatus.ok;
    }

    /// Find the nearest alive `is_subreaper` ancestor for `exiting_pid`, falling back to `root_pid`.
    fn findNearestReaper(exiting_pid: u32): u32 {
        const start_opt = findActiveSlot(exiting_pid);
        if (start_opt == null) return root_pid;
        let cursor = nodes[start_opt ?? 0].ppid;
        let steps: usize = 0;
        while (cursor != PROC_TREE_NO_PID && steps < PROC_TREE_MAX_NODES) : (steps += 1) {
            if (c_idx := findActiveSlot(cursor)) {
                if (nodes[c_idx].isAlive() && nodes[c_idx].is_subreaper) {
                    return nodes[c_idx].pid;
                }
                cursor = nodes[c_idx].ppid;
            } else {
                break;
            }
        }
        return root_pid;
    }

    /// Mark `pid` as exited (`zombie`) with `exit_code`.
    /// Atomically re-parents all direct non-reaped children of `pid` to the nearest alive
    /// `is_subreaper` ancestor (or `root_pid`), preventing orphan escape.
    fn markExited(*_, pid: u32, exit_code: i32, now_ms: u64): ProcExitReport {
        if (pid == PROC_TREE_NO_PID || pid == root_pid) {
            return ProcExitReport{
                status: ProcTreeOpStatus.invalid_pid,
                pid: pid,
                exit_code: exit_code,
                reparented_children: 0,
                new_parent_pid: 0,
            };
        }
        const idx_opt = findActiveSlot(pid);
        if (idx_opt == null) {
            return ProcExitReport{
                status: ProcTreeOpStatus.not_found,
                pid: pid,
                exit_code: exit_code,
                reparented_children: 0,
                new_parent_pid: 0,
            };
        }
        const idx = idx_opt ?? 0;
        if (!nodes[idx].isAlive()) {
            return ProcExitReport{
                status: ProcTreeOpStatus.invalid_state,
                pid: pid,
                exit_code: exit_code,
                reparented_children: 0,
                new_parent_pid: 0,
            };
        }

        const reaper_pid = findNearestReaper(pid);
        nodes[idx].state = ProcNodeState.zombie;
        nodes[idx].exit_code = exit_code;
        nodes[idx].exited_at_ms = now_ms;

        let reparented: usize = 0;
        let i: usize = 0;
        while (i < PROC_TREE_MAX_NODES) : (i += 1) {
            if (nodes[i].active && nodes[i].state != ProcNodeState.reaped && nodes[i].ppid == pid) {
                nodes[i].ppid = reaper_pid;
                reparented += 1;
            }
        }
        reparented_total += @as(u64, @intCast(reparented));

        return ProcExitReport{
            status: ProcTreeOpStatus.ok,
            pid: pid,
            exit_code: exit_code,
            reparented_children: reparented,
            new_parent_pid: reaper_pid,
        };
    }

    /// Reap zombie children belonging to `parent_pid` in deterministic ascending PID order.
    /// Marks reaped children as `ProcNodeState.reaped` and returns the count written to `out_reaped`.
    fn reapZombies(*_, parent_pid: u32, out_reaped: *[]ReapedProcInfo): usize {
        if (parent_pid == PROC_TREE_NO_PID || out_reaped.len == 0) return 0;

        let count: usize = 0;
        while (count < out_reaped.len) {
            let found = false;
            let best_idx: usize = 0;
            let best_pid: u32 = 0;

            let i: usize = 0;
            while (i < PROC_TREE_MAX_NODES) : (i += 1) {
                if (nodes[i].active && nodes[i].state == ProcNodeState.zombie && nodes[i].ppid == parent_pid) {
                    if (!found || nodes[i].pid < best_pid) {
                        found = true;
                        best_idx = i;
                        best_pid = nodes[i].pid;
                    }
                }
            }
            if (!found) break;

            out_reaped[count] = ReapedProcInfo{
                pid: nodes[best_idx].pid,
                ppid: nodes[best_idx].ppid,
                pgid: nodes[best_idx].pgid,
                exit_code: nodes[best_idx].exit_code,
            };
            nodes[best_idx].state = ProcNodeState.reaped;
            count += 1;
            reaped_total += 1;
        }
        return count;
    }

    /// Pure-iterative zero-recursion graceful shutdown planner (BFS + level reversal).
    /// Outputs alive (`running` / `stopping`) processes in the subtree rooted at `subtree_root_pid`
    /// in strict bottom-up ("children before parents") order into `out_pids`.
    fn planShutdownOrder(subtree_root_pid: u32, out_pids: *[]u32): usize {
        if (subtree_root_pid == PROC_TREE_NO_PID || out_pids.len == 0) return 0;
        const root_idx_opt = findActiveSlot(subtree_root_pid);
        if (root_idx_opt == null) return 0;
        const root_idx = root_idx_opt ?? 0;
        if (!nodes[root_idx].isAlive()) return 0;

        let bfs_pids: [PROC_TREE_MAX_NODES]u32 = [0; PROC_TREE_MAX_NODES];
        let visited: [PROC_TREE_MAX_NODES]bool = [false; PROC_TREE_MAX_NODES];
        let head: usize = 0;
        let tail: usize = 0;

        bfs_pids[tail] = subtree_root_pid;
        tail += 1;
        visited[root_idx] = true;

        while (head < tail) {
            const cur_pid = bfs_pids[head];
            head += 1;

            // Collect direct alive children in ascending PID order for deterministic BFS levels.
            let last_child_pid: u32 = 0;
            while (tail < PROC_TREE_MAX_NODES) {
                let found = false;
                let best_idx: usize = 0;
                let best_pid: u32 = 0;

                let i: usize = 0;
                while (i < PROC_TREE_MAX_NODES) : (i += 1) {
                    if (!nodes[i].isAlive()) continue;
                    if (visited[i]) continue;
                    if (nodes[i].ppid != cur_pid) continue;
                    if (nodes[i].pid <= last_child_pid) continue;
                    if (!found || nodes[i].pid < best_pid) {
                        found = true;
                        best_idx = i;
                        best_pid = nodes[i].pid;
                    }
                }
                if (!found) break;

                visited[best_idx] = true;
                bfs_pids[tail] = best_pid;
                tail += 1;
                last_child_pid = best_pid;
            }
        }

        // Reverse BFS order -> deepest leaves first, subtree root last.
        let written: usize = 0;
        let k = tail;
        while (k > 0 && written < out_pids.len) {
            k -= 1;
            out_pids[written] = bfs_pids[k];
            written += 1;
        }
        return written;
    }

    /// Collect all alive processes belonging to process group `pgid` in ascending PID order.
    fn collectProcessGroup(pgid: u32, out_pids: *[]u32): usize {
        if (pgid == PROC_TREE_NO_PID || out_pids.len == 0) return 0;
        let written: usize = 0;
        let last_pid: u32 = 0;
        while (written < out_pids.len) {
            let found = false;
            let best_pid: u32 = 0;
            let i: usize = 0;
            while (i < PROC_TREE_MAX_NODES) : (i += 1) {
                if (!nodes[i].isAlive()) continue;
                if (nodes[i].pgid != pgid) continue;
                if (nodes[i].pid <= last_pid) continue;
                if (!found || nodes[i].pid < best_pid) {
                    found = true;
                    best_pid = nodes[i].pid;
                }
            }
            if (!found) break;
            out_pids[written] = best_pid;
            written += 1;
            last_pid = best_pid;
        }
        return written;
    }

    /// Return a zero-heap global telemetry snapshot.
    fn stats(): ProcTreeStats {
        let st = ProcTreeStats{};
        st.root_pid = root_pid;
        st.capacity = PROC_TREE_MAX_NODES;
        st.reparented_total = reparented_total;
        st.reaped_total = reaped_total;

        let i: usize = 0;
        while (i < PROC_TREE_MAX_NODES) : (i += 1) {
            if (!nodes[i].active) continue;
            st.total_nodes += 1;
            if (nodes[i].state == ProcNodeState.running) {
                st.running_count += 1;
            } else if (nodes[i].state == ProcNodeState.stopping) {
                st.stopping_count += 1;
            } else if (nodes[i].state == ProcNodeState.zombie) {
                st.zombie_count += 1;
            } else {
                st.reaped_count += 1;
            }
            if (nodes[i].isAlive() && nodes[i].is_subreaper) {
                st.subreaper_count += 1;
            }
        }
        return st;
    }
}
