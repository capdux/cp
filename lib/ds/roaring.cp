// Source: cp/lib/roaring/roaring.cp (Pure CP Roaring Bitmap Compressed 32-bit Integer Set Engine)
// Zero-C-dependency compressed 32-bit integer set using adaptive ArrayContainer (<= 4096)
// and BitsetContainer (> 4096) partitioned by high 16-bit keys.

const std = @import("std");

/// Error set for RoaringBitmap operations.
pub err RoaringError {
    InvalidSerializedData,
    InvalidRange,
    OutOfMemory,
}

const ARRAY_MAX_CARD: u32 = 4096;
const BITSET_WORDS: usize = 1024;
const BITSET_BITS: u32 = 65536;
const SERIAL_MAGIC_0: u8 = 'R';
const SERIAL_MAGIC_1: u8 = 'O';
const SERIAL_MAGIC_2: u8 = 'A';
const SERIAL_MAGIC_3: u8 = 'R';
const SERIAL_VERSION: u16 = 1;

const SearchPos = struct {
    found: bool,
    idx: usize,
};

const ContainerKind = enum(u8) {
    Array = 0,
    Bitset = 1,
};

fn writeU16LE(buf: []u8, offset: usize, v: u16) {
    buf[offset] = @truncate(v & 0xFF);
    buf[offset + 1] = @truncate((v >> 8) & 0xFF);
}

fn writeU32LE(buf: []u8, offset: usize, v: u32) {
    buf[offset] = @truncate(v & 0xFF);
    buf[offset + 1] = @truncate((v >> 8) & 0xFF);
    buf[offset + 2] = @truncate((v >> 16) & 0xFF);
    buf[offset + 3] = @truncate((v >> 24) & 0xFF);
}

fn writeU64LE(buf: []u8, offset: usize, v: u64) {
    let i: usize = 0;
    while (i < 8) : (i += 1) {
        buf[offset + i] = @truncate((v >> @intCast(i * 8)) & 0xFF);
    }
}

fn readU16LE(buf: []const u8, offset: usize): u16 {
    return @as(u16, buf[offset]) | (@as(u16, buf[offset + 1]) << 8);
}

fn readU32LE(buf: []const u8, offset: usize): u32 {
    return @as(u32, buf[offset]) |
        (@as(u32, buf[offset + 1]) << 8) |
        (@as(u32, buf[offset + 2]) << 16) |
        (@as(u32, buf[offset + 3]) << 24);
}

fn readU64LE(buf: []const u8, offset: usize): u64 {
    let v: u64 = 0;
    let i: usize = 0;
    while (i < 8) : (i += 1) {
        v |= @as(u64, buf[offset + i]) << @intCast(i * 8);
    }
    return v;
}

