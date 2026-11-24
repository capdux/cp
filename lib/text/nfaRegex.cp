// Reference-hardened against BurntSushi/regex-automata (MIT/Apache-2.0)
// Source: cp/lib/text/nfaRegex.cp
// Pure CP Linear-Time Regex Engine (SIMD Prefilter + Lazy DFA Cache + PikeVM / Thompson NFA)
// Guarantees O(n) worst-case linear time (ReDoS-free) with full production APIs:
//   - 16-lane @Vector(16, u8) SIMD Prefilter (LiteralPrefix, SingleByte, TwoBytes, FourBytes, Range, Bitmap)
//   - Fixed-capacity Lazy DFA transition cache (O(1) ASCII 0..127 lookup, lossless fallback to NFA on full)
//   - PikeVM leftmost-first engine supporting captures (...), non-capturing (?:...), (?i)/(?-i)/(?i:...),
//     \d/\D/\w/\W/\s/\S, ^/$/\b/\B, greedy & lazy quantifiers (*, +, ?, {m}, {m,}, {m,n}, *?, +?, ??, {m,n}?)
//   - Production APIs: compile, deinit, isMatch, find, findFirst, findAll, captures, replace, replaceAll, split

const std = @import("std");

type RegexVec16u8 = @Vector(16, u8);

pub const MAX_NFA_STATES: usize = 4096;
pub const MAX_CAPTURES: usize = 32;
pub const MAX_SLOTS: usize = MAX_CAPTURES * 2;
pub const MAX_REPEAT_BOUND: usize = 256;
pub const MAX_PREFIX_LEN: usize = 32;

const DFA_MAX_NFA_STATES: usize = 256;
const DFA_MAX_STATES: usize = 256;
const DFA_ASCII_SIZE: usize = 128;
const DFA_UNKNOWN: u16 = 0xFFFF;
const DFA_DEAD: u16 = 0xFFFE;
const DFA_OVERFLOW: u16 = 0xFFFD;

pub err RegexError {
    EmptyPattern,
    UnmatchedParen,
    UnclosedCharClass,
    TrailingEscape,
    NothingToRepeat,
    InvalidRange,
    InvalidQuantifier,
    InvalidFlag,
    PatternTooComplex,
    OutOfMemory,
}

pub struct MatchSpan {
    start: usize;
    end: usize;
}

pub struct Captures {
    count: usize = 0;
    groups: [MAX_CAPTURES](MatchSpan?) = [
        null, null, null, null, null, null, null, null,
        null, null, null, null, null, null, null, null,
        null, null, null, null, null, null, null, null,
        null, null, null, null, null, null, null, null,
    ];

    pub fn len(): usize {
        return _.count;
    }

    pub fn get(idx: usize): MatchSpan? {
        if (idx >= _.count || idx >= MAX_CAPTURES) return null;
        return _.groups[idx];
    }

    pub fn slice(input: []const u8, idx: usize): ([]const u8)? {
        if (span := _.get(idx)) {
            if (span.start <= span.end && span.end <= input.len) {
                return input[span.start..span.end];
            }
        }
        return null;
    }
}

pub enum PrefilterKind {
    None,
    LiteralPrefix,
    SingleByte,
    TwoBytes,
    FourBytes,
    ByteRange,
    StartByteBitmap,
}

pub struct SearchStats {
    dfa_steps: usize = 0;
    dfa_hits: usize = 0;
    dfa_misses: usize = 0;
    dfa_fallbacks: usize = 0;
    prefilter_skips: usize = 0;
}

// ============================================================================
// 16-lane @Vector(16, u8) SIMD Prefilter Primitives
// ============================================================================

fn regexLoadVec16(data: []const u8, off: usize): RegexVec16u8 {
    const v: RegexVec16u8 = [
        data[off], data[off + 1], data[off + 2], data[off + 3],
        data[off + 4], data[off + 5], data[off + 6], data[off + 7],
        data[off + 8], data[off + 9], data[off + 10], data[off + 11],
        data[off + 12], data[off + 13], data[off + 14], data[off + 15],
    ];
    return v;
}

pub fn simdIndexOfByteFrom(haystack: []const u8, start: usize, needle: u8): usize? {
    let i: usize = start;
    const n = haystack.len;
    const v_needle: RegexVec16u8 = @splat(needle);
    while (i + 16 <= n) {
        const chunk = regexLoadVec16(haystack, i);
        const mask: u16 = @bitCast(chunk == v_needle);
        if (mask != 0) {
            return i + (@ctz(mask) as usize);
        }
        i += 16;
    }
    while (i < n) : (i += 1) {
        if (haystack[i] == needle) return i;
    }
    return null;
}

pub fn simdIndexOfAny2From(haystack: []const u8, start: usize, a: u8, b: u8): usize? {
    let i: usize = start;
    const n = haystack.len;
    const va: RegexVec16u8 = @splat(a);
    const vb: RegexVec16u8 = @splat(b);
    while (i + 16 <= n) {
        const chunk = regexLoadVec16(haystack, i);
        const ma: u16 = @bitCast(chunk == va);
        const mb: u16 = @bitCast(chunk == vb);
        const mask: u16 = ma | mb;
        if (mask != 0) {
            return i + (@ctz(mask) as usize);
        }
        i += 16;
    }
    while (i < n) : (i += 1) {
        const ch = haystack[i];
        if (ch == a || ch == b) return i;
    }
    return null;
}

pub fn simdIndexOfAny4From(haystack: []const u8, start: usize, a: u8, b: u8, c: u8, d: u8): usize? {
    let i: usize = start;
    const n = haystack.len;
    const va: RegexVec16u8 = @splat(a);
    const vb: RegexVec16u8 = @splat(b);
    const vc: RegexVec16u8 = @splat(c);
    const vd: RegexVec16u8 = @splat(d);
    while (i + 16 <= n) {
        const chunk = regexLoadVec16(haystack, i);
        const ma: u16 = @bitCast(chunk == va);
        const mb: u16 = @bitCast(chunk == vb);
        const mc: u16 = @bitCast(chunk == vc);
        const md: u16 = @bitCast(chunk == vd);
        const mask: u16 = ma | mb | mc | md;
        if (mask != 0) {
            return i + (@ctz(mask) as usize);
        }
        i += 16;
    }
    while (i < n) : (i += 1) {
        const ch = haystack[i];
        if (ch == a || ch == b || ch == c || ch == d) return i;
    }
    return null;
}

pub fn simdIndexOfRangeFrom(haystack: []const u8, start: usize, lo: u8, hi: u8): usize? {
    let i: usize = start;
    const n = haystack.len;
    const v_lo: RegexVec16u8 = @splat(lo);
    const v_hi: RegexVec16u8 = @splat(hi);
    while (i + 16 <= n) {
        const chunk = regexLoadVec16(haystack, i);
        const m_lo: u16 = @bitCast(chunk >= v_lo);
        const m_hi: u16 = @bitCast(chunk <= v_hi);
        const mask: u16 = m_lo & m_hi;
        if (mask != 0) {
            return i + (@ctz(mask) as usize);
        }
        i += 16;
    }
    while (i < n) : (i += 1) {
        const ch = haystack[i];
        if (ch >= lo && ch <= hi) return i;
    }
    return null;
}

pub fn simdFindLiteralFrom(haystack: []const u8, start: usize, needle: []const u8): usize? {
    const m = needle.len;
    if (m == 0) return if (start <= haystack.len) start else null;
    if (m == 1) return simdIndexOfByteFrom(haystack, start, needle[0]);
    if (start + m > haystack.len) return null;

    const first_b = needle[0];
    const last_b = needle[m - 1];
    const last_off = m - 1;
    const v_first: RegexVec16u8 = @splat(first_b);
    const v_last: RegexVec16u8 = @splat(last_b);
    const limit = haystack.len - m + 1;

    let i: usize = start;
    while (i + 16 <= limit) {
        const c_first = regexLoadVec16(haystack, i);
        const c_last = regexLoadVec16(haystack, i + last_off);
        const mf: u16 = @bitCast(c_first == v_first);
        const ml: u16 = @bitCast(c_last == v_last);
        let mask: u16 = mf & ml;
        while (mask != 0) {
            const bit = @ctz(mask) as usize;
            const pos = i + bit;
            if (m <= 2 || std.mem.eql(u8, haystack[pos + 1 .. pos + last_off], needle[1..last_off])) {
                return pos;
            }
            mask &= mask - 1;
        }
        i += 16;
    }
    while (i < limit) : (i += 1) {
        if (haystack[i] == first_b && haystack[i + last_off] == last_b) {
            if (m <= 2 || std.mem.eql(u8, haystack[i + 1 .. i + last_off], needle[1..last_off])) {
                return i;
            }
        }
    }
    return null;
}

// ============================================================================
// Character Class & Byte Utilities
// ============================================================================

fn isWordByte(b: u8): bool {
    return (b >= 'a' && b <= 'z') || (b >= 'A' && b <= 'Z') || (b >= '0' && b <= '9') || b == '_';
}

fn toAsciiLower(b: u8): u8 {
    return if (b >= 'A' && b <= 'Z') b + 32 else b;
}

fn toAsciiUpper(b: u8): u8 {
    return if (b >= 'a' && b <= 'z') b - 32 else b;
}

struct CharClass {
    negated: bool = false;
    bits: [4]u64 = [ 0, 0, 0, 0 ];

    fn setBit(*_, ch: u8) {
        const word = (ch >> 6) as usize;
        const bit: u6 = (ch & 63) as u6;
        _.bits[word] |= ((1 as u64) << bit);
    }

    fn setBitWithCase(*_, ch: u8, case_insensitive: bool) {
        _.setBit(ch);
        if (case_insensitive) {
            _.setBit(toAsciiLower(ch));
            _.setBit(toAsciiUpper(ch));
        }
    }

