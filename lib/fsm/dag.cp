// Source: cp/lib/fsm/dag.cp (Workflow DAG Scheduler with Parallel Execution Waves)
// Pure CP Directed Acyclic Graph scheduler with Kahn topological waves, ready-node scheduling & failure cascade.

const std = @import("std");

pub err DagError {
    CycleDetected,
    DuplicateNode,
    NodeNotFound,
    EmptyNodeId,
    InvalidStateTransition,
    OutOfMemory,
}

pub enum TaskStatus: u8 {
    Pending = 0,
    Running = 1,
    Succeeded = 2,
    Failed = 3,
    Skipped = 4,

    fn isTerminal(): bool {
        return _ == .Succeeded || _ == .Failed || _ == .Skipped;
    }
}

struct DagNode {
    id: []const u8 = "",
    status: TaskStatus = TaskStatus.Pending,
    deps: std.ArrayList(usize) = .empty,
    dependents: std.ArrayList(usize) = .empty,
}

pub struct WorkflowDag {
    allocator: std.mem.Allocator = undefined,
    nodes: std.ArrayList(DagNode) = .empty,
    id_to_idx: std.StringHashMapUnmanaged(usize) = .empty,
    edge_count: usize = 0,

    static fn init(alloc: std.mem.Allocator): WorkflowDag {
        return WorkflowDag{
            allocator: alloc,
            nodes: .empty,
            id_to_idx: .empty,
            edge_count: 0,
        };
    }

    fn deinit(*_) {
        let i: usize = 0;
        while (i < _.nodes.items.len) : (i += 1) {
            _.allocator.free(_.nodes.items[i].id);
            _.nodes.items[i].deps.deinit(_.allocator);
            _.nodes.items[i].dependents.deinit(_.allocator);
        }
        _.nodes.deinit(_.allocator);
        _.id_to_idx.deinit(_.allocator);
    }

    fn addNode(*_, id: []const u8): void ! DagError {
        if (id.len == 0) {
            return DagError.EmptyNodeId;
        }
        if (_.id_to_idx.contains(id)) {
            return DagError.DuplicateNode;
        }
        const owned_id = _.allocator.dupe(u8, id) catch return DagError.OutOfMemory;
        const idx = _.nodes.items.len;
        _.nodes.append(_.allocator, DagNode{
            id: owned_id,
            status: TaskStatus.Pending,
            deps: .empty,
            dependents: .empty,
        }) catch {
            _.allocator.free(owned_id);
            return DagError.OutOfMemory;
        };
        _.id_to_idx.put(_.allocator, owned_id, idx) catch {
            * = _.nodes.pop();
            _.allocator.free(owned_id);
            return DagError.OutOfMemory;
        };
    }

    fn addEdge(*_, dep_id: []const u8, task_id: []const u8): void ! DagError {
        const dep_opt = _.id_to_idx.get(dep_id);
        if (dep_opt == null) {
            return DagError.NodeNotFound;
        }
        const dep_idx = dep_opt.?;

        const task_opt = _.id_to_idx.get(task_id);
        if (task_opt == null) {
            return DagError.NodeNotFound;
        }
        const task_idx = task_opt.?;

        // Deduplicate if the exact edge already exists
        let already_exists = false;
        const existing_deps = _.nodes.items[task_idx].deps.items;
        let i: usize = 0;
        while (i < existing_deps.len) : (i += 1) {
            if (existing_deps[i] == dep_idx) {
                already_exists = true;
                break;
            }
        }

        if (!already_exists) {
            _.nodes.items[task_idx].deps.append(_.allocator, dep_idx) catch return DagError.OutOfMemory;
            _.nodes.items[dep_idx].dependents.append(_.allocator, task_idx) catch {
                * = _.nodes.items[task_idx].deps.pop();
                return DagError.OutOfMemory;
            };
            _.edge_count += 1;
        }
    }

    fn nodeCount(): usize {
        return _.nodes.items.len;
    }

    fn edgeCount(): usize {
        return _.edge_count;
    }

    fn nodeStatus(id: []const u8): TaskStatus? {
        const idx = _.id_to_idx.get(id) ?? return null;
        return _.nodes.items[idx].status;
    }

