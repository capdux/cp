// Source: cp/lib/fsm/fsm.cp (Deterministic Finite State Machine, Guards & GraphViz DOT)
// Pure CP Deterministic Finite State Machine with guard predicates, lifecycle actions,
// transition audit log, BFS topology analysis, and GraphViz DOT export.

const std = @import("std");

pub err FsmError {
    UnknownState,
    InvalidTransition,
    GuardRejected,
    DuplicateTransition,
    EmptyMachine,
    OutOfMemory,
}

pub type GuardFn = *const fn ((*anyopaque)?, []const u8, []const u8, []const u8) bool;
pub type ActionFn = *const fn ((*anyopaque)?, []const u8, []const u8, []const u8) void;

pub struct TransitionRecord {
    from: []const u8 = "",
    event: []const u8 = "",
    to: []const u8 = "",
    accepted: bool = false,
}

struct StateInfo {
    name: []const u8 = "",
    is_terminal: bool = false,
    on_enter: ActionFn? = null,
    on_exit: ActionFn? = null,
}

struct TransitionRule {
    from: []const u8 = "",
    event: []const u8 = "",
    to: []const u8 = "",
    guard: GuardFn? = null,
}

pub struct StateMachine {
    allocator: std.mem.Allocator = undefined,
    initial_state: []const u8 = "",
    cur_state: []const u8 = "",
    states: std.ArrayList(StateInfo) = .empty,
    rules: std.ArrayList(TransitionRule) = .empty,
    records: std.ArrayList(TransitionRecord) = .empty,
    interned: std.StringHashMapUnmanaged(string) = .empty,
    on_transition: ActionFn? = null,

    static fn init(alloc: std.mem.Allocator, initial_state: []const u8): StateMachine {
        let sm = StateMachine{
            allocator: alloc,
            initial_state: "",
            cur_state: "",
            states: .empty,
            rules: .empty,
            records: .empty,
            interned: .empty,
            on_transition: null,
        };
        if (initial_state.len > 0) {
            const interned_init = sm.intern(initial_state) catch {
                return sm;
            };
            sm.initial_state = interned_init;
            sm.cur_state = interned_init;
            sm.states.append(alloc, StateInfo{
                name: interned_init,
                is_terminal: false,
                on_enter: null,
                on_exit: null,
            }) catch {};
        }
        return sm;
    }

    fn deinit(*_) {
        let iter = _.interned.iterator();
        while (entry := iter.next()) {
            _.allocator.free(entry.key_ptr.*);
        }
        _.interned.deinit(_.allocator);
        _.states.deinit(_.allocator);
        _.rules.deinit(_.allocator);
        _.records.deinit(_.allocator);
    }

    fn intern(*_, s: []const u8): ([]const u8) ! FsmError {
        if (existing := _.interned.get(s)) {
            return existing;
        }
        const duped = _.allocator.dupe(u8, s) catch return FsmError.OutOfMemory;
        _.interned.put(_.allocator, duped, duped) catch {
            _.allocator.free(duped);
            return FsmError.OutOfMemory;
        };
        return duped;
    }

    fn findStateIndex(name: []const u8): usize? {
        let i: usize = 0;
        while (i < _.states.items.len) : (i += 1) {
            if (std.mem.eql(u8, _.states.items[i].name, name)) {
                return i;
            }
        }
        return null;
    }

    fn addStateInternal(*_, name: []const u8, terminal: bool): void ! FsmError {
        if (name.len == 0) {
            return FsmError.UnknownState;
        }
        if (idx := _.findStateIndex(name)) {
            if (terminal) {
                _.states.items[idx].is_terminal = true;
            }
        } else {
            const owned = _.intern(name)?;
            _.states.append(_.allocator, StateInfo{
                name: owned,
                is_terminal: terminal,
                on_enter: null,
                on_exit: null,
            }) catch return FsmError.OutOfMemory;
        }
    }

    fn addState(*_, name: []const u8): void ! FsmError {
        _.addStateInternal(name, false)?;
    }

    fn addTerminalState(*_, name: []const u8): void ! FsmError {
        _.addStateInternal(name, true)?;
    }

    fn onEnter(*_, state: []const u8, action: ActionFn): void ! FsmError {
        const idx_opt = _.findStateIndex(state);
        if (idx_opt == null) {
            return FsmError.UnknownState;
        }
        _.states.items[idx_opt.?].on_enter = action;
    }

    fn onExit(*_, state: []const u8, action: ActionFn): void ! FsmError {
        const idx_opt = _.findStateIndex(state);
        if (idx_opt == null) {
            return FsmError.UnknownState;
        }
        _.states.items[idx_opt.?].on_exit = action;
    }

    fn onTransition(*_, action: ActionFn) {
        _.on_transition = action;
    }

    fn findRuleIndex(from_state: []const u8, event: []const u8): usize? {
        let i: usize = 0;
        while (i < _.rules.items.len) : (i += 1) {
            const r = _.rules.items[i];
            if (std.mem.eql(u8, r.from, from_state) && std.mem.eql(u8, r.event, event)) {
                return i;
            }
        }
        return null;
    }