    fn setRange(*_, lo: u8, hi: u8, case_insensitive: bool) {
        let c: u16 = lo;
        while (c <= hi) : (c += 1) {
            _.setBitWithCase(c as u8, case_insensitive);
        }
    }

    fn matches(ch: u8): bool {
        const word = (ch >> 6) as usize;
        const bit: u6 = (ch & 63) as u6;
        const hit = (_.bits[word] & ((1 as u64) << bit)) != 0;
        return if (_.negated) !hit else hit;
    }

    fn orInto(dst: *[4]u64) {
        if (!_.negated) {
            dst.*[0] |= _.bits[0];
            dst.*[1] |= _.bits[1];
            dst.*[2] |= _.bits[2];
            dst.*[3] |= _.bits[3];
        } else {
            dst.*[0] |= ~_.bits[0];
            dst.*[1] |= ~_.bits[1];
            dst.*[2] |= ~_.bits[2];
            dst.*[3] |= ~_.bits[3];
        }
    }
}

struct Prefilter {
    kind: PrefilterKind = PrefilterKind.None;
    prefix_buf: [MAX_PREFIX_LEN]u8 = [
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    ];
    prefix_len: usize = 0;
    bytes: [4]u8 = [ 0, 0, 0, 0 ];
    range_lo: u8 = 0;
    range_hi: u8 = 0;
    bitmap: [4]u64 = [ 0, 0, 0, 0 ];

    fn isActive(): bool {
        return _.kind != PrefilterKind.None;
    }

    fn nextCandidate(haystack: []const u8, start: usize): usize? {
        if (start >= haystack.len) return null;
        switch (_.kind) {
            PrefilterKind.None => return start,
            PrefilterKind.LiteralPrefix => {
                return simdFindLiteralFrom(haystack, start, _.prefix_buf[0.._.prefix_len]);
            },
            PrefilterKind.SingleByte => {
                return simdIndexOfByteFrom(haystack, start, _.bytes[0]);
            },
            PrefilterKind.TwoBytes => {
                return simdIndexOfAny2From(haystack, start, _.bytes[0], _.bytes[1]);
            },
            PrefilterKind.FourBytes => {
                return simdIndexOfAny4From(haystack, start, _.bytes[0], _.bytes[1], _.bytes[2], _.bytes[3]);
            },
            PrefilterKind.ByteRange => {
                return simdIndexOfRangeFrom(haystack, start, _.range_lo, _.range_hi);
            },
            PrefilterKind.StartByteBitmap => {
                let i = start;
                while (i < haystack.len) : (i += 1) {
                    const ch = haystack[i];
                    const word = (ch >> 6) as usize;
                    const bit: u6 = (ch & 63) as u6;
                    if ((_.bitmap[word] & ((1 as u64) << bit)) != 0) {
                        return i;
                    }
                }
                return null;
            },
        }
    }
}

// ============================================================================
// NFA State Representation
// ============================================================================

enum StateKind {
    literal,
    any_char,
    char_class,
    anchor_start,
    anchor_end,
    word_boundary,
    non_word_boundary,
    save,
    split,
    match_state,
}

struct NfaState {
    kind: StateKind;
    ch: u8 = 0;
    save_slot: u16 = 0;
    cclass: CharClass = CharClass{};
    out1: i32 = -1;
    out2: i32 = -1;
}

struct Frag {
    start: i32;
    outs: std.ArrayList(i32);

    fn deinit(*_, alloc: std.mem.Allocator) {
        _.outs.deinit(alloc);
    }
}

// ============================================================================
// Lazy DFA Cache (Fixed Capacity 256 States x 128 ASCII Bytes, O(1) Lookup)
// ============================================================================

struct NfaBitSet {
    words: [4]u64 = [ 0, 0, 0, 0 ];

    fn set(*_, sid: usize) {
        const w = sid >> 6;
        const b: u6 = (sid & 63) as u6;
        _.words[w] |= ((1 as u64) << b);
    }

    fn has(sid: usize): bool {
        const w = sid >> 6;
        const b: u6 = (sid & 63) as u6;
        return (_.words[w] & ((1 as u64) << b)) != 0;
    }

    fn isEmpty(): bool {
        return (_.words[0] | _.words[1] | _.words[2] | _.words[3]) == 0;
    }

    fn eql(other: NfaBitSet): bool {
        return _.words[0] == other.words[0] &&
            _.words[1] == other.words[1] &&
            _.words[2] == other.words[2] &&
            _.words[3] == other.words[3];
    }
}

struct LazyDfa {
    state_sets: [DFA_MAX_STATES]NfaBitSet;
    is_match: [DFA_MAX_STATES]bool;
    is_match_eof: [DFA_MAX_STATES]bool;
    trans: [DFA_MAX_STATES][DFA_ASCII_SIZE]u16;
    num_states: usize;
    start_id: u16;
    unanchored: bool;

    fn initInPlace(*_, unanchored: bool) {
        _.num_states = 0;
        _.start_id = DFA_DEAD;
        _.unanchored = unanchored;
    }

    fn internState(*_, states: []const NfaState, set: NfaBitSet, has_eof_anchor: bool): u16 {
        if (set.isEmpty()) return DFA_DEAD;
        let i: usize = 0;
        while (i < _.num_states) : (i += 1) {
            if (_.state_sets[i].eql(set)) {
                return i as u16;
            }
        }
        if (_.num_states >= DFA_MAX_STATES) {
            return DFA_OVERFLOW;
        }
        const sid = _.num_states;
        _.num_states += 1;
        _.state_sets[sid] = set;

        let matched = false;
        let w: usize = 0;
        while (w < 4) : (w += 1) {
            let word = set.words[w];
            while (word != 0) {
                const bit = @ctz(word) as usize;
                const nfa_id = (w << 6) + bit;
                if (nfa_id < states.len && states[nfa_id].kind == StateKind.match_state) {
                    matched = true;
                }
                word &= word - 1;
            }
        }
        _.is_match[sid] = matched;

        if (matched) {
            _.is_match_eof[sid] = true;
        } else if (has_eof_anchor) {
            _.is_match_eof[sid] = dfaSetCanReachMatchAtEof(states, set);
        } else {
            _.is_match_eof[sid] = false;
        }

        @memset(&_.trans[sid], DFA_UNKNOWN);
        return sid as u16;
    }

    fn computeStep(
        *_,
        states: []const NfaState,
        start_state: i32,
        curr_sid: u16,
        ch: u8,
        has_eof_anchor: bool,
    ): u16 {
        const curr_set = _.state_sets[curr_sid as usize];
        let next_set = NfaBitSet{};
        let stack: [DFA_MAX_NFA_STATES]i32 = undefined;

        let w: usize = 0;
        while (w < 4) : (w += 1) {
            let word = curr_set.words[w];
            while (word != 0) {
                const bit = @ctz(word) as usize;
                const uid = (w << 6) + bit;
                if (uid < states.len) {
                    const st = states[uid];
                    let hit = false;
                    switch (st.kind) {
                        StateKind.literal => hit = (ch == st.ch),
                        StateKind.any_char => hit = (ch != '\n'),
                        StateKind.char_class => hit = st.cclass.matches(ch),
                        else => {},
                    }
                    if (hit && st.out1 >= 0) {
                        dfaAddEpsClosure(states, &next_set, &stack, st.out1, false);
                    }
                }
                word &= word - 1;
            }
        }

        if (_.unanchored && start_state >= 0) {
            dfaAddEpsClosure(states, &next_set, &stack, start_state, false);
        }

        const next_sid = _.internState(states, next_set, has_eof_anchor);
        if (next_sid != DFA_OVERFLOW) {
            _.trans[curr_sid as usize][ch as usize] = next_sid;
        }
        return next_sid;
    }
}

fn dfaAddEpsClosure(
    states: []const NfaState,
    set: *NfaBitSet,
    stack: *[DFA_MAX_NFA_STATES]i32,
    init_id: i32,
    at_start: bool,
) {
    if (init_id < 0) return;
    let sp: usize = 0;
    stack.*[sp] = init_id;
    sp += 1;

    while (sp > 0) {
        sp -= 1;
        const sid = stack.*[sp];
        if (sid < 0) continue;
        const uid = sid as usize;
        if (uid >= states.len || uid >= DFA_MAX_NFA_STATES) continue;
        if (set.has(uid)) continue;
        set.set(uid);

        const st = states[uid];
        switch (st.kind) {
            StateKind.split => {
                if (st.out2 >= 0 && sp < DFA_MAX_NFA_STATES) {
                    stack.*[sp] = st.out2;
                    sp += 1;
                }
                if (st.out1 >= 0 && sp < DFA_MAX_NFA_STATES) {
                    stack.*[sp] = st.out1;
                    sp += 1;
                }
            },
            StateKind.save => {
                if (st.out1 >= 0 && sp < DFA_MAX_NFA_STATES) {
                    stack.*[sp] = st.out1;
                    sp += 1;
                }
            },
            StateKind.anchor_start => {
                if (at_start && st.out1 >= 0 && sp < DFA_MAX_NFA_STATES) {
                    stack.*[sp] = st.out1;
                    sp += 1;
                }
            },
            else => {},
        }
    }
}

