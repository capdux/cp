// Source: cp/lib/regex/vm.cp (Anti-ReDoS Pike VM Regex Engine)
// Pure CP zero-C-dependency O(N*M) Thompson NFA / Pike VM execution engine
// with sparse-set deduplication, capture groups, replaceAll ($0-$9), and split.

const std = @import("std");
import {
    RegexError,
    Program,
    Compiler,
    Inst,
    Opcode,
    CharClass,
    MAX_SLOTS,
} from "./compiler.cp";

const UNSET_POS: usize = std.math.maxInt(usize);

pub struct MatchSpan {
    start: usize,
    end: usize,
    text: string,
}

pub type OptMatchSpan = MatchSpan?;

pub struct Captures {
    alloc: std.mem.Allocator,
    spans: []OptMatchSpan = [],

    pub fn deinit(*_) {
        if (_.spans.len > 0) {
            _.alloc.free(_.spans);
            _.spans = [];
        }
    }

    pub fn count(): usize {
        return _.spans.len;
    }

    pub fn group(idx: usize): MatchSpan? {
        if (idx >= _.spans.len) return null;
        return _.spans[idx];
    }
}

struct Thread {
    pc: usize,
    slots: [MAX_SLOTS]usize,
}

fn emptySlots(): [MAX_SLOTS]usize {
    return [UNSET_POS; MAX_SLOTS];
}

pub struct Regex {
    alloc: std.mem.Allocator,
    program: Program,

    pub static fn compile(alloc: std.mem.Allocator, pattern: []const u8): Regex ! RegexError {
        const prog = Compiler.compile(alloc, pattern)?;
        return Regex{
            alloc: alloc,
            program: prog,
        };
    }

    pub fn deinit(*_) {
        _.program.deinit();
    }

    pub fn isMatch(text: []const u8): bool {
        return _.find(text) != null;
    }

    pub fn isFullMatch(text: []const u8): bool {
        let out_slots = emptySlots();
        const ok = _.runPikeVm(text, 0, true, *out_slots) catch return false;
        if (!ok) return false;
        return out_slots[0] == 0 && out_slots[1] == text.len;
    }

    pub fn find(text: []const u8): MatchSpan? {
        let out_slots = emptySlots();
        const ok = _.runPikeVm(text, 0, false, *out_slots) catch return null;
        if (!ok) return null;
        const s = out_slots[0];
        const e = out_slots[1];
        if (s == UNSET_POS || e == UNSET_POS || s > e || e > text.len) return null;
        return MatchSpan{
            start: s,
            end: e,
            text: text[s..e],
        };
    }

    pub fn findAll(alloc: std.mem.Allocator, text: []const u8): ([]MatchSpan)! {
        let matches: std.ArrayList(MatchSpan) = .empty;
        defer matches.deinit(alloc);

        let search_pos: usize = 0;
        let out_slots = emptySlots();

        while (search_pos <= text.len) {
            out_slots = emptySlots();
            const ok = _.runPikeVm(text, search_pos, false, *out_slots)?;
            if (!ok) break;

            const s = out_slots[0];
            const e = out_slots[1];
            if (s == UNSET_POS || e == UNSET_POS || s > e || e > text.len) break;

            matches.append(alloc, MatchSpan{
                start: s,
                end: e,
                text: text[s..e],
            })?;

            if (e > s) {
                search_pos = e;
            } else {
                search_pos = e + 1;
            }
        }

        let out = alloc.alloc(MatchSpan, matches.items.len)?;
        for (i of 0..matches.items.len) {
            out[i] = matches.items[i];
        }
        return out;
    }

