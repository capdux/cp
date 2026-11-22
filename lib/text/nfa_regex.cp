// Source: cp/lib/nfa_regex/nfa.cp (Pure CP Linear-Time Thompson NFA Regex Engine - ReDoS-Free)
const std = @import("std");

pub const MAX_NFA_STATES: usize = 4096;

pub const RegexError = error{
    EmptyPattern,
    UnmatchedParen,
    UnclosedCharClass,
    TrailingEscape,
    NothingToRepeat,
    InvalidRange,
    PatternTooComplex,
    OutOfMemory,
};

pub const MatchSpan = struct {
    start: usize,
    end: usize,
};

const CharClass = struct {
    negated: bool = false,
    bits: [4]u64 = [ 0, 0, 0, 0 ],

    fn setBit(self: *CharClass, ch: u8) {
        const word = @as(usize, ch >> 6);
        const bit: u6 = @truncate(ch & 63);
        self.bits[word] |= (@as(u64, 1) << bit);
    }

    fn setRange(self: *CharClass, lo: u8, hi: u8) {
        let c: u16 = lo;
        while (c <= hi) : (c += 1) {
            self.setBit(@intCast(c));
        }
    }

    fn matches(self: CharClass, ch: u8): bool {
        const word = @as(usize, ch >> 6);
        const bit: u6 = @truncate(ch & 63);
        const hit = (self.bits[word] & (@as(u64, 1) << bit)) != 0;
        return if (self.negated) !hit else hit;
    }
};

const StateKind = enum {
    literal,
    any_char,
    char_class,
    anchor_start,
    anchor_end,
    split,
    match_state,
};

const NfaState = struct {
    kind: StateKind,
    ch: u8 = 0,
    cclass: CharClass = CharClass{},
    out1: i32 = -1,
    out2: i32 = -1,
};

const Frag = struct {
    start: i32,
    outs: std.ArrayList(i32),

    fn deinit(self: *Frag, alloc: std.mem.Allocator) {
        self.outs.deinit(alloc);
    }
};

