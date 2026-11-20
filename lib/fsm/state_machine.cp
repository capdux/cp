// Source: cp/lib/fsm/state_machine.cp (Deterministic Finite State Machine & Mermaid Exporter)
// Pure CP Deterministic Finite State Machine with transition audit log & Mermaid stateDiagram-v2 export.

const std = @import("std");

pub err FsmError {
    InvalidTransition,
    DuplicateTransition,
    EmptyState,
    EmptyEvent,
    OutOfMemory,
}

pub struct TransitionRule {
    from_state: []const u8 = "",
    event: []const u8 = "",
    to_state: []const u8 = "",
}

pub struct TransitionRecord {
    from_state: []const u8 = "",
    event: []const u8 = "",
    to_state: []const u8 = "",
    step: usize = 0,
}

pub struct StateMachine {
    allocator: std.mem.Allocator = undefined,
    initial_state: []const u8 = "",
    owned_initial: ([]u8)? = null,
    cur_state: []const u8 = "",
    rules: std.ArrayList(TransitionRule) = .empty,
    records: std.ArrayList(TransitionRecord) = .empty,
    interned: std.StringHashMapUnmanaged(string) = .empty,

    static fn init(alloc: std.mem.Allocator, initial_state: []const u8): StateMachine {
        const owned = alloc.dupe(u8, initial_state) catch null;
        let init_slice: []const u8 = initial_state;
        if (o := owned) {
            init_slice = o;
        }
        return StateMachine{
            allocator: alloc,
            initial_state: init_slice,
            owned_initial: owned,
            cur_state: init_slice,
            rules: .empty,
            records: .empty,
            interned: .empty,
        };
    }

    fn deinit(*_) {
        let iter = _.interned.iterator();
        while (entry := iter.next()) {
            _.allocator.free(entry.key_ptr.*);
        }
        _.interned.deinit(_.allocator);
        _.rules.deinit(_.allocator);
        _.records.deinit(_.allocator);
        if (owned := _.owned_initial) {
            _.allocator.free(owned);
            _.owned_initial = null;
        }
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

    fn addTransition(*_, from_state: []const u8, event: []const u8, to_state: []const u8): void ! FsmError {
        if (from_state.len == 0 || to_state.len == 0) {
            return FsmError.EmptyState;
        }
        if (event.len == 0) {
            return FsmError.EmptyEvent;
        }

        let i: usize = 0;
        while (i < _.rules.items.len) : (i += 1) {
            const r = _.rules.items[i];
            if (std.mem.eql(u8, r.from_state, from_state) && std.mem.eql(u8, r.event, event)) {
                return FsmError.DuplicateTransition;
            }
        }

        const from_owned = _.intern(from_state)?;
        const event_owned = _.intern(event)?;
        const to_owned = _.intern(to_state)?;

        _.rules.append(_.allocator, TransitionRule{
            from_state: from_owned,
            event: event_owned,
            to_state: to_owned,
        }) catch return FsmError.OutOfMemory;
    }

    fn findRuleIndex(from_state: []const u8, event: []const u8): usize? {
        let i: usize = 0;
        while (i < _.rules.items.len) : (i += 1) {
            const r = _.rules.items[i];
            if (std.mem.eql(u8, r.from_state, from_state) && std.mem.eql(u8, r.event, event)) {
                return i;
            }
        }
        return null;
    }

    fn canTrigger(event: []const u8): bool {
        return _.findRuleIndex(_.cur_state, event) != null;
    }

    fn trigger(*_, event: []const u8): ([]const u8) ! FsmError {
        const idx_opt = _.findRuleIndex(_.cur_state, event);
        if (idx_opt == null) {
            return FsmError.InvalidTransition;
        }
        const idx = idx_opt.?;
        const rule = _.rules.items[idx];
        const step_num = _.records.items.len;
        _.records.append(_.allocator, TransitionRecord{
            from_state: rule.from_state,
            event: rule.event,
            to_state: rule.to_state,
            step: step_num,
        }) catch return FsmError.OutOfMemory;
        _.cur_state = rule.to_state;
        return _.cur_state;
    }

    fn currentState(): []const u8 {
        return _.cur_state;
    }

    fn initialState(): []const u8 {
        return _.initial_state;
    }

    fn history(): []const TransitionRecord {
        return _.records.items;
    }

    fn transitionRules(): []const TransitionRule {
        return _.rules.items;
    }

    fn reset(*_) {
        _.cur_state = _.initial_state;
        _.records.clearRetainingCapacity();
    }

    fn toMermaid(alloc: std.mem.Allocator): ([]u8) ! FsmError {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        out.appendSlice(alloc, "stateDiagram-v2\n") catch return FsmError.OutOfMemory;
        if (_.initial_state.len > 0) {
            out.appendSlice(alloc, "    [*] --> ") catch return FsmError.OutOfMemory;
            out.appendSlice(alloc, _.initial_state) catch return FsmError.OutOfMemory;
            out.append(alloc, '\n') catch return FsmError.OutOfMemory;
        }

        let i: usize = 0;
        while (i < _.rules.items.len) : (i += 1) {
            const r = _.rules.items[i];
            out.appendSlice(alloc, "    ") catch return FsmError.OutOfMemory;
            out.appendSlice(alloc, r.from_state) catch return FsmError.OutOfMemory;
            out.appendSlice(alloc, " --> ") catch return FsmError.OutOfMemory;
            out.appendSlice(alloc, r.to_state) catch return FsmError.OutOfMemory;
            out.appendSlice(alloc, " : ") catch return FsmError.OutOfMemory;
            out.appendSlice(alloc, r.event) catch return FsmError.OutOfMemory;
            out.append(alloc, '\n') catch return FsmError.OutOfMemory;
        }

        return out.toOwnedSlice(alloc) catch return FsmError.OutOfMemory;
    }
}