    fn addGuardedTransition(*_, from_state: []const u8, event: []const u8, to_state: []const u8, guard: GuardFn?): void ! FsmError {
        if (_.states.items.len == 0) {
            return FsmError.EmptyMachine;
        }
        if (_.findStateIndex(from_state) == null || _.findStateIndex(to_state) == null) {
            return FsmError.UnknownState;
        }
        if (event.len == 0) {
            return FsmError.InvalidTransition;
        }
        if (_.findRuleIndex(from_state, event) != null) {
            return FsmError.DuplicateTransition;
        }

        const from_owned = _.intern(from_state)?;
        const event_owned = _.intern(event)?;
        const to_owned = _.intern(to_state)?;

        _.rules.append(_.allocator, TransitionRule{
            from: from_owned,
            event: event_owned,
            to: to_owned,
            guard: guard,
        }) catch return FsmError.OutOfMemory;
    }

    fn addTransition(*_, from_state: []const u8, event: []const u8, to_state: []const u8): void ! FsmError {
        _.addGuardedTransition(from_state, event, to_state, null)?;
    }

    fn canTrigger(event: []const u8, ctx: (*anyopaque)?): bool {
        if (_.states.items.len == 0 || _.cur_state.len == 0) {
            return false;
        }
        const idx_opt = _.findRuleIndex(_.cur_state, event);
        if (idx_opt == null) {
            return false;
        }
        const rule = _.rules.items[idx_opt.?];
        if (g := rule.guard) {
            return g(ctx, rule.from, rule.event, rule.to);
        }
        return true;
    }

    fn trigger(*_, event: []const u8, ctx: (*anyopaque)?): ([]const u8) ! FsmError {
        if (_.states.items.len == 0 || _.cur_state.len == 0) {
            return FsmError.EmptyMachine;
        }
        const idx_opt = _.findRuleIndex(_.cur_state, event);
        if (idx_opt == null) {
            return FsmError.InvalidTransition;
        }
        const rule = _.rules.items[idx_opt.?];

        if (g := rule.guard) {
            if (!g(ctx, rule.from, rule.event, rule.to)) {
                _.records.append(_.allocator, TransitionRecord{
                    from: rule.from,
                    event: rule.event,
                    to: rule.to,
                    accepted: false,
                }) catch return FsmError.OutOfMemory;
                return FsmError.GuardRejected;
            }
        }

        if (from_idx := _.findStateIndex(rule.from)) {
            if (exit_cb := _.states.items[from_idx].on_exit) {
                exit_cb(ctx, rule.from, rule.event, rule.to);
            }
        }

        _.cur_state = rule.to;
        _.records.append(_.allocator, TransitionRecord{
            from: rule.from,
            event: rule.event,
            to: rule.to,
            accepted: true,
        }) catch return FsmError.OutOfMemory;

        if (trans_cb := _.on_transition) {
            trans_cb(ctx, rule.from, rule.event, rule.to);
        }

        if (to_idx := _.findStateIndex(rule.to)) {
            if (enter_cb := _.states.items[to_idx].on_enter) {
                enter_cb(ctx, rule.from, rule.event, rule.to);
            }
        }

        return _.cur_state;
    }

    fn currentState(): []const u8 {
        return _.cur_state;
    }

    fn initialState(): []const u8 {
        return _.initial_state;
    }

    fn isTerminal(): bool {
        if (idx := _.findStateIndex(_.cur_state)) {
            return _.states.items[idx].is_terminal;
        }
        return false;
    }

    fn availableEvents(alloc: std.mem.Allocator): ([][]const u8) ! FsmError {
        let list: std.ArrayList(string) = .empty;
        errdefer list.deinit(alloc);

        let i: usize = 0;
        while (i < _.rules.items.len) : (i += 1) {
            const r = _.rules.items[i];
            if (std.mem.eql(u8, r.from, _.cur_state)) {
                list.append(alloc, r.event) catch return FsmError.OutOfMemory;
            }
        }
        return list.toOwnedSlice(alloc) catch return FsmError.OutOfMemory;
    }

    fn history(): []const TransitionRecord {
        return _.records.items;
    }

    fn reset(*_) {
        _.cur_state = _.initial_state;
        _.records.clearRetainingCapacity();
    }