fn dfaSetCanReachMatchAtEof(states: []const NfaState, set: NfaBitSet): bool {
    let visited = NfaBitSet{};
    let stack: [DFA_MAX_NFA_STATES]i32 = undefined;
    let sp: usize = 0;

    let w: usize = 0;
    while (w < 4) : (w += 1) {
        let word = set.words[w];
        while (word != 0) {
            const bit = @ctz(word) as usize;
            const uid = (w << 6) + bit;
            if (uid < states.len && sp < DFA_MAX_NFA_STATES) {
                stack[sp] = uid as i32;
                sp += 1;
            }
            word &= word - 1;
        }
    }

    while (sp > 0) {
        sp -= 1;
        const sid = stack[sp];
        if (sid < 0) continue;
        const uid = sid as usize;
        if (uid >= states.len || uid >= DFA_MAX_NFA_STATES) continue;
        if (visited.has(uid)) continue;
        visited.set(uid);

        const st = states[uid];
        switch (st.kind) {
            StateKind.match_state => return true,
            StateKind.split => {
                if (st.out2 >= 0 && sp < DFA_MAX_NFA_STATES) { stack[sp] = st.out2; sp += 1; }
                if (st.out1 >= 0 && sp < DFA_MAX_NFA_STATES) { stack[sp] = st.out1; sp += 1; }
            },
            StateKind.save => {
                if (st.out1 >= 0 && sp < DFA_MAX_NFA_STATES) { stack[sp] = st.out1; sp += 1; }
            },
            StateKind.anchor_end => {
                if (st.out1 >= 0 && sp < DFA_MAX_NFA_STATES) { stack[sp] = st.out1; sp += 1; }
            },
            else => {},
        }
    }
    return false;
}

// ============================================================================
// PikeVM Thread Representation (Strict O(n*m) Leftmost-First + Captures)
// ============================================================================

struct PikeThread {
    state: i32;
    slots: [MAX_SLOTS]isize;
}

fn initEmptySlots(): [MAX_SLOTS]isize {
    let slots: [MAX_SLOTS]isize = undefined;
    @memset(&slots, -1);
    return slots;
}

struct AnchoredDfaRes {
    used_dfa: bool;
    end_pos: usize?;
}

struct UnanchoredDfaRes {
    used_dfa: bool;
    matched: bool;
}

struct FindDfaRes {
    used_dfa: bool;
    span: MatchSpan?;
}

// ============================================================================
// Public NfaRegex Engine
// ============================================================================

pub struct NfaRegex {
    alloc: std.mem.Allocator;
    states: []NfaState;
    start_state: i32;
    anchored_start: bool;
    has_lazy: bool;
    has_alternation: bool;
    has_word_boundary: bool;
    has_inner_start_anchor: bool;
    has_end_anchor: bool;
    is_pure_literal: bool;
    num_captures: usize;
    prefilter: Prefilter;

    pub fn deinit() {
        _.alloc.free(_.states);
    }

    pub fn prefilterKind(): PrefilterKind {
        return _.prefilter.kind;
    }

    pub fn prefilterPrefix(): []const u8 {
        if (_.prefilter.kind == PrefilterKind.LiteralPrefix) {
            return _.prefilter.prefix_buf[0.._.prefilter.prefix_len];
        }
        return "";
    }

    pub fn captureGroupCount(): usize {
        return _.num_captures;
    }

    pub static fn compile(alloc: std.mem.Allocator, pattern: []const u8): RegexError!NfaRegex {
        let builder = Builder{
            alloc: alloc,
            pat: pattern,
            pos: 0,
            states: .empty,
            case_insensitive: false,
            has_lazy: false,
            has_alternation: false,
            has_word_boundary: false,
            has_start_anchor: false,
            has_end_anchor: false,
            num_captures: 1,
        };
        errdefer builder.states.deinit(alloc);

        if (pattern.len == 0) {
            const m_id = try builder.addState(NfaState{ kind: StateKind.match_state });
            return NfaRegex{
                alloc: alloc,
                states: try builder.states.toOwnedSlice(alloc),
                start_state: m_id,
                anchored_start: false,
                has_lazy: false,
                has_alternation: false,
                has_word_boundary: false,
                has_inner_start_anchor: false,
                has_end_anchor: false,
                is_pure_literal: false,
                num_captures: 1,
                prefilter: Prefilter{},
            };
        }

        let frag = try builder.parseAlternation(0);
        errdefer frag.deinit(alloc);

        if (builder.pos != pattern.len) {
            return RegexError.UnmatchedParen;
        }
        const match_id = try builder.addState(NfaState{ kind: StateKind.match_state });
        builder.patch(&frag.outs, match_id);
        frag.deinit(alloc);

        const owned_states = try builder.states.toOwnedSlice(alloc);
        const start_state = frag.start;
        const anchored = analyzeAnchoredStart(owned_states, start_state);
        const has_inner_start = builder.has_start_anchor && !anchored;
        const analysis = analyzePrefilter(owned_states, start_state);

        return NfaRegex{
            alloc: alloc,
            states: owned_states,
            start_state: start_state,
            anchored_start: anchored,
            has_lazy: builder.has_lazy,
            has_alternation: builder.has_alternation,
            has_word_boundary: builder.has_word_boundary,
            has_inner_start_anchor: has_inner_start,
            has_end_anchor: builder.has_end_anchor,
            is_pure_literal: analysis.is_pure_literal && !anchored && !builder.has_end_anchor && !builder.has_word_boundary,
            num_captures: builder.num_captures,
            prefilter: analysis.prefilter,
        };
    }

    fn canUseLazyDfa(): bool {
        return _.states.len <= DFA_MAX_NFA_STATES &&
            !_.has_word_boundary &&
            !_.has_inner_start_anchor;
    }

    // ------------------------------------------------------------------------
    // Public API: isMatch
    // ------------------------------------------------------------------------

    pub fn isMatch(alloc: std.mem.Allocator, input: []const u8): RegexError!bool {
        if (_.is_pure_literal && _.prefilter.kind == PrefilterKind.LiteralPrefix) {
            return simdFindLiteralFrom(input, 0, _.prefilter.prefix_buf[0.._.prefilter.prefix_len]) != null;
        }
        if (_.canUseLazyDfa()) {
            let stats = SearchStats{};
            if (_.anchored_start) {
                const res = try _.anchoredDfaSearch(alloc, input, 0, &stats);
                if (res.used_dfa) return res.end_pos != null;
            } else {
                const res = try _.unanchoredDfaIsMatch(alloc, input, &stats);
                if (res.used_dfa) return res.matched;
            }
        }
        const span = _.pikeFind(alloc, input, 0, false) catch (e) { return e; };
        return span != null;
    }

    // ------------------------------------------------------------------------
    // Public API: find / findFirst / findWithStats / findNfaOnly
    // ------------------------------------------------------------------------

    pub fn findFirst(alloc: std.mem.Allocator, input: []const u8): RegexError!(MatchSpan?) {
        return _.find(alloc, input);
    }

    pub fn find(alloc: std.mem.Allocator, input: []const u8): RegexError!(MatchSpan?) {
        let stats = SearchStats{};
        return _.findWithStats(alloc, input, &stats);
    }

    pub fn findWithStats(alloc: std.mem.Allocator, input: []const u8, stats: *SearchStats): RegexError!(MatchSpan?) {
        return _.findFromWithStats(alloc, input, 0, stats);
    }

    pub fn findNfaOnly(alloc: std.mem.Allocator, input: []const u8): RegexError!(MatchSpan?) {
        return _.pikeFind(alloc, input, 0, false);
    }

    fn findFromWithStats(
        alloc: std.mem.Allocator,
        input: []const u8,
        origin: usize,
        stats: *SearchStats,
    ): RegexError!(MatchSpan?) {
        if (origin > input.len) return null;
        if (_.anchored_start && origin > 0) return null;

        // Fast-path 1: Pure literal pattern -> direct 16B SIMD memmem
        if (_.is_pure_literal && _.prefilter.kind == PrefilterKind.LiteralPrefix) {
            const pfx = _.prefilter.prefix_buf[0.._.prefilter.prefix_len];
            if (pos := simdFindLiteralFrom(input, origin, pfx)) {
                stats.prefilter_skips += pos - origin;
                return MatchSpan{ start: pos, end: pos + pfx.len };
            }
            stats.prefilter_skips += input.len - origin;
            return null;
        }

        // Fast-path 2: Greedy pattern without alternation eligible for Lazy DFA + SIMD Prefilter
        if (_.canUseLazyDfa() && !_.has_lazy && !_.has_alternation) {
            if (_.anchored_start) {
                const res = try _.anchoredDfaSearch(alloc, input, 0, stats);
                if (res.used_dfa) {
                    if (end_p := res.end_pos) {
                        return MatchSpan{ start: 0, end: end_p };
                    }
                    return null;
                }
            } else {
                const dfa_span = try _.findGreedyLazyDfa(alloc, input, origin, stats);
                if (dfa_span.used_dfa) {
                    return dfa_span.span;
                }
                // If budget tripped on pathological input, do O(n) single-pass DFA no-match check first
                if (origin == 0) {
                    const check = try _.unanchoredDfaIsMatch(alloc, input, stats);
                    if (check.used_dfa && !check.matched) {
                        return null;
                    }
                }
            }
        }

        // Fallback / General path: Strict O(n*m) PikeVM with SIMD Prefilter skip
        return _.pikeFind(alloc, input, origin, true);
    }

    // ------------------------------------------------------------------------
    // Public API: findAll
    // ------------------------------------------------------------------------

    pub fn findAll(alloc: std.mem.Allocator, input: []const u8): RegexError![]MatchSpan {
        let results: std.ArrayList(MatchSpan) = .empty;
        errdefer results.deinit(alloc);

        let stats = SearchStats{};
        let pos: usize = 0;
        while (pos <= input.len) {
            if (_.anchored_start && pos > 0) break;
            const m_opt = _.findFromWithStats(alloc, input, pos, &stats) catch (e) { return e; };
            if (m := m_opt) {
                try results.append(alloc, m);
                pos = if (m.end > m.start) m.end else m.end + 1;
            } else {
                break;
            }
        }
        return results.toOwnedSlice(alloc);
    }

    // ------------------------------------------------------------------------
    // Public API: captures
    // ------------------------------------------------------------------------