pub const NfaRegex = struct {
    alloc: std.mem.Allocator,
    states: []NfaState,
    start_state: i32,
    anchored_start: bool,

    pub fn deinit(self: *const NfaRegex) {
        self.alloc.free(self.states);
    }

    pub fn compile(alloc: std.mem.Allocator, pattern: []const u8): RegexError!NfaRegex {
        let builder = Builder{
            .alloc = alloc,
            .pat = pattern,
            .pos = 0,
            .states = .empty,
        };
        errdefer builder.states.deinit(alloc);

        if (pattern.len == 0) {
            const m_id = try builder.addState(NfaState{ .kind = .match_state });
            return NfaRegex{
                .alloc = alloc,
                .states = try builder.states.toOwnedSlice(alloc),
                .start_state = m_id,
                .anchored_start = false,
            };
        }

        let frag = try builder.parseAlternation(0);
        errdefer frag.deinit(alloc);

        if (builder.pos != pattern.len) {
            return RegexError.UnmatchedParen;
        }
        const match_id = try builder.addState(NfaState{ .kind = .match_state });
        builder.patch(&frag.outs, match_id);
        frag.deinit(alloc);

        return NfaRegex{
            .alloc = alloc,
            .states = try builder.states.toOwnedSlice(alloc),
            .start_state = frag.start,
            .anchored_start = (pattern[0] == '^'),
        };
    }

    pub fn isMatch(self: *const NfaRegex, alloc: std.mem.Allocator, input: []const u8): RegexError!bool {
        const span = try self.findFirst(alloc, input);
        return span != null;
    }

    pub fn findFirst(self: *const NfaRegex, alloc: std.mem.Allocator, input: []const u8): RegexError!(MatchSpan?) {
        let start_pos: usize = 0;
        const limit: usize = if (self.anchored_start) 1 else input.len + 1;
        while (start_pos < limit) : (start_pos += 1) {
            if (try self.matchAt(alloc, input, start_pos)) |end_pos| {
                return MatchSpan{ .start = start_pos, .end = end_pos };
            }
        }
        return null;
    }

    pub fn findAll(self: *const NfaRegex, alloc: std.mem.Allocator, input: []const u8): RegexError![]MatchSpan {
        let results: std.ArrayList(MatchSpan) = .empty;
        errdefer results.deinit(alloc);

        let pos: usize = 0;
        while (pos <= input.len) {
            if (self.anchored_start and pos > 0) { break; }
            if (try self.matchAt(alloc, input, pos)) |end_pos| {
                try results.append(alloc, MatchSpan{ .start = pos, .end = end_pos });
                pos = if (end_pos > pos) end_pos else pos + 1;
            } else {
                pos += 1;
            }
        }
        return results.toOwnedSlice(alloc);
    }

    fn matchAt(self: *const NfaRegex, alloc: std.mem.Allocator, input: []const u8, origin: usize): RegexError!(usize?) {
        const n = self.states.len;
        const gen_buf = try alloc.alloc(u32, n);
        defer alloc.free(gen_buf);
        @memset(gen_buf, 0);

        let curr: std.ArrayList(i32) = .empty;
        defer curr.deinit(alloc);
        let next: std.ArrayList(i32) = .empty;
        defer next.deinit(alloc);
        let stack: std.ArrayList(i32) = .empty;
        defer stack.deinit(alloc);

        let gen: u32 = 1;
        let best_end: usize? = null;

        try addStateEps(&self.states, &curr, &stack, alloc, gen_buf, &gen, self.start_state, origin, input.len);
        for (sid of curr.items) {
            const uid: usize = @intCast(sid);
            if (self.states[uid].kind == .match_state) {
                best_end = origin;
                break;
            }
        }

        let pos: usize = origin;
        while (pos < input.len and curr.items.len > 0) : (pos += 1) {
            const ch = input[pos];
            gen += 1;
            next.items.len = 0;

            for (sid of curr.items) {
                const uid: usize = @intCast(sid);
                const st = self.states[uid];
                let matched = false;
                switch (st.kind) {
                    .literal => matched = (ch == st.ch),
                    .any_char => matched = (ch != '\n'),
                    .char_class => matched = st.cclass.matches(ch),
                    else => {},
                }
                if (matched and st.out1 >= 0) {
                    try addStateEps(&self.states, &next, &stack, alloc, gen_buf, &gen, st.out1, pos + 1, input.len);
                }
            }

            // Swap curr and next
            const tmp = curr;
            curr = next;
            next = tmp;

            for (sid of curr.items) {
                const uid: usize = @intCast(sid);
                if (self.states[uid].kind == .match_state) {
                    best_end = pos + 1;
                    break;
                }
            }
        }
        return best_end;
    }
};

fn addStateEps(
    states_ptr: *const []NfaState,
    list: *std.ArrayList(i32),
    stack: *std.ArrayList(i32),
    alloc: std.mem.Allocator,
    gen_buf: []u32,
    gen: *u32,
    init_id: i32,
    pos: usize,
    input_len: usize,
): !void {
    if (init_id < 0) { return; }
    const states = states_ptr.*;
    stack.items.len = 0;
    try stack.append(alloc, init_id);

    while (stack.items.len > 0) {
        const sid = stack.pop().?;
        if (sid < 0) { continue; }
        const uid: usize = @intCast(sid);
        if (gen_buf[uid] == gen.*) { continue; }
        gen_buf[uid] = gen.*;

        const st = states[uid];
        switch (st.kind) {
            .split => {
                if (st.out2 >= 0) { try stack.append(alloc, st.out2); }
                if (st.out1 >= 0) { try stack.append(alloc, st.out1); }
            },
            .anchor_start => {
                if (pos == 0 and st.out1 >= 0) {
                    try stack.append(alloc, st.out1);
                }
            },
            .anchor_end => {
                if (pos == input_len and st.out1 >= 0) {
                    try stack.append(alloc, st.out1);
                }
            },
            else => {
                try list.append(alloc, sid);
            },
        }
    }
}