    fn reachableStates(alloc: std.mem.Allocator): ([][]const u8) ! FsmError {
        if (_.states.items.len == 0 || _.initial_state.len == 0) {
            return FsmError.EmptyMachine;
        }
        const n = _.states.items.len;
        let visited = alloc.alloc(bool, n) catch return FsmError.OutOfMemory;
        defer alloc.free(visited);
        let k: usize = 0;
        while (k < n) : (k += 1) {
            visited[k] = false;
        }

        let queue: std.ArrayList(usize) = .empty;
        defer queue.deinit(alloc);

        let result: std.ArrayList(string) = .empty;
        errdefer result.deinit(alloc);

        const start_idx_opt = _.findStateIndex(_.initial_state);
        if (start_idx_opt == null) {
            return FsmError.EmptyMachine;
        }
        const start_idx = start_idx_opt.?;
        visited[start_idx] = true;
        queue.append(alloc, start_idx) catch return FsmError.OutOfMemory;
        result.append(alloc, _.states.items[start_idx].name) catch return FsmError.OutOfMemory;

        let head: usize = 0;
        while (head < queue.items.len) : (head += 1) {
            const u_idx = queue.items[head];
            const u_name = _.states.items[u_idx].name;
            let r_i: usize = 0;
            while (r_i < _.rules.items.len) : (r_i += 1) {
                const rule = _.rules.items[r_i];
                if (std.mem.eql(u8, rule.from, u_name)) {
                    if (v_idx := _.findStateIndex(rule.to)) {
                        if (!visited[v_idx]) {
                            visited[v_idx] = true;
                            queue.append(alloc, v_idx) catch return FsmError.OutOfMemory;
                            result.append(alloc, _.states.items[v_idx].name) catch return FsmError.OutOfMemory;
                        }
                    }
                }
            }
        }

        return result.toOwnedSlice(alloc) catch return FsmError.OutOfMemory;
    }

    fn findDeadEndStates(alloc: std.mem.Allocator): ([][]const u8) ! FsmError {
        if (_.states.items.len == 0 || _.initial_state.len == 0) {
            return FsmError.EmptyMachine;
        }
        const reachable = _.reachableStates(alloc)?;
        defer alloc.free(reachable);

        let dead_ends: std.ArrayList(string) = .empty;
        errdefer dead_ends.deinit(alloc);

        let i: usize = 0;
        while (i < reachable.len) : (i += 1) {
            const state_name = reachable[i];
            if (s_idx := _.findStateIndex(state_name)) {
                if (_.states.items[s_idx].is_terminal) {
                    continue;
                }
                let has_outgoing = false;
                let r_i: usize = 0;
                while (r_i < _.rules.items.len) : (r_i += 1) {
                    if (std.mem.eql(u8, _.rules.items[r_i].from, state_name)) {
                        has_outgoing = true;
                        break;
                    }
                }
                if (!has_outgoing) {
                    dead_ends.append(alloc, state_name) catch return FsmError.OutOfMemory;
                }
            }
        }

        return dead_ends.toOwnedSlice(alloc) catch return FsmError.OutOfMemory;
    }

    fn toDot(alloc: std.mem.Allocator): ([]u8) ! FsmError {
        if (_.states.items.len == 0 || _.initial_state.len == 0) {
            return FsmError.EmptyMachine;
        }
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        out.appendSlice(alloc, "digraph fsm {\n") catch return FsmError.OutOfMemory;
        out.appendSlice(alloc, "    rankdir=LR;\n") catch return FsmError.OutOfMemory;
        out.appendSlice(alloc, "    __start [shape=point, width=0.2];\n") catch return FsmError.OutOfMemory;
        out.appendSlice(alloc, "    __start -> \"") catch return FsmError.OutOfMemory;
        out.appendSlice(alloc, _.initial_state) catch return FsmError.OutOfMemory;
        out.appendSlice(alloc, "\";\n") catch return FsmError.OutOfMemory;

        let i: usize = 0;
        while (i < _.states.items.len) : (i += 1) {
            const s = _.states.items[i];
            out.appendSlice(alloc, "    \"") catch return FsmError.OutOfMemory;
            out.appendSlice(alloc, s.name) catch return FsmError.OutOfMemory;
            if (s.is_terminal) {
                out.appendSlice(alloc, "\" [shape=doublecircle];\n") catch return FsmError.OutOfMemory;
            } else {
                out.appendSlice(alloc, "\" [shape=circle];\n") catch return FsmError.OutOfMemory;
            }
        }

        let r_i: usize = 0;
        while (r_i < _.rules.items.len) : (r_i += 1) {
            const r = _.rules.items[r_i];
            out.appendSlice(alloc, "    \"") catch return FsmError.OutOfMemory;
            out.appendSlice(alloc, r.from) catch return FsmError.OutOfMemory;
            out.appendSlice(alloc, "\" -> \"") catch return FsmError.OutOfMemory;
            out.appendSlice(alloc, r.to) catch return FsmError.OutOfMemory;
            out.appendSlice(alloc, "\" [label=\"") catch return FsmError.OutOfMemory;
            out.appendSlice(alloc, r.event) catch return FsmError.OutOfMemory;
            out.appendSlice(alloc, "\"];\n") catch return FsmError.OutOfMemory;
        }

        out.appendSlice(alloc, "}\n") catch return FsmError.OutOfMemory;
        return out.toOwnedSlice(alloc) catch return FsmError.OutOfMemory;
    }
}