const Container = struct {
    key: u16,
    kind: ContainerKind,
    card: u32,
    items: []u16 = [],
    cap: usize = 0,
    words: []u64 = [],

    fn initArrayWithCap(alloc: std.mem.Allocator, key: u16, initial_cap: usize): Container ! RoaringError {
        const cap_val = if (initial_cap < 4) 4 else initial_cap;
        let buf = alloc.alloc(u16, cap_val) catch return RoaringError.OutOfMemory;
        return Container{
            key: key,
            kind: ContainerKind.Array,
            card: 0,
            items: buf,
            cap: cap_val,
            words: [],
        };
    }

    fn initBitsetEmpty(alloc: std.mem.Allocator, key: u16): Container ! RoaringError {
        let words = alloc.alloc(u64, BITSET_WORDS) catch return RoaringError.OutOfMemory;
        @memset(words, 0);
        return Container{
            key: key,
            kind: ContainerKind.Bitset,
            card: 0,
            items: [],
            cap: 0,
            words: words,
        };
    }

    fn deinit(*_, alloc: std.mem.Allocator) {
        if (_.kind == ContainerKind.Array) {
            if (_.items.len > 0) {
                alloc.free(_.items);
                _.items = [];
            }
            _.cap = 0;
        } else {
            if (_.words.len > 0) {
                alloc.free(_.words);
                _.words = [];
            }
        }
        _.card = 0;
    }

    fn clone(alloc: std.mem.Allocator): Container ! RoaringError {
        if (_.kind == ContainerKind.Array) {
            const len = @as(usize, _.card);
            if (len == 0) {
                return Container.initArrayWithCap(alloc, _.key, 4);
            }
            let buf = alloc.alloc(u16, len) catch return RoaringError.OutOfMemory;
            @memcpy(buf[0..len], _.items[0..len]);
            return Container{
                key: _.key,
                kind: ContainerKind.Array,
                card: _.card,
                items: buf,
                cap: len,
                words: [],
            };
        } else {
            let words = alloc.alloc(u64, BITSET_WORDS) catch return RoaringError.OutOfMemory;
            @memcpy(words, _.words);
            return Container{
                key: _.key,
                kind: ContainerKind.Bitset,
                card: _.card,
                items: [],
                cap: 0,
                words: words,
            };
        }
    }

    fn arrayBinarySearch(low: u16): SearchPos {
        let lo: usize = 0;
        let hi: usize = @as(usize, _.card);
        while (lo < hi) {
            const mid = lo + (hi - lo) / 2;
            const v = _.items[mid];
            if (v == low) {
                return SearchPos{ found: true, idx: mid };
            } else if (v < low) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }
        return SearchPos{ found: false, idx: lo };
    }

    fn contains(low: u16): bool {
        if (_.kind == ContainerKind.Array) {
            return _.arrayBinarySearch(low).found;
        } else {
            const w_idx = @as(usize, low >> 6);
            const b_off = @as(u6, @truncate(low & 63));
            return (_.words[w_idx] & (@as(u64, 1) << b_off)) != 0;
        }
    }

    fn upgradeToBitset(*_, alloc: std.mem.Allocator): void ! RoaringError {
        let words = alloc.alloc(u64, BITSET_WORDS) catch return RoaringError.OutOfMemory;
        @memset(words, 0);
        let i: usize = 0;
        const n = @as(usize, _.card);
        while (i < n) : (i += 1) {
            const v = _.items[i];
            const w_idx = @as(usize, v >> 6);
            const b_off = @as(u6, @truncate(v & 63));
            words[w_idx] |= (@as(u64, 1) << b_off);
        }
        if (_.items.len > 0) {
            alloc.free(_.items);
            _.items = [];
        }
        _.cap = 0;
        _.words = words;
        _.kind = ContainerKind.Bitset;
    }

    fn downgradeToArray(*_, alloc: std.mem.Allocator): void ! RoaringError {
        const n = @as(usize, _.card);
        const cap_val = if (n < 4) 4 else n;
        let buf = alloc.alloc(u16, cap_val) catch return RoaringError.OutOfMemory;
        let out: usize = 0;
        let wi: usize = 0;
        while (wi < BITSET_WORDS) : (wi += 1) {
            let w = _.words[wi];
            const base: u16 = @intCast(wi * 64);
            while (w != 0) {
                const bit: u16 = @intCast(@ctz(w));
                buf[out] = base + bit;
                out += 1;
                w &= w - 1;
            }
        }
        if (_.words.len > 0) {
            alloc.free(_.words);
            _.words = [];
        }
        _.items = buf;
        _.cap = cap_val;
        _.kind = ContainerKind.Array;
    }

    fn ensureArrayCap(*_, alloc: std.mem.Allocator, needed: usize): void ! RoaringError {
        if (needed <= _.cap) return;
        let new_cap = if (_.cap == 0) 8 else _.cap * 2;
        if (new_cap < needed) new_cap = needed;
        if (new_cap > @as(usize, ARRAY_MAX_CARD)) new_cap = @as(usize, ARRAY_MAX_CARD);
        let new_buf = alloc.alloc(u16, new_cap) catch return RoaringError.OutOfMemory;
        if (_.card > 0) {
            @memcpy(new_buf[0..@as(usize, _.card)], _.items[0..@as(usize, _.card)]);
        }
        if (_.items.len > 0) {
            alloc.free(_.items);
        }
        _.items = new_buf;
        _.cap = new_cap;
    }

    fn add(*_, alloc: std.mem.Allocator, low: u16): bool ! RoaringError {
        if (_.kind == ContainerKind.Array) {
            const res = _.arrayBinarySearch(low);
            if (res.found) return false;
            if (_.card >= ARRAY_MAX_CARD) {
                _.upgradeToBitset(alloc)?;
                const w_idx = @as(usize, low >> 6);
                const b_off = @as(u6, @truncate(low & 63));
                _.words[w_idx] |= (@as(u64, 1) << b_off);
                _.card += 1;
                return true;
            }
            const n = @as(usize, _.card);
            _.ensureArrayCap(alloc, n + 1)?;
            let i = n;
            while (i > res.idx) {
                _.items[i] = _.items[i - 1];
                i -= 1;
            }
            _.items[res.idx] = low;
            _.card += 1;
            return true;
        } else {
            const w_idx = @as(usize, low >> 6);
            const b_off = @as(u6, @truncate(low & 63));
            const mask = @as(u64, 1) << b_off;
            if ((_.words[w_idx] & mask) != 0) return false;
            _.words[w_idx] |= mask;
            _.card += 1;
            return true;
        }
    }

    fn removeVal(*_, alloc: std.mem.Allocator, low: u16): bool {
        if (_.kind == ContainerKind.Array) {
            const res = _.arrayBinarySearch(low);
            if (!res.found) return false;
            let i = res.idx;
            const last = @as(usize, _.card) - 1;
            while (i < last) : (i += 1) {
                _.items[i] = _.items[i + 1];
            }
            _.card -= 1;
            return true;
        } else {
            const w_idx = @as(usize, low >> 6);
            const b_off = @as(u6, @truncate(low & 63));
            const mask = @as(u64, 1) << b_off;
            if ((_.words[w_idx] & mask) == 0) return false;
            _.words[w_idx] &= ~mask;
            _.card -= 1;
            if (_.card <= ARRAY_MAX_CARD) {
                _.downgradeToArray(alloc) catch {};
            }
            return true;
        }
    }

    fn addRangeInclusive(*_, alloc: std.mem.Allocator, low_start: u16, low_end: u16): void ! RoaringError {
        if (low_start > low_end) return;
        const range_len: u32 = @as(u32, low_end) - @as(u32, low_start) + 1;
        if (_.kind == ContainerKind.Array) {
            if (_.card + range_len > ARRAY_MAX_CARD) {
                _.upgradeToBitset(alloc)?;
                _.bitsetSetRange(low_start, low_end);
                return;
            }
            let v: u32 = @as(u32, low_start);
            const end_u32: u32 = @as(u32, low_end);
            while (v <= end_u32) : (v += 1) {
                * = _.add(alloc, @intCast(v))?;
            }
        } else {
            _.bitsetSetRange(low_start, low_end);
        }
    }

    fn bitsetSetRange(*_, low_start: u16, low_end: u16) {
        const start_w: usize = @as(usize, low_start >> 6);
        const end_w: usize = @as(usize, low_end >> 6);
        const start_b: u6 = @truncate(low_start & 63);
        const end_b: u6 = @truncate(low_end & 63);

        if (start_w == end_w) {
            const hi_mask: u64 = (~@as(u64, 0)) >> @intCast(63 - @as(u32, end_b));
            const lo_mask: u64 = (~@as(u64, 0)) << start_b;
            const mask = hi_mask & lo_mask;
            const old_bits = @popCount(_.words[start_w]);
            _.words[start_w] |= mask;
            const new_bits = @popCount(_.words[start_w]);
            _.card += @as(u32, new_bits - old_bits);
            return;
        }

        const first_mask: u64 = (~@as(u64, 0)) << start_b;
        const old_first = @popCount(_.words[start_w]);
        _.words[start_w] |= first_mask;
        _.card += @as(u32, @popCount(_.words[start_w]) - old_first);

        let w = start_w + 1;
        while (w < end_w) : (w += 1) {
            const old_mid = @popCount(_.words[w]);
            _.words[w] = ~@as(u64, 0);
            _.card += @as(u32, 64 - old_mid);
        }

        const last_mask: u64 = (~@as(u64, 0)) >> @intCast(63 - @as(u32, end_b));
        const old_last = @popCount(_.words[end_w]);
        _.words[end_w] |= last_mask;
        _.card += @as(u32, @popCount(_.words[end_w]) - old_last);
    }

    fn minimumLow(): u16 {
        if (_.kind == ContainerKind.Array) {
            return _.items[0];
        } else {
            let wi: usize = 0;
            while (wi < BITSET_WORDS) : (wi += 1) {
                const w = _.words[wi];
                if (w != 0) {
                    return @intCast(wi * 64 + @as(usize, @ctz(w)));
                }
            }
            return 0;
        }
    }

    fn maximumLow(): u16 {
        if (_.kind == ContainerKind.Array) {
            return _.items[@as(usize, _.card) - 1];
        } else {
            let wi: usize = BITSET_WORDS;
            while (wi > 0) {
                wi -= 1;
                const w = _.words[wi];
                if (w != 0) {
                    const bit: usize = 63 - @as(usize, @clz(w));
                    return @intCast(wi * 64 + bit);
                }
            }
            return 0;
        }
    }

    fn rankLow(low: u16): u64 {
        if (_.kind == ContainerKind.Array) {
            let lo: usize = 0;
            let hi: usize = @as(usize, _.card);
            while (lo < hi) {
                const mid = lo + (hi - lo) / 2;
                if (_.items[mid] <= low) {
                    lo = mid + 1;
                } else {
                    hi = mid;
                }
            }
            return @as(u64, lo);
        } else {
            const w_idx = @as(usize, low >> 6);
            const b_off = @as(u32, low & 63);
            let total: u64 = 0;
            let wi: usize = 0;
            while (wi < w_idx) : (wi += 1) {
                total += @as(u64, @popCount(_.words[wi]));
            }
            const mask: u64 = (~@as(u64, 0)) >> @intCast(63 - b_off);
            total += @as(u64, @popCount(_.words[w_idx] & mask));
            return total;
        }
    }

    fn appendToSlice(out: []u32, start_pos: usize): usize {
        let pos = start_pos;
        const high_prefix: u32 = @as(u32, _.key) << 16;
        if (_.kind == ContainerKind.Array) {
            let i: usize = 0;
            const n = @as(usize, _.card);
            while (i < n) : (i += 1) {
                out[pos] = high_prefix | @as(u32, _.items[i]);
                pos += 1;
            }
        } else {
            let wi: usize = 0;
            while (wi < BITSET_WORDS) : (wi += 1) {
                let w = _.words[wi];
                const base: u32 = high_prefix | @as(u32, wi * 64);
                while (w != 0) {
                    const bit: u32 = @intCast(@ctz(w));
                    out[pos] = base | bit;
                    pos += 1;
                    w &= w - 1;
                }
            }
        }
        return pos;
    }

    fn finalizeBitset(alloc: std.mem.Allocator, key: u16, words: []u64, card: u32): Container? ! RoaringError {
        if (card == 0) {
            alloc.free(words);
            return null;
        }
        let c = Container{
            key: key,
            kind: ContainerKind.Bitset,
            card: card,
            items: [],
            cap: 0,
            words: words,
        };
        if (card <= ARRAY_MAX_CARD) {
            c.downgradeToArray(alloc) catch |e| {
                alloc.free(words);
                return e;
            };
        }
        return c;
    }
};