    pub fn captures(alloc: std.mem.Allocator, input: []const u8): RegexError!(Captures?) {
        return _.capturesFrom(alloc, input, 0);
    }

    fn capturesFrom(alloc: std.mem.Allocator, input: []const u8, origin: usize): RegexError!(Captures?) {
        if (origin > input.len) return null;
        if (_.anchored_start && origin > 0) return null;

        let out_slots = initEmptySlots();
        const found = try _.pikeExec(alloc, input, origin, true, &out_slots);
        if (!found) return null;

        let caps = Captures{ count: _.num_captures };
        let g: usize = 0;
        while (g < _.num_captures && g < MAX_CAPTURES) : (g += 1) {
            const s = out_slots[g * 2];
            const e = out_slots[g * 2 + 1];
            if (s >= 0 && e >= s) {
                caps.groups[g] = MatchSpan{ start: s as usize, end: e as usize };
            }
        }
        return caps;
    }

    // ------------------------------------------------------------------------
    // Public API: replace / replaceAll / split
    // ------------------------------------------------------------------------

    pub fn replace(alloc: std.mem.Allocator, input: []const u8, replacement: []const u8): RegexError![]u8 {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        const caps_opt = _.capturesFrom(alloc, input, 0) catch (e) { return e; };
        if (caps := caps_opt) {
            const m0 = caps.get(0).?;
            try out.appendSlice(alloc, input[0..m0.start]);
            try appendExpandedReplacement(alloc, &out, input, &caps, replacement);
            try out.appendSlice(alloc, input[m0.end..]);
        } else {
            try out.appendSlice(alloc, input);
        }
        return out.toOwnedSlice(alloc);
    }

    pub fn replaceAll(alloc: std.mem.Allocator, input: []const u8, replacement: []const u8): RegexError![]u8 {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        let last_end: usize = 0;
        let search_pos: usize = 0;

        while (search_pos <= input.len) {
            if (_.anchored_start && search_pos > 0) break;
            const caps_opt = _.capturesFrom(alloc, input, search_pos) catch (e) { return e; };
            if (caps := caps_opt) {
                const m0 = caps.get(0).?;
                try out.appendSlice(alloc, input[last_end..m0.start]);
                try appendExpandedReplacement(alloc, &out, input, &caps, replacement);
                last_end = m0.end;
                if (m0.end > m0.start) {
                    search_pos = m0.end;
                } else {
                    if (m0.end < input.len) {
                        try out.append(alloc, input[m0.end]);
                        last_end = m0.end + 1;
                    }
                    search_pos = m0.end + 1;
                }
            } else {
                break;
            }
        }
        if (last_end <= input.len) {
            try out.appendSlice(alloc, input[last_end..]);
        }
        return out.toOwnedSlice(alloc);
    }

    pub fn split(alloc: std.mem.Allocator, input: []const u8): RegexError![][]const u8 {
        let parts: std.ArrayList([]const u8) = .empty;
        errdefer parts.deinit(alloc);

        let stats = SearchStats{};
        let last_end: usize = 0;
        let search_pos: usize = 0;

        while (search_pos <= input.len) {
            if (_.anchored_start && search_pos > 0) break;
            const m_opt = _.findFromWithStats(alloc, input, search_pos, &stats) catch (e) { return e; };
            if (m := m_opt) {
                try parts.append(alloc, input[last_end..m.start]);
                last_end = m.end;
                search_pos = if (m.end > m.start) m.end else m.end + 1;
            } else {
                break;
            }
        }
        try parts.append(alloc, input[last_end..]);
        return parts.toOwnedSlice(alloc);
    }

    // ------------------------------------------------------------------------
    // Lazy DFA Execution Paths (O(1) ASCII Lookup + Lossless NFA Fallback)
    // ------------------------------------------------------------------------

    fn anchoredDfaSearch(
        alloc: std.mem.Allocator,
        input: []const u8,
        origin: usize,
        stats: *SearchStats,
    ): RegexError!AnchoredDfaRes {
        const dfa = try alloc.create(LazyDfa);
        defer alloc.destroy(dfa);
        dfa.initInPlace(false);

        let start_set = NfaBitSet{};
        let stack: [DFA_MAX_NFA_STATES]i32 = undefined;
        dfaAddEpsClosure(_.states, &start_set, &stack, _.start_state, origin == 0);
        const init_sid = dfa.internState(_.states, start_set, _.has_end_anchor);
        if (init_sid == DFA_OVERFLOW) {
            stats.dfa_fallbacks += 1;
            return AnchoredDfaRes{ used_dfa: false, end_pos: null };
        }
        if (init_sid == DFA_DEAD) {
            return AnchoredDfaRes{ used_dfa: true, end_pos: null };
        }

        let curr_sid = init_sid;
        let best_end: usize? = null;
        if (dfa.is_match[curr_sid as usize]) {
            best_end = origin;
        }
        if (origin == input.len && dfa.is_match_eof[curr_sid as usize]) {
            best_end = origin;
        }

        let pos: usize = origin;
        while (pos < input.len) : (pos += 1) {
            const ch = input[pos];
            if (ch >= DFA_ASCII_SIZE) {
                stats.dfa_fallbacks += 1;
                return AnchoredDfaRes{ used_dfa: false, end_pos: null };
            }
            stats.dfa_steps += 1;
            let next_sid = dfa.trans[curr_sid as usize][ch as usize];
            if (next_sid == DFA_UNKNOWN) {
                stats.dfa_misses += 1;
                next_sid = dfa.computeStep(_.states, _.start_state, curr_sid, ch, _.has_end_anchor);
                if (next_sid == DFA_OVERFLOW) {
                    stats.dfa_fallbacks += 1;
                    return AnchoredDfaRes{ used_dfa: false, end_pos: null };
                }
            } else {
                stats.dfa_hits += 1;
            }
            if (next_sid == DFA_DEAD) break;
            curr_sid = next_sid;
            if (dfa.is_match[curr_sid as usize]) {
                best_end = pos + 1;
            }
            if (pos + 1 == input.len && dfa.is_match_eof[curr_sid as usize]) {
                best_end = pos + 1;
            }
        }
        return AnchoredDfaRes{ used_dfa: true, end_pos: best_end };
    }

    fn unanchoredDfaIsMatch(
        alloc: std.mem.Allocator,
        input: []const u8,
        stats: *SearchStats,
    ): RegexError!UnanchoredDfaRes {
        const dfa = try alloc.create(LazyDfa);
        defer alloc.destroy(dfa);
        dfa.initInPlace(true);

        let start_set = NfaBitSet{};
        let stack: [DFA_MAX_NFA_STATES]i32 = undefined;
        dfaAddEpsClosure(_.states, &start_set, &stack, _.start_state, true);
        const start_sid = dfa.internState(_.states, start_set, _.has_end_anchor);
        if (start_sid == DFA_OVERFLOW) {
            stats.dfa_fallbacks += 1;
            return UnanchoredDfaRes{ used_dfa: false, matched: false };
        }
        if (start_sid == DFA_DEAD) {
            return UnanchoredDfaRes{ used_dfa: true, matched: false };
        }
        if (dfa.is_match[start_sid as usize]) {
            return UnanchoredDfaRes{ used_dfa: true, matched: true };
        }
        if (input.len == 0 && dfa.is_match_eof[start_sid as usize]) {
            return UnanchoredDfaRes{ used_dfa: true, matched: true };
        }

        let curr_sid = start_sid;
        let pos: usize = 0;
        while (pos < input.len) {
            // When in start state and prefilter is active, jump via 16B SIMD
            if (curr_sid == start_sid && _.prefilter.isActive()) {
                if (cand := _.prefilter.nextCandidate(input, pos)) {
                    stats.prefilter_skips += cand - pos;
                    pos = cand;
                } else {
                    stats.prefilter_skips += input.len - pos;
                    return UnanchoredDfaRes{ used_dfa: true, matched: false };
                }
            }
            const ch = input[pos];
            if (ch >= DFA_ASCII_SIZE) {
                stats.dfa_fallbacks += 1;
                return UnanchoredDfaRes{ used_dfa: false, matched: false };
            }
            stats.dfa_steps += 1;
            let next_sid = dfa.trans[curr_sid as usize][ch as usize];
            if (next_sid == DFA_UNKNOWN) {
                stats.dfa_misses += 1;
                next_sid = dfa.computeStep(_.states, _.start_state, curr_sid, ch, _.has_end_anchor);
                if (next_sid == DFA_OVERFLOW) {
                    stats.dfa_fallbacks += 1;
                    return UnanchoredDfaRes{ used_dfa: false, matched: false };
                }
            } else {
                stats.dfa_hits += 1;
            }
            if (next_sid == DFA_DEAD) {
                curr_sid = start_sid;
                pos += 1;
                continue;
            }
            curr_sid = next_sid;
            if (dfa.is_match[curr_sid as usize]) {
                return UnanchoredDfaRes{ used_dfa: true, matched: true };
            }
            if (pos + 1 == input.len && dfa.is_match_eof[curr_sid as usize]) {
                return UnanchoredDfaRes{ used_dfa: true, matched: true };
            }
            pos += 1;
        }
        return UnanchoredDfaRes{ used_dfa: true, matched: false };
    }

