// Reference-hardened against rust-lang/regex (MIT/Apache-2.0)
// Source: cp/lib/regex/vm.cp (Anti-ReDoS Pike VM Regex Engine)
// Pure CP zero-C-dependency O(N*M) Thompson NFA / Pike VM execution engine
// with sparse-set deduplication, capture groups, inline flags (?i/?m/?s/?U),
// word boundaries (\b/\B), replaceAll ($0-$9), split, and cap:simd literal/first-byte prefiltering.

const std = @import("std");
import { Simd } from "cap:simd";
import {
    RegexError,
    Program,
    Compiler,
    Inst,
    Opcode,
    CharClass,
    Prefilter,
    PrefilterKind,
    MAX_SLOTS,
} from "./regexCompiler.cp";

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

fn isWordByte(c: u8): bool {
    return (c >= '0' && c <= '9') ||
        (c >= 'A' && c <= 'Z') ||
        (c >= 'a' && c <= 'z') ||
        c == '_';
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

    /// 返回当前编译程序启用的 SIMD 预过滤策略类型
    pub fn prefilterKind(): PrefilterKind {
        return _.program.prefilter.kind;
    }

    /// 是否启用了 SIMD 或锚点预过滤
    pub fn hasPrefilter(): bool {
        return _.program.prefilter.kind != PrefilterKind.None;
    }

    pub fn isMatch(text: []const u8): bool {
        return _.find(text) != null;
    }

    pub fn isFullMatch(text: []const u8): bool {
        let out_slots = emptySlots();
        const ok = _.runPikeVm(text, 0, true, false, *out_slots) catch return false;
        if (!ok) return false;
        return out_slots[0] == 0 && out_slots[1] == text.len;
    }

    pub fn find(text: []const u8): MatchSpan? {
        let out_slots = emptySlots();
        const ok = _.runPikeVm(text, 0, false, true, *out_slots) catch return null;
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

    /// 绕过 SIMD 预过滤的纯 PikeVM 查找（用于基准对照与 100% 语义等价验证）
    pub fn findNoPrefilter(text: []const u8): MatchSpan? {
        let out_slots = emptySlots();
        const ok = _.runPikeVm(text, 0, false, false, *out_slots) catch return null;
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
        let last_non_empty_end: usize? = null;
        let out_slots = emptySlots();

        while (search_pos <= text.len) {
            out_slots = emptySlots();
            const ok = _.runPikeVm(text, search_pos, false, true, *out_slots)?;
            if (!ok) break;

            const s = out_slots[0];
            const e = out_slots[1];
            if (s == UNSET_POS || e == UNSET_POS || s > e || e > text.len) break;

            // rust-lang/regex empty-match rule: disallow a zero-width match right at
            // the end offset of a preceding non-empty match.
            if (s == e && last_non_empty_end != null && s == last_non_empty_end.?) {
                search_pos = e + 1;
                continue;
            }

            matches.append(alloc, MatchSpan{
                start: s,
                end: e,
                text: text[s..e],
            })?;

            if (e > s) {
                last_non_empty_end = e;
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
        const ok = _.runPikeVm(text, 0, false, true, *out_slots)?;
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
            const ok = _.runPikeVm(text, search_pos, false, true, *out_slots)?;
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
            const ok = _.runPikeVm(text, search_pos, false, true, *out_slots)?;
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

    /// 使用 `cap:simd` 加速定位下一个可能匹配的候选字节偏移
    fn prefilterNext(text: []const u8, from_pos: usize): usize? {
        if (from_pos >= text.len) return null;
        const pf = _.program.prefilter;
        const slice = text[from_pos..];

        switch (pf.kind) {
            .SingleByte => {
                if (rel := Simd.indexOfByte(slice, pf.b0)) {
                    return from_pos + rel;
                }
                return null;
            },
            .TwoBytes => {
                if (rel := Simd.indexOfAny2(slice, pf.b0, pf.b1)) {
                    return from_pos + rel;
                }
                return null;
            },
            .FourBytes => {
                if (rel := Simd.indexOfAny4(slice, pf.b0, pf.b1, pf.b2, pf.b3)) {
                    return from_pos + rel;
                }
                return null;
            },
            .LiteralPrefix => {
                const plen = pf.prefix_len;
                let cur = from_pos;
                while (cur + plen <= text.len) {
                    const rem = text[cur .. text.len - plen + 1];
                    const rel = Simd.indexOfByte(rem, pf.b0) orelse return null;
                    const cand = cur + rel;
                    if (std.mem.eql(u8, text[cand .. cand + plen], pf.prefix[0..plen])) {
                        return cand;
                    }
                    cur = cand + 1;
                }
                return null;
            },
            else => return from_pos,
        }
    }

    fn addThread(
        list: *std.ArrayList(Thread),
        pc: usize,
        slots: [MAX_SLOTS]usize,
        text: []const u8,
        pos: usize,
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
                _.addThread(list, inst.out1, slots, text, pos, visited_gen, gen)?;
            },
            .Split => {
                _.addThread(list, inst.out1, slots, text, pos, visited_gen, gen)?;
                _.addThread(list, inst.out2, slots, text, pos, visited_gen, gen)?;
            },
            .Save => {
                let next_slots = emptySlots();
                for (si of 0..MAX_SLOTS) {
                    next_slots[si] = slots[si];
                }
                if (inst.slot < MAX_SLOTS) {
                    next_slots[inst.slot] = pos;
                }
                _.addThread(list, inst.out1, next_slots, text, pos, visited_gen, gen)?;
            },
            .AssertStart => {
                if (pos == 0) {
                    _.addThread(list, inst.out1, slots, text, pos, visited_gen, gen)?;
                }
            },
            .AssertEnd => {
                if (pos == text.len) {
                    _.addThread(list, inst.out1, slots, text, pos, visited_gen, gen)?;
                }
            },
            .AssertStartLine => {
                if (pos == 0 || (pos > 0 && text[pos - 1] == '\n')) {
                    _.addThread(list, inst.out1, slots, text, pos, visited_gen, gen)?;
                }
            },
            .AssertEndLine => {
                if (pos == text.len || text[pos] == '\n') {
                    _.addThread(list, inst.out1, slots, text, pos, visited_gen, gen)?;
                }
            },
            .AssertWordBoundary => {
                const prev_w = pos > 0 && isWordByte(text[pos - 1]);
                const curr_w = pos < text.len && isWordByte(text[pos]);
                if (prev_w != curr_w) {
                    _.addThread(list, inst.out1, slots, text, pos, visited_gen, gen)?;
                }
            },
            .AssertNonWordBoundary => {
                const prev_w = pos > 0 && isWordByte(text[pos - 1]);
                const curr_w = pos < text.len && isWordByte(text[pos]);
                if (prev_w == curr_w) {
                    _.addThread(list, inst.out1, slots, text, pos, visited_gen, gen)?;
                }
            },
            .Char | .Any | .AnyAll | .Class | .Match => {
                list.append(_.alloc, Thread{ pc: pc, slots: slots })?;
            },
        }
    }

    fn runPikeVm(
        text: []const u8,
        start_pos: usize,
        anchored_in: bool,
        use_prefilter: bool,
        out_slots: *[MAX_SLOTS]usize,
    ): bool ! {
        const num_insts = _.program.insts.len;
        if (num_insts == 0) return false;

        let anchored = anchored_in;
        const pf_kind = _.program.prefilter.kind;
        let simd_prefilter = false;

        if (!anchored && use_prefilter) {
            if (pf_kind == PrefilterKind.AnchoredStart) {
                if (start_pos > 0) return false;
                anchored = true;
            } else if (pf_kind != PrefilterKind.None) {
                simd_prefilter = true;
            }
        }

        let pos: usize = start_pos;
        if (simd_prefilter) {
            if (cand := _.prefilterNext(text, start_pos)) {
                pos = cand;
            } else {
                return false;
            }
        }

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

        _.addThread(*clist, 0, emptySlots(), text, pos, visited_gen, gen)?;

        while (pos <= text.len) {
            if (clist.items.len == 0) {
                if (anchored || matched || pos >= text.len) break;
                if (simd_prefilter) {
                    if (cand := _.prefilterNext(text, pos)) {
                        pos = cand;
                        gen += 1;
                        _.addThread(*clist, 0, emptySlots(), text, pos, visited_gen, gen)?;
                        if (clist.items.len == 0) {
                            pos += 1;
                            continue;
                        }
                    } else {
                        break;
                    }
                } else {
                    // Unanchored without prefilter: current zero-width assertion (e.g. \b) failed
                    // at `pos`, advance to `pos + 1` and seed a fresh start thread.
                    pos += 1;
                    gen += 1;
                    _.addThread(*clist, 0, emptySlots(), text, pos, visited_gen, gen)?;
                    continue;
                }
            }

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
                            _.addThread(*nlist, inst.out1, t.slots, text, pos + 1, visited_gen, gen)?;
                        }
                    },
                    .Any => {
                        if (pos < text.len && text[pos] != '\n') {
                            _.addThread(*nlist, inst.out1, t.slots, text, pos + 1, visited_gen, gen)?;
                        }
                    },
                    .AnyAll => {
                        if (pos < text.len) {
                            _.addThread(*nlist, inst.out1, t.slots, text, pos + 1, visited_gen, gen)?;
                        }
                    },
                    .Class => {
                        if (pos < text.len && _.program.classes[inst.class_idx].contains(text[pos])) {
                            _.addThread(*nlist, inst.out1, t.slots, text, pos + 1, visited_gen, gen)?;
                        }
                    },
                    else => {},
                }
            }

            // Unanchored search: if nlist is empty and SIMD prefilter is active, jump directly
            if (!anchored && !matched && pos < text.len) {
                if (simd_prefilter && nlist.items.len == 0) {
                    if (cand := _.prefilterNext(text, pos + 1)) {
                        pos = cand;
                        gen += 1;
                        clist.clearRetainingCapacity();
                        _.addThread(*clist, 0, emptySlots(), text, pos, visited_gen, gen)?;
                        continue;
                    } else {
                        break;
                    }
                }
                _.addThread(*nlist, 0, emptySlots(), text, pos + 1, visited_gen, gen)?;
            }

            const tmp = clist;
            clist = nlist;
            nlist = tmp;
            pos += 1;
        }

        if (matched) {
            out_slots.* = best_slots;
        }
        return matched;
    }
}