    pub fn captures(alloc: std.mem.Allocator, text: []const u8): (Captures?)! {
        let out_slots = emptySlots();
        const ok = _.runPikeVm(text, 0, false, *out_slots)?;
        if (!ok) return null;

        const total_groups = _.program.num_groups + 1;
        let spans = alloc.alloc(OptMatchSpan, total_groups)?;
        for (i of 0..total_groups) {
            const s = out_slots[i * 2];
            const e = out_slots[i * 2 + 1];
            if (s != UNSET_POS && e != UNSET_POS && s <= e && e <= text.len) {
                spans[i] = MatchSpan{
                    start: s,
                    end: e,
                    text: text[s..e],
                };
            } else {
                spans[i] = null;
            }
        }

        return Captures{
            alloc: alloc,
            spans: spans,
        };
    }

    pub fn replaceAll(alloc: std.mem.Allocator, text: []const u8, replacement: []const u8): ([]u8)! {
        let buf: std.ArrayList(u8) = .empty;
        defer buf.deinit(alloc);

        let search_pos: usize = 0;
        let last_end: usize = 0;
        let out_slots = emptySlots();

        while (search_pos <= text.len) {
            out_slots = emptySlots();
            const ok = _.runPikeVm(text, search_pos, false, *out_slots)?;
            if (!ok) break;

            const s = out_slots[0];
            const e = out_slots[1];
            if (s == UNSET_POS || e == UNSET_POS || s > e || e > text.len) break;

            if (s > last_end) {
                buf.appendSlice(alloc, text[last_end..s])?;
            }
            _.appendReplacement(alloc, *buf, text, replacement, out_slots)?;

            last_end = e;
            if (e > s) {
                search_pos = e;
            } else {
                if (e < text.len) {
                    buf.append(alloc, text[e])?;
                    last_end = e + 1;
                }
                search_pos = e + 1;
            }
        }

        if (last_end < text.len) {
            buf.appendSlice(alloc, text[last_end..])?;
        }

        let out = alloc.alloc(u8, buf.items.len)?;
        @memcpy(out, buf.items);
        return out;
    }

    pub fn split(alloc: std.mem.Allocator, text: []const u8): ([]string)! {
        let parts: std.ArrayList(string) = .empty;
        defer parts.deinit(alloc);

        let search_pos: usize = 0;
        let last_end: usize = 0;
        let out_slots = emptySlots();

        while (search_pos <= text.len) {
            out_slots = emptySlots();
            const ok = _.runPikeVm(text, search_pos, false, *out_slots)?;
            if (!ok) break;

            const s = out_slots[0];
            const e = out_slots[1];
            if (s == UNSET_POS || e == UNSET_POS || s > e || e > text.len) break;

            parts.append(alloc, text[last_end..s])?;
            last_end = e;

            if (e > s) {
                search_pos = e;
            } else {
                search_pos = e + 1;
            }
        }

        parts.append(alloc, text[last_end..])?;

        let out = alloc.alloc(string, parts.items.len)?;
        for (i of 0..parts.items.len) {
            out[i] = parts.items[i];
        }
        return out;
    }

    fn appendReplacement(
        alloc: std.mem.Allocator,
        buf: *std.ArrayList(u8),
        text: []const u8,
        replacement: []const u8,
        slots: [MAX_SLOTS]usize,
    ): ! {
        let i: usize = 0;
        while (i < replacement.len) {
            if (replacement[i] == '$' && i + 1 < replacement.len) {
                const next_c = replacement[i + 1];
                if (next_c == '$') {
                    buf.append(alloc, '$')?;
                    i += 2;
                    continue;
                }
                if (next_c >= '0' && next_c <= '9') {
                    const g = usize(next_c - '0');
                    if (g <= _.program.num_groups) {
                        const s = slots[g * 2];
                        const e = slots[g * 2 + 1];
                        if (s != UNSET_POS && e != UNSET_POS && s <= e && e <= text.len) {
                            buf.appendSlice(alloc, text[s..e])?;
                        }
                    }
                    i += 2;
                    continue;
                }
            }
            buf.append(alloc, replacement[i])?;
            i += 1;
        }
    }