    fn findGreedyLazyDfa(
        alloc: std.mem.Allocator,
        input: []const u8,
        origin: usize,
        stats: *SearchStats,
    ): RegexError!FindDfaRes {
        const dfa = try alloc.create(LazyDfa);
        defer alloc.destroy(dfa);
        dfa.initInPlace(false);

        let start_set = NfaBitSet{};
        let stack: [DFA_MAX_NFA_STATES]i32 = undefined;
        dfaAddEpsClosure(_.states, &start_set, &stack, _.start_state, origin == 0);
        const start_sid = dfa.internState(_.states, start_set, _.has_end_anchor);
        if (start_sid == DFA_OVERFLOW) {
            stats.dfa_fallbacks += 1;
            return FindDfaRes{ used_dfa: false, span: null };
        }
        if (start_sid == DFA_DEAD) {
            return FindDfaRes{ used_dfa: true, span: null };
        }

        // Guard budget against quadratic worst-case (e.g., unanchored (a+)+b on 5000 'a's)
        const fail_budget: usize = (input.len - origin) + 128;
        let fail_steps: usize = 0;

        let cand: usize = origin;
        while (cand <= input.len) {
            if (_.prefilter.isActive() && cand < input.len) {
                if (next_c := _.prefilter.nextCandidate(input, cand)) {
                    stats.prefilter_skips += next_c - cand;
                    cand = next_c;
                } else {
                    stats.prefilter_skips += input.len - cand;
                    return FindDfaRes{ used_dfa: true, span: null };
                }
            }

            let curr_sid = start_sid;
            let best_end: usize? = null;
            if (dfa.is_match[curr_sid as usize]) {
                best_end = cand;
            }
            if (cand == input.len && dfa.is_match_eof[curr_sid as usize]) {
                best_end = cand;
            }

            let pos: usize = cand;
            while (pos < input.len) : (pos += 1) {
                const ch = input[pos];
                if (ch >= DFA_ASCII_SIZE) {
                    stats.dfa_fallbacks += 1;
                    return FindDfaRes{ used_dfa: false, span: null };
                }
                stats.dfa_steps += 1;
                let next_sid = dfa.trans[curr_sid as usize][ch as usize];
                if (next_sid == DFA_UNKNOWN) {
                    stats.dfa_misses += 1;
                    next_sid = dfa.computeStep(_.states, _.start_state, curr_sid, ch, _.has_end_anchor);
                    if (next_sid == DFA_OVERFLOW) {
                        stats.dfa_fallbacks += 1;
                        return FindDfaRes{ used_dfa: false, span: null };
                    }
                } else {
                    stats.dfa_hits += 1;
                }
                if (next_sid == DFA_DEAD) break;
                curr_sid = next_sid;
                if (dfa.is_match[curr_sid as usize]) {
                    best_end = pos + 1;
                }
                if (pos + 1 == input.len && dfa.is_match_eof[curr_sid as usize]) {
                    best_end = pos + 1;
                }
            }

            if (end_p := best_end) {
                return FindDfaRes{ used_dfa: true, span: MatchSpan{ start: cand, end: end_p } };
            }

            fail_steps += (pos - cand) + 1;
            if (fail_steps > fail_budget) {
                // Switch to single-pass O(n) unanchored DFA check + PikeVM fallback
                stats.dfa_fallbacks += 1;
                return FindDfaRes{ used_dfa: false, span: null };
            }
            cand += 1;
        }
        return FindDfaRes{ used_dfa: true, span: null };
    }

    // ------------------------------------------------------------------------
    // Strict O(n*m) PikeVM (Leftmost-First + Captures + Lazy Quantifiers + \b)
    // ------------------------------------------------------------------------

    fn pikeFind(
        alloc: std.mem.Allocator,
        input: []const u8,
        origin: usize,
        use_prefilter: bool,
    ): RegexError!(MatchSpan?) {
        let slots = initEmptySlots();
        const found = try _.pikeExec(alloc, input, origin, use_prefilter, &slots);
        if (!found) return null;
        return MatchSpan{ start: slots[0] as usize, end: slots[1] as usize };
    }

    fn pikeExec(
        alloc: std.mem.Allocator,
        input: []const u8,
        origin: usize,
        use_prefilter: bool,
        out_slots: *[MAX_SLOTS]isize,
    ): RegexError!bool {
        const n_states = _.states.len;
        const gen_buf = try alloc.alloc(u32, n_states);
        defer alloc.free(gen_buf);
        @memset(gen_buf, 0);

        let curr: std.ArrayList(PikeThread) = .empty;
        defer curr.deinit(alloc);
        let next: std.ArrayList(PikeThread) = .empty;
        defer next.deinit(alloc);
        let eps_stack: std.ArrayList(PikeThread) = .empty;
        defer eps_stack.deinit(alloc);

        let gen: u32 = 1;
        let has_best = false;
        let best_slots = initEmptySlots();

        let pos: usize = origin;
        while (pos <= input.len) {
            // SIMD Prefilter jump when no active threads and no match found yet
            if (curr.items.len == 0) {
                if (has_best) break;
                if (_.anchored_start && pos > 0) break;
                if (use_prefilter && _.prefilter.isActive() && pos < input.len) {
                    if (cand := _.prefilter.nextCandidate(input, pos)) {
                        pos = cand;
                    } else {
                        break;
                    }
                }
            }

            // Inject a new start thread at current pos (lowest priority)
            if (!has_best && (!_.anchored_start || pos == 0)) {
                let start_slots = initEmptySlots();
                start_slots[0] = pos as isize;
                try pikeAddThread(
                    _.states,
                    &curr,
                    &eps_stack,
                    alloc,
                    gen_buf,
                    gen,
                    _.start_state,
                    start_slots,
                    input,
                    pos,
                );
            }

            // Scan active threads in leftmost-first priority order
            gen += 1;
            next.items.len = 0;

            let i: usize = 0;
            while (i < curr.items.len) : (i += 1) {
                const th = curr.items[i];
                const uid = th.state as usize;
                const st = _.states[uid];
                if (st.kind == StateKind.match_state) {
                    best_slots = th.slots;
                    best_slots[1] = pos as isize;
                    has_best = true;
                    // Prune all lower-priority threads at this position
                    break;
                }
                if (pos < input.len) {
                    const ch = input[pos];
                    let matched = false;
                    switch (st.kind) {
                        StateKind.literal => matched = (ch == st.ch),
                        StateKind.any_char => matched = (ch != '\n'),
                        StateKind.char_class => matched = st.cclass.matches(ch),
                        else => {},
                    }
                    if (matched && st.out1 >= 0) {
                        try pikeAddThread(
                            _.states,
                            &next,
                            &eps_stack,
                            alloc,
                            gen_buf,
                            gen,
                            st.out1,
                            th.slots,
                            input,
                            pos + 1,
                        );
                    }
                }
            }

            const tmp = curr;
            curr = next;
            next = tmp;

            if (pos == input.len) break;
            pos += 1;
        }

        if (has_best) {
            out_slots.* = best_slots;
            return true;
        }
        return false;
    }
}

fn pikeAddThread(
    states: []const NfaState,
    list: *std.ArrayList(PikeThread),
    stack: *std.ArrayList(PikeThread),
    alloc: std.mem.Allocator,
    gen_buf: []u32,
    gen: u32,
    init_sid: i32,
    init_slots: [MAX_SLOTS]isize,
    input: []const u8,
    pos: usize,
): RegexError!void {
    if (init_sid < 0) return;
    stack.items.len = 0;
    try stack.append(alloc, PikeThread{ state: init_sid, slots: init_slots });

    while (stack.items.len > 0) {
        const cur = stack.pop().?;
        if (cur.state < 0) continue;
        const uid = cur.state as usize;
        if (uid >= states.len) continue;
        if (gen_buf[uid] == gen) continue;
        gen_buf[uid] = gen;

        const st = states[uid];
        switch (st.kind) {
            StateKind.split => {
                // Push out2 first so out1 is popped first (leftmost-first priority)
                if (st.out2 >= 0) {
                    try stack.append(alloc, PikeThread{ state: st.out2, slots: cur.slots });
                }
                if (st.out1 >= 0) {
                    try stack.append(alloc, PikeThread{ state: st.out1, slots: cur.slots });
                }
            },
            StateKind.save => {
                if (st.out1 >= 0) {
                    let next_slots = cur.slots;
                    const slot_idx = st.save_slot as usize;
                    if (slot_idx < MAX_SLOTS) {
                        next_slots[slot_idx] = pos as isize;
                    }
                    try stack.append(alloc, PikeThread{ state: st.out1, slots: next_slots });
                }
            },
            StateKind.anchor_start => {
                if (pos == 0 && st.out1 >= 0) {
                    try stack.append(alloc, PikeThread{ state: st.out1, slots: cur.slots });
                }
            },
            StateKind.anchor_end => {
                if (pos == input.len && st.out1 >= 0) {
                    try stack.append(alloc, PikeThread{ state: st.out1, slots: cur.slots });
                }
            },
            StateKind.word_boundary => {
                const prev_w = (pos > 0) && isWordByte(input[pos - 1]);
                const curr_w = (pos < input.len) && isWordByte(input[pos]);
                if (prev_w != curr_w && st.out1 >= 0) {
                    try stack.append(alloc, PikeThread{ state: st.out1, slots: cur.slots });
                }
            },
            StateKind.non_word_boundary => {
                const prev_w = (pos > 0) && isWordByte(input[pos - 1]);
                const curr_w = (pos < input.len) && isWordByte(input[pos]);
                if (prev_w == curr_w && st.out1 >= 0) {
                    try stack.append(alloc, PikeThread{ state: st.out1, slots: cur.slots });
                }
            },
            else => {
                try list.append(alloc, cur);
            },
        }
    }
}

fn appendExpandedReplacement(
    alloc: std.mem.Allocator,
    out: *std.ArrayList(u8),
    input: []const u8,
    caps: *const Captures,
    replacement: []const u8,
): RegexError!void {
    let i: usize = 0;
    while (i < replacement.len) {
        if (replacement[i] == '$' && i + 1 < replacement.len) {
            const nxt = replacement[i + 1];
            if (nxt == '$') {
                try out.append(alloc, '$');
                i += 2;
                continue;
            }
            if (nxt == '&') {
                if (s := caps.slice(input, 0)) {
                    try out.appendSlice(alloc, s);
                }
                i += 2;
                continue;
            }
            if (nxt >= '0' && nxt <= '9') {
                const g = (nxt - '0') as usize;
                if (s := caps.slice(input, g)) {
                    try out.appendSlice(alloc, s);
                }
                i += 2;
                continue;
            }
        }
        try out.append(alloc, replacement[i]);
        i += 1;
    }
}