fn containerAnd(alloc: std.mem.Allocator, a: Container, b: Container): Container? ! RoaringError {
    if (a.kind == ContainerKind.Array && b.kind == ContainerKind.Array) {
        const max_n = if (a.card < b.card) @as(usize, a.card) else @as(usize, b.card);
        if (max_n == 0) return null;
        let buf = alloc.alloc(u16, max_n) catch return RoaringError.OutOfMemory;
        let ia: usize = 0;
        let ib: usize = 0;
        let out: usize = 0;
        const na = @as(usize, a.card);
        const nb = @as(usize, b.card);
        while (ia < na && ib < nb) {
            const va = a.items[ia];
            const vb = b.items[ib];
            if (va == vb) {
                buf[out] = va;
                out += 1;
                ia += 1;
                ib += 1;
            } else if (va < vb) {
                ia += 1;
            } else {
                ib += 1;
            }
        }
        if (out == 0) {
            alloc.free(buf);
            return null;
        }
        return Container{
            key: a.key,
            kind: ContainerKind.Array,
            card: @intCast(out),
            items: buf,
            cap: max_n,
            words: [],
        };
    }

    if (a.kind == ContainerKind.Bitset && b.kind == ContainerKind.Bitset) {
        let words = alloc.alloc(u64, BITSET_WORDS) catch return RoaringError.OutOfMemory;
        let card: u32 = 0;
        let wi: usize = 0;
        while (wi < BITSET_WORDS) : (wi += 1) {
            const w = a.words[wi] & b.words[wi];
            words[wi] = w;
            card += @as(u32, @popCount(w));
        }
        return Container.finalizeBitset(alloc, a.key, words, card);
    }

    const arr = if (a.kind == ContainerKind.Array) a else b;
    const bs = if (a.kind == ContainerKind.Bitset) a else b;
    const na = @as(usize, arr.card);
    if (na == 0) return null;
    let buf = alloc.alloc(u16, na) catch return RoaringError.OutOfMemory;
    let i: usize = 0;
    let out: usize = 0;
    while (i < na) : (i += 1) {
        const v = arr.items[i];
        if (bs.contains(v)) {
            buf[out] = v;
            out += 1;
        }
    }
    if (out == 0) {
        alloc.free(buf);
        return null;
    }
    return Container{
        key: a.key,
        kind: ContainerKind.Array,
        card: @intCast(out),
        items: buf,
        cap: na,
        words: [],
    };
}

fn containerOr(alloc: std.mem.Allocator, a: Container, b: Container): Container ! RoaringError {
    if (a.kind == ContainerKind.Bitset && b.kind == ContainerKind.Bitset) {
        let words = alloc.alloc(u64, BITSET_WORDS) catch return RoaringError.OutOfMemory;
        let card: u32 = 0;
        let wi: usize = 0;
        while (wi < BITSET_WORDS) : (wi += 1) {
            const w = a.words[wi] | b.words[wi];
            words[wi] = w;
            card += @as(u32, @popCount(w));
        }
        return Container{
            key: a.key,
            kind: ContainerKind.Bitset,
            card: card,
            items: [],
            cap: 0,
            words: words,
        };
    }

    if (a.kind == ContainerKind.Array && b.kind == ContainerKind.Array) {
        const total_max = @as(usize, a.card) + @as(usize, b.card);
        if (total_max <= @as(usize, ARRAY_MAX_CARD)) {
            let buf = alloc.alloc(u16, if (total_max < 4) 4 else total_max) catch return RoaringError.OutOfMemory;
            let ia: usize = 0;
            let ib: usize = 0;
            let out: usize = 0;
            const na = @as(usize, a.card);
            const nb = @as(usize, b.card);
            while (ia < na && ib < nb) {
                const va = a.items[ia];
                const vb = b.items[ib];
                if (va == vb) {
                    buf[out] = va;
                    out += 1;
                    ia += 1;
                    ib += 1;
                } else if (va < vb) {
                    buf[out] = va;
                    out += 1;
                    ia += 1;
                } else {
                    buf[out] = vb;
                    out += 1;
                    ib += 1;
                }
            }
            while (ia < na) : (ia += 1) {
                buf[out] = a.items[ia];
                out += 1;
            }
            while (ib < nb) : (ib += 1) {
                buf[out] = b.items[ib];
                out += 1;
            }
            return Container{
                key: a.key,
                kind: ContainerKind.Array,
                card: @intCast(out),
                items: buf,
                cap: if (total_max < 4) 4 else total_max,
                words: [],
            };
        }
        let words = alloc.alloc(u64, BITSET_WORDS) catch return RoaringError.OutOfMemory;
        @memset(words, 0);
        let card: u32 = 0;
        let i: usize = 0;
        while (i < @as(usize, a.card)) : (i += 1) {
            const v = a.items[i];
            words[@as(usize, v >> 6)] |= (@as(u64, 1) << @as(u6, @truncate(v & 63)));
            card += 1;
        }
        let j: usize = 0;
        while (j < @as(usize, b.card)) : (j += 1) {
            const v = b.items[j];
            const w_idx = @as(usize, v >> 6);
            const mask = @as(u64, 1) << @as(u6, @truncate(v & 63));
            if ((words[w_idx] & mask) == 0) {
                words[w_idx] |= mask;
                card += 1;
            }
        }
        let c = Container{
            key: a.key,
            kind: ContainerKind.Bitset,
            card: card,
            items: [],
            cap: 0,
            words: words,
        };
        if (card <= ARRAY_MAX_CARD) {
            c.downgradeToArray(alloc) catch |e| {
                alloc.free(words);
                return e;
            };
        }
        return c;
    }

    const arr = if (a.kind == ContainerKind.Array) a else b;
    const bs = if (a.kind == ContainerKind.Bitset) a else b;
    let out_c = bs.clone(alloc)?;
    let i: usize = 0;
    const na = @as(usize, arr.card);
    while (i < na) : (i += 1) {
        const v = arr.items[i];
        const w_idx = @as(usize, v >> 6);
        const mask = @as(u64, 1) << @as(u6, @truncate(v & 63));
        if ((out_c.words[w_idx] & mask) == 0) {
            out_c.words[w_idx] |= mask;
            out_c.card += 1;
        }
    }
    return out_c;
}