    fn checkCycleWithAlloc(alloc: std.mem.Allocator): bool ! DagError {
        const n = _.nodes.items.len;
        if (n == 0) return false;

        let in_deg = alloc.alloc(usize, n) catch return DagError.OutOfMemory;
        defer alloc.free(in_deg);

        let queue = alloc.alloc(usize, n) catch return DagError.OutOfMemory;
        defer alloc.free(queue);

        let head: usize = 0;
        let tail: usize = 0;
        let i: usize = 0;
        while (i < n) : (i += 1) {
            const deg = _.nodes.items[i].deps.items.len;
            in_deg[i] = deg;
            if (deg == 0) {
                queue[tail] = i;
                tail += 1;
            }
        }

        let visited: usize = 0;
        while (head < tail) {
            const u = queue[head];
            head += 1;
            visited += 1;

            const outs = _.nodes.items[u].dependents.items;
            let k: usize = 0;
            while (k < outs.len) : (k += 1) {
                const v = outs[k];
                in_deg[v] -= 1;
                if (in_deg[v] == 0) {
                    queue[tail] = v;
                    tail += 1;
                }
            }
        }

        return visited != n;
    }

    fn hasCycle(): bool {
        const n = _.nodes.items.len;
        if (n == 0) return false;
        if (n <= 256) {
            let in_deg: [256]usize = undefined;
            let queue: [256]usize = undefined;
            let head: usize = 0;
            let tail: usize = 0;
            let i: usize = 0;
            while (i < n) : (i += 1) {
                const deg = _.nodes.items[i].deps.items.len;
                in_deg[i] = deg;
                if (deg == 0) {
                    queue[tail] = i;
                    tail += 1;
                }
            }
            let visited: usize = 0;
            while (head < tail) {
                const u = queue[head];
                head += 1;
                visited += 1;
                const outs = _.nodes.items[u].dependents.items;
                let k: usize = 0;
                while (k < outs.len) : (k += 1) {
                    const v = outs[k];
                    in_deg[v] -= 1;
                    if (in_deg[v] == 0) {
                        queue[tail] = v;
                        tail += 1;
                    }
                }
            }
            return visited != n;
        }
        return _.checkCycleWithAlloc(_.allocator) catch true;
    }

    static fn freeWaves(alloc: std.mem.Allocator, waves: [][]string) {
        let i: usize = 0;
        while (i < waves.len) : (i += 1) {
            if (waves[i].len > 0) {
                alloc.free(waves[i]);
            }
        }
        if (waves.len > 0) {
            alloc.free(waves);
        }
    }

    fn executionWaves(alloc: std.mem.Allocator): ([][]string) ! DagError {
        const n = _.nodes.items.len;
        let waves: std.ArrayList([]string) = .empty;
        errdefer {
            let wi: usize = 0;
            while (wi < waves.items.len) : (wi += 1) {
                if (waves.items[wi].len > 0) {
                    alloc.free(waves.items[wi]);
                }
            }
            waves.deinit(alloc);
        }

        if (n == 0) {
            return waves.toOwnedSlice(alloc) catch return DagError.OutOfMemory;
        }

        let in_deg = alloc.alloc(usize, n) catch return DagError.OutOfMemory;
        defer alloc.free(in_deg);

        let cur_wave: std.ArrayList(usize) = .empty;
        defer cur_wave.deinit(alloc);

        let next_wave: std.ArrayList(usize) = .empty;
        defer next_wave.deinit(alloc);

        let i: usize = 0;
        while (i < n) : (i += 1) {
            const deg = _.nodes.items[i].deps.items.len;
            in_deg[i] = deg;
            if (deg == 0) {
                cur_wave.append(alloc, i) catch return DagError.OutOfMemory;
            }
        }

        let visited: usize = 0;
        while (cur_wave.items.len > 0) {
            const w_len = cur_wave.items.len;
            let wave_ids = alloc.alloc(string, w_len) catch return DagError.OutOfMemory;
            errdefer alloc.free(wave_ids);

            next_wave.clearRetainingCapacity();

            let j: usize = 0;
            while (j < w_len) : (j += 1) {
                const u = cur_wave.items[j];
                wave_ids[j] = _.nodes.items[u].id;
                visited += 1;

                const outs = _.nodes.items[u].dependents.items;
                let k: usize = 0;
                while (k < outs.len) : (k += 1) {
                    const v = outs[k];
                    in_deg[v] -= 1;
                    if (in_deg[v] == 0) {
                        next_wave.append(alloc, v) catch return DagError.OutOfMemory;
                    }
                }
            }

            waves.append(alloc, wave_ids) catch {
                alloc.free(wave_ids);
                return DagError.OutOfMemory;
            };

            // Keep deterministic insertion-index order within next_wave
            let si: usize = 1;
            while (si < next_wave.items.len) : (si += 1) {
                const key = next_wave.items[si];
                let sj: usize = si;
                while (sj > 0 && next_wave.items[sj - 1] > key) : (sj -= 1) {
                    next_wave.items[sj] = next_wave.items[sj - 1];
                }
                next_wave.items[sj] = key;
            }

            cur_wave.clearRetainingCapacity();
            cur_wave.appendSlice(alloc, next_wave.items) catch return DagError.OutOfMemory;
        }

        if (visited != n) {
            return DagError.CycleDetected;
        }

        return waves.toOwnedSlice(alloc) catch return DagError.OutOfMemory;
    }