// ============================================================================
// Static Analysis: Anchored Start & SIMD Prefilter Extraction
// ============================================================================

fn analyzeAnchoredStart(states: []const NfaState, start_state: i32): bool {
    if (start_state < 0) return false;
    let visited: [MAX_NFA_STATES]bool = undefined;
    const n = states.len;
    @memset(visited[0..n], false);

    let stack: [MAX_NFA_STATES]i32 = undefined;
    let sp: usize = 0;
    stack[sp] = start_state;
    sp += 1;

    let saw_anchor = false;
    while (sp > 0) {
        sp -= 1;
        const sid = stack[sp];
        if (sid < 0) continue;
        const uid = sid as usize;
        if (uid >= n || visited[uid]) continue;
        visited[uid] = true;

        const st = states[uid];
        switch (st.kind) {
            StateKind.anchor_start => {
                saw_anchor = true;
            },
            StateKind.split => {
                if (st.out1 >= 0 && sp < MAX_NFA_STATES) { stack[sp] = st.out1; sp += 1; }
                if (st.out2 >= 0 && sp < MAX_NFA_STATES) { stack[sp] = st.out2; sp += 1; }
            },
            StateKind.save => {
                if (st.out1 >= 0 && sp < MAX_NFA_STATES) { stack[sp] = st.out1; sp += 1; }
            },
            else => {
                // Reached a consuming state or match_state without passing anchor_start
                return false;
            },
        }
    }
    return saw_anchor;
}

struct PrefilterAnalysis {
    prefilter: Prefilter;
    is_pure_literal: bool;
}

fn analyzePrefilter(states: []const NfaState, start_state: i32): PrefilterAnalysis {
    let pf = Prefilter{};
    if (start_state < 0) {
        return PrefilterAnalysis{ prefilter: pf, is_pure_literal: false };
    }

    // 1. Try extracting a mandatory linear literal prefix
    let curr: i32 = start_state;
    let steps: usize = 0;
    let pfx_len: usize = 0;
    let pure_lit = false;

    while (curr >= 0 && steps < 128) : (steps += 1) {
        const uid = curr as usize;
        if (uid >= states.len) break;
        const st = states[uid];
        if (st.kind == StateKind.save || st.kind == StateKind.anchor_start) {
            curr = st.out1;
            continue;
        }
        if (st.kind == StateKind.literal) {
            if (pfx_len < MAX_PREFIX_LEN) {
                pf.prefix_buf[pfx_len] = st.ch;
                pfx_len += 1;
                curr = st.out1;
                continue;
            } else {
                break;
            }
        }
        if (st.kind == StateKind.match_state && pfx_len > 0) {
            pure_lit = true;
        }
        break;
    }

    if (pfx_len >= 2) {
        pf.kind = PrefilterKind.LiteralPrefix;
        pf.prefix_len = pfx_len;
        return PrefilterAnalysis{ prefilter: pf, is_pure_literal: pure_lit };
    }
    if (pfx_len == 1) {
        pf.kind = PrefilterKind.SingleByte;
        pf.bytes[0] = pf.prefix_buf[0];
        pf.prefix_len = 1;
        return PrefilterAnalysis{ prefilter: pf, is_pure_literal: pure_lit };
    }

    // 2. General start-byte bitmap analysis via epsilon DFS
    let visited: [MAX_NFA_STATES]bool = undefined;
    const n = states.len;
    @memset(visited[0..n], false);

    let stack: [MAX_NFA_STATES]i32 = undefined;
    let sp: usize = 0;
    stack[sp] = start_state;
    sp += 1;

    let bitmap: [4]u64 = [ 0, 0, 0, 0 ];
    while (sp > 0) {
        sp -= 1;
        const sid = stack[sp];
        if (sid < 0) continue;
        const uid = sid as usize;
        if (uid >= n || visited[uid]) continue;
        visited[uid] = true;

        const st = states[uid];
        switch (st.kind) {
            StateKind.split => {
                if (st.out2 >= 0 && sp < MAX_NFA_STATES) { stack[sp] = st.out2; sp += 1; }
                if (st.out1 >= 0 && sp < MAX_NFA_STATES) { stack[sp] = st.out1; sp += 1; }
            },
            StateKind.save, StateKind.anchor_start, StateKind.word_boundary, StateKind.non_word_boundary => {
                if (st.out1 >= 0 && sp < MAX_NFA_STATES) { stack[sp] = st.out1; sp += 1; }
            },
            StateKind.literal => {
                const w = (st.ch >> 6) as usize;
                const b: u6 = (st.ch & 63) as u6;
                bitmap[w] |= ((1 as u64) << b);
            },
            StateKind.char_class => {
                st.cclass.orInto(&bitmap);
            },
            StateKind.any_char, StateKind.match_state, StateKind.anchor_end => {
                // Can match empty or almost any byte -> no useful start-byte prefilter
                return PrefilterAnalysis{ prefilter: Prefilter{}, is_pure_literal: false };
            },
        }
    }

    // Analyze collected start bytes
    let count: usize = 0;
    let first_bytes: [4]u8 = [ 0, 0, 0, 0 ];
    let min_b: u16 = 256;
    let max_b: u16 = 0;

    let b_idx: u16 = 0;
    while (b_idx < 256) : (b_idx += 1) {
        const w = (b_idx >> 6) as usize;
        const bit: u6 = (b_idx & 63) as u6;
        if ((bitmap[w] & ((1 as u64) << bit)) != 0) {
            if (count < 4) {
                first_bytes[count] = b_idx as u8;
            }
            if (b_idx < min_b) min_b = b_idx;
            if (b_idx > max_b) max_b = b_idx;
            count += 1;
        }
    }

    if (count == 0 || count > 96) {
        return PrefilterAnalysis{ prefilter: Prefilter{}, is_pure_literal: false };
    }
    if (count == 1) {
        pf.kind = PrefilterKind.SingleByte;
        pf.bytes[0] = first_bytes[0];
    } else if (count == 2) {
        pf.kind = PrefilterKind.TwoBytes;
        pf.bytes[0] = first_bytes[0];
        pf.bytes[1] = first_bytes[1];
    } else if (count <= 4) {
        pf.kind = PrefilterKind.FourBytes;
        pf.bytes[0] = first_bytes[0];
        pf.bytes[1] = first_bytes[1];
        pf.bytes[2] = first_bytes[2];
        pf.bytes[3] = if (count == 4) first_bytes[3] else first_bytes[0];
    } else if ((max_b - min_b + 1) as usize == count) {
        // Contiguous byte range (e.g., [0-9], [a-z], [A-Z]) -> 16B SIMD range scan
        pf.kind = PrefilterKind.ByteRange;
        pf.range_lo = min_b as u8;
        pf.range_hi = max_b as u8;
    } else {
        pf.kind = PrefilterKind.StartByteBitmap;
        pf.bitmap = bitmap;
    }
    return PrefilterAnalysis{ prefilter: pf, is_pure_literal: false };
}

// ============================================================================
// Pattern Parser & Thompson NFA Compiler
// ============================================================================

struct Builder {
    alloc: std.mem.Allocator;
    pat: []const u8;
    pos: usize;
    states: std.ArrayList(NfaState);
    case_insensitive: bool;
    has_lazy: bool;
    has_alternation: bool;
    has_word_boundary: bool;
    has_start_anchor: bool;
    has_end_anchor: bool;
    num_captures: usize;

    fn addState(*_, st: NfaState): RegexError!i32 {
        if (_.states.items.len >= MAX_NFA_STATES) {
            return RegexError.PatternTooComplex;
        }
        const id = _.states.items.len as i32;
        try _.states.append(_.alloc, st);
        return id;
    }

    fn patch(*_, outs: *const std.ArrayList(i32), target: i32) {
        for (sid of outs.items) {
            const uid = sid as usize;
            if (_.states.items[uid].out1 == -1) {
                _.states.items[uid].out1 = target;
            } else if (_.states.items[uid].out2 == -1) {
                _.states.items[uid].out2 = target;
            }
        }
    }

    fn singleState(*_, st: NfaState): RegexError!Frag {
        const id = try _.addState(st);
        let outs: std.ArrayList(i32) = .empty;
        errdefer outs.deinit(_.alloc);
        try outs.append(_.alloc, id);
        return Frag{ start: id, outs: outs };
    }

    fn emptyFrag(*_): RegexError!Frag {
        const id = try _.addState(NfaState{ kind: StateKind.split, out1: -1, out2: -1 });
        let outs: std.ArrayList(i32) = .empty;
        errdefer outs.deinit(_.alloc);
        try outs.append(_.alloc, id);
        return Frag{ start: id, outs: outs };
    }

    fn cloneFrag(*_, orig_start_idx: usize, orig_end_idx: usize, orig: *const Frag): RegexError!Frag {
        const count = orig_end_idx - orig_start_idx;
        if (_.states.items.len + count > MAX_NFA_STATES) {
            return RegexError.PatternTooComplex;
        }
        const base = _.states.items.len;
        const delta: i32 = (base as i32) - (orig_start_idx as i32);

        let i: usize = orig_start_idx;
        while (i < orig_end_idx) : (i += 1) {
            let st = _.states.items[i];
            if (st.out1 >= (orig_start_idx as i32) && st.out1 < (orig_end_idx as i32)) {
                st.out1 += delta;
            }
            if (st.out2 >= (orig_start_idx as i32) && st.out2 < (orig_end_idx as i32)) {
                st.out2 += delta;
            }
            try _.states.append(_.alloc, st);
        }

        let new_outs: std.ArrayList(i32) = .empty;
        errdefer new_outs.deinit(_.alloc);
        for (o of orig.outs.items) {
            try new_outs.append(_.alloc, o + delta);
        }
        return Frag{ start: orig.start + delta, outs: new_outs };
    }