fn containerAndNot(alloc: std.mem.Allocator, a: Container, b: Container): Container? ! RoaringError {
    if (a.kind == ContainerKind.Array && b.kind == ContainerKind.Array) {
        const na = @as(usize, a.card);
        if (na == 0) return null;
        let buf = alloc.alloc(u16, na) catch return RoaringError.OutOfMemory;
        let ia: usize = 0;
        let ib: usize = 0;
        let out: usize = 0;
        const nb = @as(usize, b.card);
        while (ia < na && ib < nb) {
            const va = a.items[ia];
            const vb = b.items[ib];
            if (va == vb) {
                ia += 1;
                ib += 1;
            } else if (va < vb) {
                buf[out] = va;
                out += 1;
                ia += 1;
            } else {
                ib += 1;
            }
        }
        while (ia < na) : (ia += 1) {
            buf[out] = a.items[ia];
            out += 1;
        }
        if (out == 0) {
            alloc.free(buf);
            return null;
        }
        return Container{
            key: a.key,
            kind: ContainerKind.Array,
            card: @intCast(out),
            items: buf,
            cap: na,
            words: [],
        };
    }

    if (a.kind == ContainerKind.Bitset && b.kind == ContainerKind.Bitset) {
        let words = alloc.alloc(u64, BITSET_WORDS) catch return RoaringError.OutOfMemory;
        let card: u32 = 0;
        let wi: usize = 0;
        while (wi < BITSET_WORDS) : (wi += 1) {
            const w = a.words[wi] & ~b.words[wi];
            words[wi] = w;
            card += @as(u32, @popCount(w));
        }
        return Container.finalizeBitset(alloc, a.key, words, card);
    }

    if (a.kind == ContainerKind.Array && b.kind == ContainerKind.Bitset) {
        const na = @as(usize, a.card);
        if (na == 0) return null;
        let buf = alloc.alloc(u16, na) catch return RoaringError.OutOfMemory;
        let i: usize = 0;
        let out: usize = 0;
        while (i < na) : (i += 1) {
            const v = a.items[i];
            if (!b.contains(v)) {
                buf[out] = v;
                out += 1;
            }
        }
        if (out == 0) {
            alloc.free(buf);
            return null;
        }
        return Container{
            key: a.key,
            kind: ContainerKind.Array,
            card: @intCast(out),
            items: buf,
            cap: na,
            words: [],
        };
    }

    // a is Bitset, b is Array
    let words = alloc.alloc(u64, BITSET_WORDS) catch return RoaringError.OutOfMemory;
    @memcpy(words, a.words);
    let card = a.card;
    let i: usize = 0;
    const nb = @as(usize, b.card);
    while (i < nb) : (i += 1) {
        const v = b.items[i];
        const w_idx = @as(usize, v >> 6);
        const mask = @as(u64, 1) << @as(u6, @truncate(v & 63));
        if ((words[w_idx] & mask) != 0) {
            words[w_idx] &= ~mask;
            card -= 1;
        }
    }
    return Container.finalizeBitset(alloc, a.key, words, card);
}

fn containerXor(alloc: std.mem.Allocator, a: Container, b: Container): Container? ! RoaringError {
    if (a.kind == ContainerKind.Bitset && b.kind == ContainerKind.Bitset) {
        let words = alloc.alloc(u64, BITSET_WORDS) catch return RoaringError.OutOfMemory;
        let card: u32 = 0;
        let wi: usize = 0;
        while (wi < BITSET_WORDS) : (wi += 1) {
            const w = a.words[wi] ^ b.words[wi];
            words[wi] = w;
            card += @as(u32, @popCount(w));
        }
        return Container.finalizeBitset(alloc, a.key, words, card);
    }

    if (a.kind == ContainerKind.Array && b.kind == ContainerKind.Array) {
        const total_max = @as(usize, a.card) + @as(usize, b.card);
        if (total_max == 0) return null;
        if (total_max <= @as(usize, ARRAY_MAX_CARD)) {
            let buf = alloc.alloc(u16, total_max) catch return RoaringError.OutOfMemory;
            let ia: usize = 0;
            let ib: usize = 0;
            let out: usize = 0;
            const na = @as(usize, a.card);
            const nb = @as(usize, b.card);
            while (ia < na && ib < nb) {
                const va = a.items[ia];
                const vb = b.items[ib];
                if (va == vb) {
                    ia += 1;
                    ib += 1;
                } else if (va < vb) {
                    buf[out] = va;
                    out += 1;
                    ia += 1;
                } else {
                    buf[out] = vb;
                    out += 1;
                    ib += 1;
                }
            }
            while (ia < na) : (ia += 1) {
                buf[out] = a.items[ia];
                out += 1;
            }
            while (ib < nb) : (ib += 1) {
                buf[out] = b.items[ib];
                out += 1;
            }
            if (out == 0) {
                alloc.free(buf);
                return null;
            }
            return Container{
                key: a.key,
                kind: ContainerKind.Array,
                card: @intCast(out),
                items: buf,
                cap: total_max,
                words: [],
            };
        }
        let words = alloc.alloc(u64, BITSET_WORDS) catch return RoaringError.OutOfMemory;
        @memset(words, 0);
        let card: u32 = 0;
        let i: usize = 0;
        while (i < @as(usize, a.card)) : (i += 1) {
            const v = a.items[i];
            words[@as(usize, v >> 6)] |= (@as(u64, 1) << @as(u6, @truncate(v & 63)));
            card += 1;
        }
        let j: usize = 0;
        while (j < @as(usize, b.card)) : (j += 1) {
            const v = b.items[j];
            const w_idx = @as(usize, v >> 6);
            const mask = @as(u64, 1) << @as(u6, @truncate(v & 63));
            if ((words[w_idx] & mask) != 0) {
                words[w_idx] &= ~mask;
                card -= 1;
            } else {
                words[w_idx] |= mask;
                card += 1;
            }
        }
        return Container.finalizeBitset(alloc, a.key, words, card);
    }

    const arr = if (a.kind == ContainerKind.Array) a else b;
    const bs = if (a.kind == ContainerKind.Bitset) a else b;
    let words = alloc.alloc(u64, BITSET_WORDS) catch return RoaringError.OutOfMemory;
    @memcpy(words, bs.words);
    let card = bs.card;
    let i: usize = 0;
    const na = @as(usize, arr.card);
    while (i < na) : (i += 1) {
        const v = arr.items[i];
        const w_idx = @as(usize, v >> 6);
        const mask = @as(u64, 1) << @as(u6, @truncate(v & 63));
        if ((words[w_idx] & mask) != 0) {
            words[w_idx] &= ~mask;
            card -= 1;
        } else {
            words[w_idx] |= mask;
            card += 1;
        }
    }
    return Container.finalizeBitset(alloc, a.key, words, card);
}