    fn addThread(
        list: *std.ArrayList(Thread),
        pc: usize,
        slots: [MAX_SLOTS]usize,
        pos: usize,
        text_len: usize,
        visited_gen: []usize,
        gen: usize,
    ): ! {
        if (visited_gen[pc] == gen) {
            return;
        }
        visited_gen[pc] = gen;

        const inst = _.program.insts[pc];
        switch (inst.op) {
            .Jmp => {
                _.addThread(list, inst.out1, slots, pos, text_len, visited_gen, gen)?;
            },
            .Split => {
                _.addThread(list, inst.out1, slots, pos, text_len, visited_gen, gen)?;
                _.addThread(list, inst.out2, slots, pos, text_len, visited_gen, gen)?;
            },
            .Save => {
                let next_slots = emptySlots();
                for (si of 0..MAX_SLOTS) {
                    next_slots[si] = slots[si];
                }
                if (inst.slot < MAX_SLOTS) {
                    next_slots[inst.slot] = pos;
                }
                _.addThread(list, inst.out1, next_slots, pos, text_len, visited_gen, gen)?;
            },
            .AssertStart => {
                if (pos == 0) {
                    _.addThread(list, inst.out1, slots, pos, text_len, visited_gen, gen)?;
                }
            },
            .AssertEnd => {
                if (pos == text_len) {
                    _.addThread(list, inst.out1, slots, pos, text_len, visited_gen, gen)?;
                }
            },
            .Char | .Any | .Class | .Match => {
                list.append(_.alloc, Thread{ pc: pc, slots: slots })?;
            },
        }
    }

    fn runPikeVm(
        text: []const u8,
        start_pos: usize,
        anchored: bool,
        out_slots: *[MAX_SLOTS]usize,
    ): bool ! {
        const num_insts = _.program.insts.len;
        if (num_insts == 0) return false;

        let clist: std.ArrayList(Thread) = .empty;
        defer clist.deinit(_.alloc);

        let nlist: std.ArrayList(Thread) = .empty;
        defer nlist.deinit(_.alloc);

        let visited_gen = _.alloc.alloc(usize, num_insts)?;
        defer _.alloc.free(visited_gen);
        for (i of 0..num_insts) {
            visited_gen[i] = 0;
        }

        let gen: usize = 1;
        let matched = false;
        let best_slots = emptySlots();

        _.addThread(*clist, 0, emptySlots(), start_pos, text.len, visited_gen, gen)?;

        let pos: usize = start_pos;
        while (pos <= text.len) : (pos += 1) {
            if (clist.items.len == 0) break;

            gen += 1;
            nlist.clearRetainingCapacity();

            for (t of clist.items) {
                const inst = _.program.insts[t.pc];
                switch (inst.op) {
                    .Match => {
                        best_slots = t.slots;
                        matched = true;
                        break;
                    },
                    .Char => {
                        if (pos < text.len && text[pos] == inst.ch) {
                            _.addThread(*nlist, inst.out1, t.slots, pos + 1, text.len, visited_gen, gen)?;
                        }
                    },
                    .Any => {
                        if (pos < text.len && text[pos] != '\n') {
                            _.addThread(*nlist, inst.out1, t.slots, pos + 1, text.len, visited_gen, gen)?;
                        }
                    },
                    .Class => {
                        if (pos < text.len && _.program.classes[inst.class_idx].contains(text[pos])) {
                            _.addThread(*nlist, inst.out1, t.slots, pos + 1, text.len, visited_gen, gen)?;
                        }
                    },
                    else => {},
                }
            }

            // Unanchored search: spawn a lower-priority start thread at the next position
            // only if we haven't found a match yet.
            if (!anchored && !matched && pos < text.len) {
                _.addThread(*nlist, 0, emptySlots(), pos + 1, text.len, visited_gen, gen)?;
            }

            // Swap clist and nlist
            const tmp = clist;
            clist = nlist;
            nlist = tmp;
        }

        if (matched) {
            out_slots.* = best_slots;
        }
        return matched;
    }
}