    fn parseAlternation(*_, depth: usize): RegexError!Frag {
        if (depth > 64) return RegexError.PatternTooComplex;
        let left = try _.parseConcat(depth);
        errdefer left.deinit(_.alloc);

        while (_.pos < _.pat.len && _.pat[_.pos] == '|') {
            _.has_alternation = true;
            _.pos += 1;
            let right = try _.parseConcat(depth);
            errdefer right.deinit(_.alloc);

            const split_id = try _.addState(NfaState{
                kind: StateKind.split,
                out1: left.start,
                out2: right.start,
            });
            let merged: std.ArrayList(i32) = .empty;
            errdefer merged.deinit(_.alloc);
            try merged.appendSlice(_.alloc, left.outs.items);
            try merged.appendSlice(_.alloc, right.outs.items);
            left.deinit(_.alloc);
            right.deinit(_.alloc);
            left = Frag{ start: split_id, outs: merged };
        }
        return left;
    }

    fn parseConcat(*_, depth: usize): RegexError!Frag {
        let has_prev = false;
        let prev = Frag{ start: -1, outs: .empty };
        errdefer {
            if (has_prev) prev.deinit(_.alloc);
        }

        while (_.pos < _.pat.len) {
            const ch = _.pat[_.pos];
            if (ch == '|' || ch == ')') break;

            // Handle inline flags (?i) or (?-i) that do not produce a consuming fragment
            if (ch == '(' && _.pos + 1 < _.pat.len && _.pat[_.pos + 1] == '?') {
                if (try _.tryConsumeInlineFlagOnly()) {
                    continue;
                }
            }

            let next_frag = try _.parseRepeat(depth);
            if (!has_prev) {
                prev = next_frag;
                has_prev = true;
            } else {
                _.patch(&prev.outs, next_frag.start);
                prev.deinit(_.alloc);
                prev = Frag{ start: prev.start, outs: next_frag.outs };
            }
        }

        if (!has_prev) {
            return _.emptyFrag();
        }
        return prev;
    }

    fn tryConsumeInlineFlagOnly(*_): RegexError!bool {
        // Pattern at _.pos is "(?..."
        const save_pos = _.pos;
        let p = _.pos + 2;
        if (p >= _.pat.len) return false;
        if (_.pat[p] == ':') return false; // non-capturing group (?:...)

        let neg = false;
        let saw_flag = false;
        let new_ci = _.case_insensitive;

        while (p < _.pat.len) : (p += 1) {
            const c = _.pat[p];
            if (c == ')') {
                if (!saw_flag) return RegexError.InvalidFlag;
                _.case_insensitive = new_ci;
                _.pos = p + 1;
                return true;
            }
            if (c == ':') {
                // Scoped flag group (?i:...) -> handled in parseAtom
                _.pos = save_pos;
                return false;
            }
            if (c == '-') {
                neg = true;
            } else if (c == 'i') {
                new_ci = !neg;
                saw_flag = true;
            } else {
                return RegexError.InvalidFlag;
            }
        }
        return RegexError.UnmatchedParen;
    }

    fn parseRepeat(*_, depth: usize): RegexError!Frag {
        const atom_start_idx = _.states.items.len;
        let frag = try _.parseAtom(depth);
        errdefer frag.deinit(_.alloc);
        const atom_end_idx = _.states.items.len;

        while (_.pos < _.pat.len) {
            const ch = _.pat[_.pos];
            if (ch == '*') {
                _.pos += 1;
                let lazy = false;
                if (_.pos < _.pat.len && _.pat[_.pos] == '?') {
                    lazy = true;
                    _.has_lazy = true;
                    _.pos += 1;
                }
                const s = if (lazy)
                    try _.addState(NfaState{ kind: StateKind.split, out1: -1, out2: frag.start })
                else
                    try _.addState(NfaState{ kind: StateKind.split, out1: frag.start, out2: -1 });
                _.patch(&frag.outs, s);
                frag.outs.items.len = 0;
                try frag.outs.append(_.alloc, s);
                frag.start = s;
            } else if (ch == '+') {
                _.pos += 1;
                let lazy = false;
                if (_.pos < _.pat.len && _.pat[_.pos] == '?') {
                    lazy = true;
                    _.has_lazy = true;
                    _.pos += 1;
                }
                const s = if (lazy)
                    try _.addState(NfaState{ kind: StateKind.split, out1: -1, out2: frag.start })
                else
                    try _.addState(NfaState{ kind: StateKind.split, out1: frag.start, out2: -1 });
                _.patch(&frag.outs, s);
                frag.outs.items.len = 0;
                try frag.outs.append(_.alloc, s);
            } else if (ch == '?') {
                _.pos += 1;
                let lazy = false;
                if (_.pos < _.pat.len && _.pat[_.pos] == '?') {
                    lazy = true;
                    _.has_lazy = true;
                    _.pos += 1;
                }
                const s = if (lazy)
                    try _.addState(NfaState{ kind: StateKind.split, out1: -1, out2: frag.start })
                else
                    try _.addState(NfaState{ kind: StateKind.split, out1: frag.start, out2: -1 });
                try frag.outs.append(_.alloc, s);
                frag.start = s;
            } else if (ch == '{') {
                frag = try _.applyBoundedQuantifier(atom_start_idx, atom_end_idx, &frag);
            } else {
                break;
            }
        }
        return frag;
    }

    fn applyBoundedQuantifier(
        *_,
        atom_start_idx: usize,
        atom_end_idx: usize,
        base_frag: *Frag,
    ): RegexError!Frag {
        // Parse {m}, {m,}, {m,n}
        _.pos += 1; // skip '{'
        if (_.pos >= _.pat.len || _.pat[_.pos] < '0' || _.pat[_.pos] > '9') {
            return RegexError.InvalidQuantifier;
        }
        let min_val: usize = 0;
        while (_.pos < _.pat.len && _.pat[_.pos] >= '0' && _.pat[_.pos] <= '9') {
            min_val = min_val * 10 + ((_.pat[_.pos] - '0') as usize);
            if (min_val > MAX_REPEAT_BOUND) return RegexError.PatternTooComplex;
            _.pos += 1;
        }

        let max_val: usize? = min_val;
        if (_.pos < _.pat.len && _.pat[_.pos] == ',') {
            _.pos += 1;
            if (_.pos < _.pat.len && _.pat[_.pos] == '}') {
                max_val = null; // unbounded {m,}
            } else if (_.pos < _.pat.len && _.pat[_.pos] >= '0' && _.pat[_.pos] <= '9') {
                let mv: usize = 0;
                while (_.pos < _.pat.len && _.pat[_.pos] >= '0' && _.pat[_.pos] <= '9') {
                    mv = mv * 10 + ((_.pat[_.pos] - '0') as usize);
                    if (mv > MAX_REPEAT_BOUND) return RegexError.PatternTooComplex;
                    _.pos += 1;
                }
                max_val = mv;
            } else {
                return RegexError.InvalidQuantifier;
            }
        }

        if (_.pos >= _.pat.len || _.pat[_.pos] != '}') {
            return RegexError.InvalidQuantifier;
        }
        _.pos += 1; // skip '}'

        let lazy = false;
        if (_.pos < _.pat.len && _.pat[_.pos] == '?') {
            lazy = true;
            _.has_lazy = true;
            _.pos += 1;
        }

        if (mv := max_val) {
            if (mv < min_val) return RegexError.InvalidRange;
        }

        // Case 1: {0} -> empty epsilon
        if (min_val == 0 && max_val != null && max_val.? == 0) {
            base_frag.deinit(_.alloc);
            return _.emptyFrag();
        }

        // Case 2: {0,} -> * or *?
        if (min_val == 0 && max_val == null) {
            const s = if (lazy)
                try _.addState(NfaState{ kind: StateKind.split, out1: -1, out2: base_frag.start })
            else
                try _.addState(NfaState{ kind: StateKind.split, out1: base_frag.start, out2: -1 });
            _.patch(&base_frag.outs, s);
            base_frag.outs.items.len = 0;
            try base_frag.outs.append(_.alloc, s);
            base_frag.start = s;
            return base_frag.*;
        }

        // Build result by concatenating required copies + optional/unbounded tail
        let has_res = false;
        let res = Frag{ start: -1, outs: .empty };
        errdefer {
            if (has_res) res.deinit(_.alloc);
        }

        // Required copies (min_val)
        let k: usize = 0;
        while (k < min_val) : (k += 1) {
            let copy = try _.cloneFrag(atom_start_idx, atom_end_idx, base_frag);
            if (!has_res) {
                res = copy;
                has_res = true;
            } else {
                _.patch(&res.outs, copy.start);
                res.deinit(_.alloc);
                res = Frag{ start: res.start, outs: copy.outs };
            }
        }

        if (max_val == null) {
            // {m,} with m >= 1 -> append one * / *? copy
            let tail = try _.cloneFrag(atom_start_idx, atom_end_idx, base_frag);
            errdefer tail.deinit(_.alloc);
            const s = if (lazy)
                try _.addState(NfaState{ kind: StateKind.split, out1: -1, out2: tail.start })
            else
                try _.addState(NfaState{ kind: StateKind.split, out1: tail.start, out2: -1 });
            _.patch(&tail.outs, s);
            tail.outs.items.len = 0;
            try tail.outs.append(_.alloc, s);
            tail.start = s;

            _.patch(&res.outs, tail.start);
            res.deinit(_.alloc);
            res = Frag{ start: res.start, outs: tail.outs };
        } else {
            const opt_count = max_val.? - min_val;
            let o: usize = 0;
            while (o < opt_count) : (o += 1) {
                let opt_copy = try _.cloneFrag(atom_start_idx, atom_end_idx, base_frag);
                errdefer opt_copy.deinit(_.alloc);
                const s = if (lazy)
                    try _.addState(NfaState{ kind: StateKind.split, out1: -1, out2: opt_copy.start })
                else
                    try _.addState(NfaState{ kind: StateKind.split, out1: opt_copy.start, out2: -1 });
                try opt_copy.outs.append(_.alloc, s);
                opt_copy.start = s;

                if (!has_res) {
                    res = opt_copy;
                    has_res = true;
                } else {
                    _.patch(&res.outs, opt_copy.start);
                    res.deinit(_.alloc);
                    res = Frag{ start: res.start, outs: opt_copy.outs };
                }
            }
        }

        base_frag.deinit(_.alloc);
        return res;
    }