fn containerAndCardinality(a: Container, b: Container): u64 {
    if (a.kind == ContainerKind.Array && b.kind == ContainerKind.Array) {
        let ia: usize = 0;
        let ib: usize = 0;
        let count: u64 = 0;
        const na = @as(usize, a.card);
        const nb = @as(usize, b.card);
        while (ia < na && ib < nb) {
            const va = a.items[ia];
            const vb = b.items[ib];
            if (va == vb) {
                count += 1;
                ia += 1;
                ib += 1;
            } else if (va < vb) {
                ia += 1;
            } else {
                ib += 1;
            }
        }
        return count;
    }

    if (a.kind == ContainerKind.Bitset && b.kind == ContainerKind.Bitset) {
        let count: u64 = 0;
        let wi: usize = 0;
        while (wi < BITSET_WORDS) : (wi += 1) {
            count += @as(u64, @popCount(a.words[wi] & b.words[wi]));
        }
        return count;
    }

    const arr = if (a.kind == ContainerKind.Array) a else b;
    const bs = if (a.kind == ContainerKind.Bitset) a else b;
    let count: u64 = 0;
    let i: usize = 0;
    const na = @as(usize, arr.card);
    while (i < na) : (i += 1) {
        if (bs.contains(arr.items[i])) {
            count += 1;
        }
    }
    return count;
}

fn containerIntersects(a: Container, b: Container): bool {
    if (a.kind == ContainerKind.Array && b.kind == ContainerKind.Array) {
        let ia: usize = 0;
        let ib: usize = 0;
        const na = @as(usize, a.card);
        const nb = @as(usize, b.card);
        while (ia < na && ib < nb) {
            const va = a.items[ia];
            const vb = b.items[ib];
            if (va == vb) return true;
            if (va < vb) {
                ia += 1;
            } else {
                ib += 1;
            }
        }
        return false;
    }

    if (a.kind == ContainerKind.Bitset && b.kind == ContainerKind.Bitset) {
        let wi: usize = 0;
        while (wi < BITSET_WORDS) : (wi += 1) {
            if ((a.words[wi] & b.words[wi]) != 0) return true;
        }
        return false;
    }

    const arr = if (a.kind == ContainerKind.Array) a else b;
    const bs = if (a.kind == ContainerKind.Bitset) a else b;
    let i: usize = 0;
    const na = @as(usize, arr.card);
    while (i < na) : (i += 1) {
        if (bs.contains(arr.items[i])) return true;
    }
    return false;
}

/// Compressed 32-bit integer set using adaptive Array/Bitset containers.
pub struct RoaringBitmap {
    alloc: std.mem.Allocator,
    containers: []Container = [],
    len: usize = 0,
    cap: usize = 0,

    /// Initialize an empty RoaringBitmap.
    pub fn init(alloc: std.mem.Allocator): RoaringBitmap {
        return RoaringBitmap{
            alloc: alloc,
            containers: [],
            len: 0,
            cap: 0,
        };
    }