    fn allDepsSucceeded(node_idx: usize): bool {
        const deps = _.nodes.items[node_idx].deps.items;
        let i: usize = 0;
        while (i < deps.len) : (i += 1) {
            if (_.nodes.items[deps[i]].status != TaskStatus.Succeeded) {
                return false;
            }
        }
        return true;
    }

    fn readyNodes(alloc: std.mem.Allocator): ([]string) ! DagError {
        let out: std.ArrayList(string) = .empty;
        errdefer out.deinit(alloc);

        let i: usize = 0;
        while (i < _.nodes.items.len) : (i += 1) {
            if (_.nodes.items[i].status == TaskStatus.Pending && _.allDepsSucceeded(i)) {
                out.append(alloc, _.nodes.items[i].id) catch return DagError.OutOfMemory;
            }
        }

        return out.toOwnedSlice(alloc) catch return DagError.OutOfMemory;
    }

    fn markRunning(*_, id: []const u8): void ! DagError {
        const idx_opt = _.id_to_idx.get(id);
        if (idx_opt == null) {
            return DagError.NodeNotFound;
        }
        const idx = idx_opt.?;
        if (_.nodes.items[idx].status != TaskStatus.Pending) {
            return DagError.InvalidStateTransition;
        }
        if (!_.allDepsSucceeded(idx)) {
            return DagError.InvalidStateTransition;
        }
        _.nodes.items[idx].status = TaskStatus.Running;
    }

    fn markSucceeded(*_, id: []const u8): void ! DagError {
        const idx_opt = _.id_to_idx.get(id);
        if (idx_opt == null) {
            return DagError.NodeNotFound;
        }
        const idx = idx_opt.?;
        const st = _.nodes.items[idx].status;
        if (st != TaskStatus.Pending && st != TaskStatus.Running) {
            return DagError.InvalidStateTransition;
        }
        if (!_.allDepsSucceeded(idx)) {
            return DagError.InvalidStateTransition;
        }
        _.nodes.items[idx].status = TaskStatus.Succeeded;
    }

    fn markFailed(*_, id: []const u8): void ! DagError {
        const idx_opt = _.id_to_idx.get(id);
        if (idx_opt == null) {
            return DagError.NodeNotFound;
        }
        const idx = idx_opt.?;
        const st = _.nodes.items[idx].status;
        if (st == TaskStatus.Succeeded || st == TaskStatus.Skipped) {
            return DagError.InvalidStateTransition;
        }
        _.nodes.items[idx].status = TaskStatus.Failed;

        // BFS cascade Skipped to all reachable pending downstream dependents
        let queue: std.ArrayList(usize) = .empty;
        defer queue.deinit(_.allocator);

        queue.append(_.allocator, idx) catch return DagError.OutOfMemory;

        let head: usize = 0;
        while (head < queue.items.len) {
            const u = queue.items[head];
            head += 1;

            const outs = _.nodes.items[u].dependents.items;
            let k: usize = 0;
            while (k < outs.len) : (k += 1) {
                const v = outs[k];
                if (_.nodes.items[v].status == TaskStatus.Pending) {
                    _.nodes.items[v].status = TaskStatus.Skipped;
                    queue.append(_.allocator, v) catch return DagError.OutOfMemory;
                }
            }
        }
    }

    fn isComplete(): bool {
        let i: usize = 0;
        while (i < _.nodes.items.len) : (i += 1) {
            if (!_.nodes.items[i].status.isTerminal()) {
                return false;
            }
        }
        return true;
    }

    fn reset(*_) {
        let i: usize = 0;
        while (i < _.nodes.items.len) : (i += 1) {
            _.nodes.items[i].status = TaskStatus.Pending;
        }
    }
}