    fn parseAtom(*_, depth: usize): RegexError!Frag {
        const ch = _.pat[_.pos];
        if (ch == '*' || ch == '+' || ch == '?' || ch == '{') {
            return RegexError.NothingToRepeat;
        }
        if (ch == '(') {
            _.pos += 1;
            let capturing = true;
            let group_id: usize = 0;
            const prev_ci = _.case_insensitive;

            if (_.pos < _.pat.len && _.pat[_.pos] == '?') {
                capturing = false;
                _.pos += 1;
                if (_.pos >= _.pat.len) return RegexError.UnmatchedParen;
                if (_.pat[_.pos] == ':') {
                    _.pos += 1;
                } else {
                    // Scoped flag group (?i:...) or (?-i:...)
                    let neg = false;
                    let saw_flag = false;
                    while (_.pos < _.pat.len && _.pat[_.pos] != ':') : (_.pos += 1) {
                        const fc = _.pat[_.pos];
                        if (fc == '-') {
                            neg = true;
                        } else if (fc == 'i') {
                            _.case_insensitive = !neg;
                            saw_flag = true;
                        } else {
                            return RegexError.InvalidFlag;
                        }
                    }
                    if (!saw_flag || _.pos >= _.pat.len || _.pat[_.pos] != ':') {
                        return RegexError.InvalidFlag;
                    }
                    _.pos += 1; // skip ':'
                }
            } else {
                if (_.num_captures < MAX_CAPTURES) {
                    group_id = _.num_captures;
                    _.num_captures += 1;
                } else {
                    capturing = false;
                }
            }

            let inner = try _.parseAlternation(depth + 1);
            _.case_insensitive = prev_ci;

            if (_.pos >= _.pat.len || _.pat[_.pos] != ')') {
                inner.deinit(_.alloc);
                return RegexError.UnmatchedParen;
            }
            _.pos += 1;

            if (!capturing) {
                return inner;
            }

            // Wrap capturing group with save(2*g) and save(2*g + 1)
            const open_id = try _.addState(NfaState{
                kind: StateKind.save,
                save_slot: (group_id * 2) as u16,
                out1: inner.start,
            });
            const close_id = try _.addState(NfaState{
                kind: StateKind.save,
                save_slot: (group_id * 2 + 1) as u16,
                out1: -1,
            });
            _.patch(&inner.outs, close_id);
            inner.outs.items.len = 0;
            try inner.outs.append(_.alloc, close_id);
            inner.start = open_id;
            return inner;
        }
        if (ch == '[') {
            return _.parseCharClass();
        }
        if (ch == '.') {
            _.pos += 1;
            return _.singleState(NfaState{ kind: StateKind.any_char });
        }
        if (ch == '^') {
            _.pos += 1;
            _.has_start_anchor = true;
            return _.singleState(NfaState{ kind: StateKind.anchor_start });
        }
        if (ch == '$') {
            _.pos += 1;
            _.has_end_anchor = true;
            return _.singleState(NfaState{ kind: StateKind.anchor_end });
        }
        if (ch == '\\') {
            _.pos += 1;
            if (_.pos >= _.pat.len) return RegexError.TrailingEscape;
            const esc = _.pat[_.pos];
            _.pos += 1;
            return _.buildEscape(esc);
        }
        _.pos += 1;
        return _.buildLiteral(ch);
    }

    fn buildLiteral(*_, ch: u8): RegexError!Frag {
        if (_.case_insensitive && ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z'))) {
            let cc = CharClass{};
            cc.setBitWithCase(ch, true);
            return _.singleState(NfaState{ kind: StateKind.char_class, cclass: cc });
        }
        return _.singleState(NfaState{ kind: StateKind.literal, ch: ch });
    }

    fn buildEscape(*_, esc: u8): RegexError!Frag {
        let cc = CharClass{};
        switch (esc) {
            'd' => {
                cc.setRange('0', '9', false);
                return _.singleState(NfaState{ kind: StateKind.char_class, cclass: cc });
            },
            'D' => {
                cc.negated = true;
                cc.setRange('0', '9', false);
                return _.singleState(NfaState{ kind: StateKind.char_class, cclass: cc });
            },
            'w' => {
                cc.setRange('a', 'z', false);
                cc.setRange('A', 'Z', false);
                cc.setRange('0', '9', false);
                cc.setBit('_');
                return _.singleState(NfaState{ kind: StateKind.char_class, cclass: cc });
            },
            'W' => {
                cc.negated = true;
                cc.setRange('a', 'z', false);
                cc.setRange('A', 'Z', false);
                cc.setRange('0', '9', false);
                cc.setBit('_');
                return _.singleState(NfaState{ kind: StateKind.char_class, cclass: cc });
            },
            's' => {
                cc.setBit(' ');
                cc.setBit('\t');
                cc.setBit('\n');
                cc.setBit('\r');
                cc.setBit(0x0B);
                cc.setBit(0x0C);
                return _.singleState(NfaState{ kind: StateKind.char_class, cclass: cc });
            },
            'S' => {
                cc.negated = true;
                cc.setBit(' ');
                cc.setBit('\t');
                cc.setBit('\n');
                cc.setBit('\r');
                cc.setBit(0x0B);
                cc.setBit(0x0C);
                return _.singleState(NfaState{ kind: StateKind.char_class, cclass: cc });
            },
            'b' => {
                _.has_word_boundary = true;
                return _.singleState(NfaState{ kind: StateKind.word_boundary });
            },
            'B' => {
                _.has_word_boundary = true;
                return _.singleState(NfaState{ kind: StateKind.non_word_boundary });
            },
            'n' => return _.singleState(NfaState{ kind: StateKind.literal, ch: '\n' }),
            'r' => return _.singleState(NfaState{ kind: StateKind.literal, ch: '\r' }),
            't' => return _.singleState(NfaState{ kind: StateKind.literal, ch: '\t' }),
            '0' => return _.singleState(NfaState{ kind: StateKind.literal, ch: 0 }),
            else => return _.buildLiteral(esc),
        }
    }

    fn parseCharClass(*_): RegexError!Frag {
        _.pos += 1; // skip '['
        let cc = CharClass{};
        if (_.pos < _.pat.len && _.pat[_.pos] == '^') {
            cc.negated = true;
            _.pos += 1;
        }
        let first = true;
        while (_.pos < _.pat.len) {
            const c = _.pat[_.pos];
            if (c == ']' && !first) {
                _.pos += 1;
                return _.singleState(NfaState{ kind: StateKind.char_class, cclass: cc });
            }
            first = false;

            // Handle shorthand classes inside [...]
            if (c == '\\') {
                _.pos += 1;
                if (_.pos >= _.pat.len) return RegexError.UnclosedCharClass;
                const esc = _.pat[_.pos];
                _.pos += 1;
                if (esc == 'd') { cc.setRange('0', '9', false); continue; }
                if (esc == 'w') {
                    cc.setRange('a', 'z', false);
                    cc.setRange('A', 'Z', false);
                    cc.setRange('0', '9', false);
                    cc.setBit('_');
                    continue;
                }
                if (esc == 's') {
                    cc.setBit(' ');
                    cc.setBit('\t');
                    cc.setBit('\n');
                    cc.setBit('\r');
                    continue;
                }
                const lo: u8 = switch (esc) {
                    'n' => '\n',
                    'r' => '\r',
                    't' => '\t',
                    else => esc,
                };
                if (_.pos + 1 < _.pat.len && _.pat[_.pos] == '-' && _.pat[_.pos + 1] != ']') {
                    _.pos += 1;
                    const hi = try _.readClassChar();
                    if (lo > hi) return RegexError.InvalidRange;
                    cc.setRange(lo, hi, _.case_insensitive);
                } else {
                    cc.setBitWithCase(lo, _.case_insensitive);
                }
                continue;
            }

            const lo = c;
            _.pos += 1;
            if (_.pos + 1 < _.pat.len && _.pat[_.pos] == '-' && _.pat[_.pos + 1] != ']') {
                _.pos += 1;
                const hi = try _.readClassChar();
                if (lo > hi) return RegexError.InvalidRange;
                cc.setRange(lo, hi, _.case_insensitive);
            } else {
                cc.setBitWithCase(lo, _.case_insensitive);
            }
        }
        return RegexError.UnclosedCharClass;
    }

    fn readClassChar(*_): RegexError!u8 {
        if (_.pos >= _.pat.len) return RegexError.UnclosedCharClass;
        let ch = _.pat[_.pos];
        _.pos += 1;
        if (ch == '\\') {
            if (_.pos >= _.pat.len) return RegexError.UnclosedCharClass;
            const esc = _.pat[_.pos];
            _.pos += 1;
            ch = switch (esc) {
                'n' => '\n',
                'r' => '\r',
                't' => '\t',
                else => esc,
            };
        }
        return ch;
    }
}