const Builder = struct {
    alloc: std.mem.Allocator,
    pat: []const u8,
    pos: usize,
    states: std.ArrayList(NfaState),

    fn addState(self: *Builder, st: NfaState): RegexError!i32 {
        if (self.states.items.len >= MAX_NFA_STATES) {
            return RegexError.PatternTooComplex;
        }
        const id: i32 = @intCast(self.states.items.len);
        try self.states.append(self.alloc, st);
        return id;
    }

    fn patch(self: *Builder, outs: *const std.ArrayList(i32), target: i32) {
        for (sid of outs.items) {
            const uid: usize = @intCast(sid);
            if (self.states[uid].out1 == -1) {
                self.states[uid].out1 = target;
            } else if (self.states[uid].out2 == -1) {
                self.states[uid].out2 = target;
            }
        }
    }

    fn parseAlternation(self: *Builder, depth: usize): RegexError!Frag {
        if (depth > 64) { return RegexError.PatternTooComplex; }
        let left = try self.parseConcat(depth);
        errdefer left.deinit(self.alloc);

        while (self.pos < self.pat.len and self.pat[self.pos] == '|') {
            self.pos += 1;
            let right = try self.parseConcat(depth);
            errdefer right.deinit(self.alloc);

            const split_id = try self.addState(NfaState{
                .kind = .split,
                .out1 = left.start,
                .out2 = right.start,
            });
            let merged: std.ArrayList(i32) = .empty;
            errdefer merged.deinit(self.alloc);
            try merged.appendSlice(self.alloc, left.outs.items);
            try merged.appendSlice(self.alloc, right.outs.items);
            left.deinit(self.alloc);
            right.deinit(self.alloc);
            left = Frag{ .start = split_id, .outs = merged };
        }
        return left;
    }

    fn parseConcat(self: *Builder, depth: usize): RegexError!Frag {
        let has_prev = false;
        let prev = Frag{ .start = -1, .outs = .empty };
        errdefer if (has_prev) { prev.deinit(self.alloc); };

        while (self.pos < self.pat.len) {
            const ch = self.pat[self.pos];
            if (ch == '|' or ch == ')') { break; }
            let next_frag = try self.parseRepeat(depth);
            if (!has_prev) {
                prev = next_frag;
                has_prev = true;
            } else {
                self.patch(&prev.outs, next_frag.start);
                prev.deinit(self.alloc);
                prev = Frag{ .start = prev.start, .outs = next_frag.outs };
            }
        }

        if (!has_prev) {
            // Empty branch epsilon state
            const s = try self.addState(NfaState{ .kind = .split, .out1 = -1, .out2 = -1 });
            let outs: std.ArrayList(i32) = .empty;
            try outs.append(self.alloc, s);
            return Frag{ .start = s, .outs = outs };
        }
        return prev;
    }

    fn parseRepeat(self: *Builder, depth: usize): RegexError!Frag {
        let frag = try self.parseAtom(depth);
        errdefer frag.deinit(self.alloc);

        while (self.pos < self.pat.len) {
            const ch = self.pat[self.pos];
            if (ch == '*') {
                self.pos += 1;
                const s = try self.addState(NfaState{ .kind = .split, .out1 = frag.start, .out2 = -1 });
                self.patch(&frag.outs, s);
                frag.outs.items.len = 0;
                try frag.outs.append(self.alloc, s);
                frag.start = s;
            } else if (ch == '+') {
                self.pos += 1;
                const s = try self.addState(NfaState{ .kind = .split, .out1 = frag.start, .out2 = -1 });
                self.patch(&frag.outs, s);
                frag.outs.items.len = 0;
                try frag.outs.append(self.alloc, s);
            } else if (ch == '?') {
                self.pos += 1;
                const s = try self.addState(NfaState{ .kind = .split, .out1 = frag.start, .out2 = -1 });
                try frag.outs.append(self.alloc, s);
                frag.start = s;
            } else {
                break;
            }
        }
        return frag;
    }

    fn parseAtom(self: *Builder, depth: usize): RegexError!Frag {
        const ch = self.pat[self.pos];
        if (ch == '*' or ch == '+' or ch == '?') {
            return RegexError.NothingToRepeat;
        }
        if (ch == '(') {
            self.pos += 1;
            const inner = try self.parseAlternation(depth + 1);
            if (self.pos >= self.pat.len or self.pat[self.pos] != ')') {
                let tmp = inner;
                tmp.deinit(self.alloc);
                return RegexError.UnmatchedParen;
            }
            self.pos += 1;
            return inner;
        }
        if (ch == '[') {
            return self.parseCharClass();
        }
        if (ch == '.') {
            self.pos += 1;
            return self.singleState(NfaState{ .kind = .any_char });
        }
        if (ch == '^') {
            self.pos += 1;
            return self.singleState(NfaState{ .kind = .anchor_start });
        }
        if (ch == '$') {
            self.pos += 1;
            return self.singleState(NfaState{ .kind = .anchor_end });
        }
        if (ch == '\\') {
            self.pos += 1;
            if (self.pos >= self.pat.len) { return RegexError.TrailingEscape; }
            const esc = self.pat[self.pos];
            self.pos += 1;
            return self.buildEscape(esc);
        }
        self.pos += 1;
        return self.singleState(NfaState{ .kind = .literal, .ch = ch });
    }

    fn singleState(self: *Builder, st: NfaState): RegexError!Frag {
        const id = try self.addState(st);
        let outs: std.ArrayList(i32) = .empty;
        errdefer outs.deinit(self.alloc);
        try outs.append(self.alloc, id);
        return Frag{ .start = id, .outs = outs };
    }

    fn buildEscape(self: *Builder, esc: u8): RegexError!Frag {
        let cc = CharClass{};
        switch (esc) {
            'd' => { cc.setRange('0', '9'); return self.singleState(NfaState{ .kind = .char_class, .cclass = cc }); },
            'D' => { cc.negated = true; cc.setRange('0', '9'); return self.singleState(NfaState{ .kind = .char_class, .cclass = cc }); },
            'w' => {
                cc.setRange('a', 'z');
                cc.setRange('A', 'Z');
                cc.setRange('0', '9');
                cc.setBit('_');
                return self.singleState(NfaState{ .kind = .char_class, .cclass = cc });
            },
            'W' => {
                cc.negated = true;
                cc.setRange('a', 'z');
                cc.setRange('A', 'Z');
                cc.setRange('0', '9');
                cc.setBit('_');
                return self.singleState(NfaState{ .kind = .char_class, .cclass = cc });
            },
            's' => {
                cc.setBit(' ');
                cc.setBit('\t');
                cc.setBit('\n');
                cc.setBit('\r');
                return self.singleState(NfaState{ .kind = .char_class, .cclass = cc });
            },
            'S' => {
                cc.negated = true;
                cc.setBit(' ');
                cc.setBit('\t');
                cc.setBit('\n');
                cc.setBit('\r');
                return self.singleState(NfaState{ .kind = .char_class, .cclass = cc });
            },
            'n' => return self.singleState(NfaState{ .kind = .literal, .ch = '\n' }),
            'r' => return self.singleState(NfaState{ .kind = .literal, .ch = '\r' }),
            't' => return self.singleState(NfaState{ .kind = .literal, .ch = '\t' }),
            else => return self.singleState(NfaState{ .kind = .literal, .ch = esc }),
        }
    }

    fn parseCharClass(self: *Builder): RegexError!Frag {
        self.pos += 1; // skip '['
        let cc = CharClass{};
        if (self.pos < self.pat.len and self.pat[self.pos] == '^') {
            cc.negated = true;
            self.pos += 1;
        }
        let first = true;
        while (self.pos < self.pat.len) {
            const c = self.pat[self.pos];
            if (c == ']' and !first) {
                self.pos += 1;
                return self.singleState(NfaState{ .kind = .char_class, .cclass = cc });
            }
            first = false;
            let lo = c;
            self.pos += 1;
            if (lo == '\\') {
                if (self.pos >= self.pat.len) { return RegexError.UnclosedCharClass; }
                lo = self.pat[self.pos];
                self.pos += 1;
            }
            if (self.pos + 1 < self.pat.len and self.pat[self.pos] == '-' and self.pat[self.pos + 1] != ']') {
                self.pos += 1;
                let hi = self.pat[self.pos];
                self.pos += 1;
                if (hi == '\\') {
                    if (self.pos >= self.pat.len) { return RegexError.UnclosedCharClass; }
                    hi = self.pat[self.pos];
                    self.pos += 1;
                }
                if (lo > hi) { return RegexError.InvalidRange; }
                cc.setRange(lo, hi);
            } else {
                cc.setBit(lo);
            }
        }
        return RegexError.UnclosedCharClass;
    }
};