    /// Free all containers and backing storage.
    pub fn deinit(*_) {
        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            _.containers[i].deinit(_.alloc);
        }
        if (_.containers.len > 0) {
            _.alloc.free(_.containers);
            _.containers = [];
        }
        _.len = 0;
        _.cap = 0;
    }

    /// Deep-copy this RoaringBitmap using `alloc`.
    pub fn clone(alloc: std.mem.Allocator): RoaringBitmap ! RoaringError {
        if (_.len == 0) {
            return RoaringBitmap.init(alloc);
        }
        let buf = alloc.alloc(Container, _.len) catch return RoaringError.OutOfMemory;
        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            buf[i] = _.containers[i].clone(alloc) catch |e| {
                let j: usize = 0;
                while (j < i) : (j += 1) {
                    buf[j].deinit(alloc);
                }
                alloc.free(buf);
                return e;
            };
        }
        return RoaringBitmap{
            alloc: alloc,
            containers: buf,
            len: _.len,
            cap: _.len,
        };
    }

    fn findContainerIndex(key: u16): SearchPos {
        let lo: usize = 0;
        let hi: usize = _.len;
        while (lo < hi) {
            const mid = lo + (hi - lo) / 2;
            const k = _.containers[mid].key;
            if (k == key) {
                return SearchPos{ found: true, idx: mid };
            } else if (k < key) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }
        return SearchPos{ found: false, idx: lo };
    }

    fn ensureContainerCap(*_, needed: usize): void ! RoaringError {
        if (needed <= _.cap) return;
        let new_cap = if (_.cap == 0) 4 else _.cap * 2;
        if (new_cap < needed) new_cap = needed;
        let new_buf = _.alloc.alloc(Container, new_cap) catch return RoaringError.OutOfMemory;
        if (_.len > 0) {
            @memcpy(new_buf[0.._.len], _.containers[0.._.len]);
        }
        if (_.containers.len > 0) {
            _.alloc.free(_.containers);
        }
        _.containers = new_buf;
        _.cap = new_cap;
    }

    fn insertContainerAt(*_, idx: usize, c: Container): void ! RoaringError {
        _.ensureContainerCap(_.len + 1)?;
        let i = _.len;
        while (i > idx) {
            _.containers[i] = _.containers[i - 1];
            i -= 1;
        }
        _.containers[idx] = c;
        _.len += 1;
    }

    fn removeContainerAt(*_, idx: usize) {
        _.containers[idx].deinit(_.alloc);
        let i = idx;
        const last = _.len - 1;
        while (i < last) : (i += 1) {
            _.containers[i] = _.containers[i + 1];
        }
        _.len -= 1;
    }

    /// Add `val` to the bitmap. Returns true if `val` was newly inserted.
    pub fn add(*_, val: u32): bool ! RoaringError {
        const key: u16 = @intCast(val >> 16);
        const low: u16 = @intCast(val & 0xFFFF);
        const res = _.findContainerIndex(key);
        if (res.found) {
            return _.containers[res.idx].add(_.alloc, low);
        }
        let c = Container.initArrayWithCap(_.alloc, key, 4)?;
        * = c.add(_.alloc, low) catch |e| {
            c.deinit(_.alloc);
            return e;
        };
        _.insertContainerAt(res.idx, c) catch |e| {
            c.deinit(_.alloc);
            return e;
        };
        return true;
    }

    /// Add all values in the half-open range `[start, end_excl)`.
    pub fn addRange(*_, start: u32, end_excl: u32): void ! RoaringError {
        if (start > end_excl) {
            return RoaringError.InvalidRange;
        }
        if (start == end_excl) {
            return;
        }
        const last_val: u32 = end_excl - 1;
        const start_key: u32 = start >> 16;
        const end_key: u32 = last_val >> 16;

        let cur_key: u32 = start_key;
        while (cur_key <= end_key) : (cur_key += 1) {
            const k: u16 = @intCast(cur_key);
            const low_s: u16 = if (cur_key == start_key) @intCast(start & 0xFFFF) else 0;
            const low_e: u16 = if (cur_key == end_key) @intCast(last_val & 0xFFFF) else 0xFFFF;
            const range_len: u32 = @as(u32, low_e) - @as(u32, low_s) + 1;

            const res = _.findContainerIndex(k);
            if (res.found) {
                _.containers[res.idx].addRangeInclusive(_.alloc, low_s, low_e)?;
            } else {
                let c = if (range_len > ARRAY_MAX_CARD)
                    Container.initBitsetEmpty(_.alloc, k)?
                else
                    Container.initArrayWithCap(_.alloc, k, @intCast(range_len))?;
                c.addRangeInclusive(_.alloc, low_s, low_e) catch |e| {
                    c.deinit(_.alloc);
                    return e;
                };
                _.insertContainerAt(res.idx, c) catch |e| {
                    c.deinit(_.alloc);
                    return e;
                };
            }
        }
    }

    /// Remove `val` from the bitmap. Returns true if it was present.
    pub fn removeVal(*_, val: u32): bool {
        const key: u16 = @intCast(val >> 16);
        const low: u16 = @intCast(val & 0xFFFF);
        const res = _.findContainerIndex(key);
        if (!res.found) return false;
        const removed = _.containers[res.idx].removeVal(_.alloc, low);
        if (removed && _.containers[res.idx].card == 0) {
            _.removeContainerAt(res.idx);
        }
        return removed;
    }

    /// Check whether `val` is present in the bitmap.
    pub fn contains(val: u32): bool {
        const key: u16 = @intCast(val >> 16);
        const low: u16 = @intCast(val & 0xFFFF);
        const res = _.findContainerIndex(key);
        if (!res.found) return false;
        return _.containers[res.idx].contains(low);
    }

    /// Return total number of elements in the bitmap.
    pub fn cardinality(): u64 {
        let total: u64 = 0;
        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            total += @as(u64, _.containers[i].card);
        }
        return total;
    }

    /// Return true if the bitmap contains zero elements.
    pub fn isEmpty(): bool {
        return _.len == 0;
    }

    /// Return the smallest element, or null if empty.
    pub fn minimum(): u32? {
        if (_.len == 0) return null;
        const c = _.containers[0];
        return (@as(u32, c.key) << 16) | @as(u32, c.minimumLow());
    }

    /// Return the largest element, or null if empty.
    pub fn maximum(): u32? {
        if (_.len == 0) return null;
        const c = _.containers[_.len - 1];
        return (@as(u32, c.key) << 16) | @as(u32, c.maximumLow());
    }

    /// Return the count of elements <= `val`.
    pub fn rank(val: u32): u64 {
        const key: u16 = @intCast(val >> 16);
        const low: u16 = @intCast(val & 0xFFFF);
        let total: u64 = 0;
        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            const c = _.containers[i];
            if (c.key < key) {
                total += @as(u64, c.card);
            } else if (c.key == key) {
                total += c.rankLow(low);
                break;
            } else {
                break;
            }
        }
        return total;
    }

    fn appendResultContainer(
        alloc: std.mem.Allocator,
        list: *[]Container,
        list_len: *usize,
        list_cap: *usize,
        c: Container
    ): void ! RoaringError {
        if (list_len.* >= list_cap.*) {
            const new_cap = if (list_cap.* == 0) 4 else list_cap.* * 2;
            let new_buf = alloc.alloc(Container, new_cap) catch return RoaringError.OutOfMemory;
            if (list_len.* > 0) {
                @memcpy(new_buf[0..list_len.*], list.*[0..list_len.*]);
            }
            if (list.*.len > 0) {
                alloc.free(list.*);
            }
            list.* = new_buf;
            list_cap.* = new_cap;
        }
        list.*[list_len.*] = c;
        list_len.* += 1;
    }

    fn freePartialContainers(alloc: std.mem.Allocator, list: []Container, list_len: usize) {
        let i: usize = 0;
        while (i < list_len) : (i += 1) {
            list[i].deinit(alloc);
        }
        if (list.len > 0) {
            alloc.free(list);
        }
    }

    /// Compute intersection (`self & other`) into a new RoaringBitmap.
    pub fn bitAnd(alloc: std.mem.Allocator, other: RoaringBitmap): RoaringBitmap ! RoaringError {
        let res_list: []Container = [];
        let res_len: usize = 0;
        let res_cap: usize = 0;
        errdefer RoaringBitmap.freePartialContainers(alloc, res_list, res_len);

        let ia: usize = 0;
        let ib: usize = 0;
        while (ia < _.len && ib < other.len) {
            const ka = _.containers[ia].key;
            const kb = other.containers[ib].key;
            if (ka == kb) {
                const mc = containerAnd(alloc, _.containers[ia], other.containers[ib])?;
                if (c := mc) {
                    RoaringBitmap.appendResultContainer(alloc, &res_list, &res_len, &res_cap, c) catch |e| {
                        let tmp = c;
                        tmp.deinit(alloc);
                        return e;
                    };
                }
                ia += 1;
                ib += 1;
            } else if (ka < kb) {
                ia += 1;
            } else {
                ib += 1;
            }
        }
        return RoaringBitmap{
            alloc: alloc,
            containers: res_list,
            len: res_len,
            cap: res_cap,
        };
    }

    /// Compute union (`self | other`) into a new RoaringBitmap.
    pub fn bitOr(alloc: std.mem.Allocator, other: RoaringBitmap): RoaringBitmap ! RoaringError {
        let res_list: []Container = [];
        let res_len: usize = 0;
        let res_cap: usize = 0;
        errdefer RoaringBitmap.freePartialContainers(alloc, res_list, res_len);

        let ia: usize = 0;
        let ib: usize = 0;
        while (ia < _.len && ib < other.len) {
            const ka = _.containers[ia].key;
            const kb = other.containers[ib].key;
            if (ka == kb) {
                const c = containerOr(alloc, _.containers[ia], other.containers[ib])?;
                RoaringBitmap.appendResultContainer(alloc, &res_list, &res_len, &res_cap, c) catch |e| {
                    let tmp = c;
                    tmp.deinit(alloc);
                    return e;
                };
                ia += 1;
                ib += 1;
            } else if (ka < kb) {
                const c = _.containers[ia].clone(alloc)?;
                RoaringBitmap.appendResultContainer(alloc, &res_list, &res_len, &res_cap, c) catch |e| {
                    let tmp = c;
                    tmp.deinit(alloc);
                    return e;
                };
                ia += 1;
            } else {
                const c = other.containers[ib].clone(alloc)?;
                RoaringBitmap.appendResultContainer(alloc, &res_list, &res_len, &res_cap, c) catch |e| {
                    let tmp = c;
                    tmp.deinit(alloc);
                    return e;
                };
                ib += 1;
            }
        }
        while (ia < _.len) : (ia += 1) {
            const c = _.containers[ia].clone(alloc)?;
            RoaringBitmap.appendResultContainer(alloc, &res_list, &res_len, &res_cap, c) catch |e| {
                let tmp = c;
                tmp.deinit(alloc);
                return e;
            };
        }
        while (ib < other.len) : (ib += 1) {
            const c = other.containers[ib].clone(alloc)?;
            RoaringBitmap.appendResultContainer(alloc, &res_list, &res_len, &res_cap, c) catch |e| {
                let tmp = c;
                tmp.deinit(alloc);
                return e;
            };
        }
        return RoaringBitmap{
            alloc: alloc,
            containers: res_list,
            len: res_len,
            cap: res_cap,
        };
    }

    /// Compute difference (`self \ other`) into a new RoaringBitmap.
    pub fn bitAndNot(alloc: std.mem.Allocator, other: RoaringBitmap): RoaringBitmap ! RoaringError {
        let res_list: []Container = [];
        let res_len: usize = 0;
        let res_cap: usize = 0;
        errdefer RoaringBitmap.freePartialContainers(alloc, res_list, res_len);

        let ia: usize = 0;
        let ib: usize = 0;
        while (ia < _.len && ib < other.len) {
            const ka = _.containers[ia].key;
            const kb = other.containers[ib].key;
            if (ka == kb) {
                const mc = containerAndNot(alloc, _.containers[ia], other.containers[ib])?;
                if (c := mc) {
                    RoaringBitmap.appendResultContainer(alloc, &res_list, &res_len, &res_cap, c) catch |e| {
                        let tmp = c;
                        tmp.deinit(alloc);
                        return e;
                    };
                }
                ia += 1;
                ib += 1;
            } else if (ka < kb) {
                const c = _.containers[ia].clone(alloc)?;
                RoaringBitmap.appendResultContainer(alloc, &res_list, &res_len, &res_cap, c) catch |e| {
                    let tmp = c;
                    tmp.deinit(alloc);
                    return e;
                };
                ia += 1;
            } else {
                ib += 1;
            }
        }
        while (ia < _.len) : (ia += 1) {
            const c = _.containers[ia].clone(alloc)?;
            RoaringBitmap.appendResultContainer(alloc, &res_list, &res_len, &res_cap, c) catch |e| {
                let tmp = c;
                tmp.deinit(alloc);
                return e;
            };
        }
        return RoaringBitmap{
            alloc: alloc,
            containers: res_list,
            len: res_len,
            cap: res_cap,
        };
    }

    /// Compute symmetric difference (`self ^ other`) into a new RoaringBitmap.
    pub fn bitXor(alloc: std.mem.Allocator, other: RoaringBitmap): RoaringBitmap ! RoaringError {
        let res_list: []Container = [];
        let res_len: usize = 0;
        let res_cap: usize = 0;
        errdefer RoaringBitmap.freePartialContainers(alloc, res_list, res_len);

        let ia: usize = 0;
        let ib: usize = 0;
        while (ia < _.len && ib < other.len) {
            const ka = _.containers[ia].key;
            const kb = other.containers[ib].key;
            if (ka == kb) {
                const mc = containerXor(alloc, _.containers[ia], other.containers[ib])?;
                if (c := mc) {
                    RoaringBitmap.appendResultContainer(alloc, &res_list, &res_len, &res_cap, c) catch |e| {
                        let tmp = c;
                        tmp.deinit(alloc);
                        return e;
                    };
                }
                ia += 1;
                ib += 1;
            } else if (ka < kb) {
                const c = _.containers[ia].clone(alloc)?;
                RoaringBitmap.appendResultContainer(alloc, &res_list, &res_len, &res_cap, c) catch |e| {
                    let tmp = c;
                    tmp.deinit(alloc);
                    return e;
                };
                ia += 1;
            } else {
                const c = other.containers[ib].clone(alloc)?;
                RoaringBitmap.appendResultContainer(alloc, &res_list, &res_len, &res_cap, c) catch |e| {
                    let tmp = c;
                    tmp.deinit(alloc);
                    return e;
                };
                ib += 1;
            }
        }
        while (ia < _.len) : (ia += 1) {
            const c = _.containers[ia].clone(alloc)?;
            RoaringBitmap.appendResultContainer(alloc, &res_list, &res_len, &res_cap, c) catch |e| {
                let tmp = c;
                tmp.deinit(alloc);
                return e;
            };
        }
        while (ib < other.len) : (ib += 1) {
            const c = other.containers[ib].clone(alloc)?;
            RoaringBitmap.appendResultContainer(alloc, &res_list, &res_len, &res_cap, c) catch |e| {
                let tmp = c;
                tmp.deinit(alloc);
                return e;
            };
        }
        return RoaringBitmap{
            alloc: alloc,
            containers: res_list,
            len: res_len,
            cap: res_cap,
        };
    }

    /// Return the cardinality of `self & other` without allocating.
    pub fn andCardinality(other: RoaringBitmap): u64 {
        let total: u64 = 0;
        let ia: usize = 0;
        let ib: usize = 0;
        while (ia < _.len && ib < other.len) {
            const ka = _.containers[ia].key;
            const kb = other.containers[ib].key;
            if (ka == kb) {
                total += containerAndCardinality(_.containers[ia], other.containers[ib]);
                ia += 1;
                ib += 1;
            } else if (ka < kb) {
                ia += 1;
            } else {
                ib += 1;
            }
        }
        return total;
    }

    /// Return true if `self` and `other` share at least one element.
    pub fn intersects(other: RoaringBitmap): bool {
        let ia: usize = 0;
        let ib: usize = 0;
        while (ia < _.len && ib < other.len) {
            const ka = _.containers[ia].key;
            const kb = other.containers[ib].key;
            if (ka == kb) {
                if (containerIntersects(_.containers[ia], other.containers[ib])) {
                    return true;
                }
                ia += 1;
                ib += 1;
            } else if (ka < kb) {
                ia += 1;
            } else {
                ib += 1;
            }
        }
        return false;
    }

    /// Export all elements in strictly ascending order. Caller frees with `alloc.free`.
    pub fn toSlice(alloc: std.mem.Allocator): []u32 ! RoaringError {
        const total: usize = @intCast(_.cardinality());
        if (total == 0) {
            return alloc.alloc(u32, 0) catch return RoaringError.OutOfMemory;
        }
        let out = alloc.alloc(u32, total) catch return RoaringError.OutOfMemory;
        let pos: usize = 0;
        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            pos = _.containers[i].appendToSlice(out, pos);
        }
        return out;
    }

    /// Serialize to compact binary format with "ROAR" magic header.
    pub fn serialize(alloc: std.mem.Allocator): []u8 ! RoaringError {
        let total_bytes: usize = 10; // "ROAR" (4) + version (2) + container_count (4)
        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            const c = _.containers[i];
            total_bytes += 7; // key (2) + kind (1) + card (4)
            if (c.kind == ContainerKind.Array) {
                total_bytes += @as(usize, c.card) * 2;
            } else {
                total_bytes += BITSET_WORDS * 8;
            }
        }
        let buf = alloc.alloc(u8, total_bytes) catch return RoaringError.OutOfMemory;
        buf[0] = SERIAL_MAGIC_0;
        buf[1] = SERIAL_MAGIC_1;
        buf[2] = SERIAL_MAGIC_2;
        buf[3] = SERIAL_MAGIC_3;
        writeU16LE(buf, 4, SERIAL_VERSION);
        writeU32LE(buf, 6, @intCast(_.len));

        let off: usize = 10;
        i = 0;
        while (i < _.len) : (i += 1) {
            const c = _.containers[i];
            writeU16LE(buf, off, c.key);
            buf[off + 2] = @intFromEnum(c.kind);
            writeU32LE(buf, off + 3, c.card);
            off += 7;
            if (c.kind == ContainerKind.Array) {
                let j: usize = 0;
                const n = @as(usize, c.card);
                while (j < n) : (j += 1) {
                    writeU16LE(buf, off, c.items[j]);
                    off += 2;
                }
            } else {
                let wi: usize = 0;
                while (wi < BITSET_WORDS) : (wi += 1) {
                    writeU64LE(buf, off, c.words[wi]);
                    off += 8;
                }
            }
        }
        return buf;
    }

    /// Deserialize from binary bytes, validating "ROAR" magic and container integrity.
    pub fn deserialize(alloc: std.mem.Allocator, bytes: []const u8): RoaringBitmap ! RoaringError {
        if (bytes.len < 10) {
            return RoaringError.InvalidSerializedData;
        }
        if (bytes[0] != SERIAL_MAGIC_0 ||
            bytes[1] != SERIAL_MAGIC_1 ||
            bytes[2] != SERIAL_MAGIC_2 ||
            bytes[3] != SERIAL_MAGIC_3)
        {
            return RoaringError.InvalidSerializedData;
        }
        const ver = readU16LE(bytes, 4);
        if (ver != SERIAL_VERSION) {
            return RoaringError.InvalidSerializedData;
        }
        const count = readU32LE(bytes, 6);
        if (count > BITSET_BITS) {
            return RoaringError.InvalidSerializedData;
        }
        if (count == 0) {
            if (bytes.len != 10) {
                return RoaringError.InvalidSerializedData;
            }
            return RoaringBitmap.init(alloc);
        }

        const c_count: usize = @intCast(count);
        let list = alloc.alloc(Container, c_count) catch return RoaringError.OutOfMemory;
        let parsed: usize = 0;
        errdefer RoaringBitmap.freePartialContainers(alloc, list, parsed);

        let off: usize = 10;
        let prev_key: u16 = 0;
        while (parsed < c_count) : (parsed += 1) {
            if (off + 7 > bytes.len) {
                return RoaringError.InvalidSerializedData;
            }
            const key = readU16LE(bytes, off);
            if (parsed > 0 && key <= prev_key) {
                return RoaringError.InvalidSerializedData;
            }
            prev_key = key;

            const kind_raw = bytes[off + 2];
            const card = readU32LE(bytes, off + 3);
            off += 7;

            if (kind_raw == 0) {
                if (card == 0 || card > ARRAY_MAX_CARD) {
                    return RoaringError.InvalidSerializedData;
                }
                const n: usize = @intCast(card);
                const payload_bytes = n * 2;
                if (off + payload_bytes > bytes.len) {
                    return RoaringError.InvalidSerializedData;
                }
                let items = alloc.alloc(u16, n) catch return RoaringError.OutOfMemory;
                let j: usize = 0;
                let prev_low: u16 = 0;
                while (j < n) : (j += 1) {
                    const v = readU16LE(bytes, off);
                    off += 2;
                    if (j > 0 && v <= prev_low) {
                        alloc.free(items);
                        return RoaringError.InvalidSerializedData;
                    }
                    prev_low = v;
                    items[j] = v;
                }
                list[parsed] = Container{
                    key: key,
                    kind: ContainerKind.Array,
                    card: card,
                    items: items,
                    cap: n,
                    words: [],
                };
            } else if (kind_raw == 1) {
                if (card <= ARRAY_MAX_CARD || card > BITSET_BITS) {
                    return RoaringError.InvalidSerializedData;
                }
                const payload_bytes = BITSET_WORDS * 8;
                if (off + payload_bytes > bytes.len) {
                    return RoaringError.InvalidSerializedData;
                }
                let words = alloc.alloc(u64, BITSET_WORDS) catch return RoaringError.OutOfMemory;
                let wi: usize = 0;
                let actual_card: u32 = 0;
                while (wi < BITSET_WORDS) : (wi += 1) {
                    const w = readU64LE(bytes, off);
                    off += 8;
                    words[wi] = w;
                    actual_card += @as(u32, @popCount(w));
                }
                if (actual_card != card) {
                    alloc.free(words);
                    return RoaringError.InvalidSerializedData;
                }
                list[parsed] = Container{
                    key: key,
                    kind: ContainerKind.Bitset,
                    card: card,
                    items: [],
                    cap: 0,
                    words: words,
                };
            } else {
                return RoaringError.InvalidSerializedData;
            }
        }

        if (off != bytes.len) {
            return RoaringError.InvalidSerializedData;
        }

        return RoaringBitmap{
            alloc: alloc,
            containers: list,
            len: c_count,
            cap: c_count,
        };
    }
}
