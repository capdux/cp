// lib/col/enumSet.cp — Zero-heap O(1) Enum BitSet & Enum Map (inspired by Zig std.enums)
// Uses hardware @popCount / @ctz for constant-time set algebra without hashing or allocation.

import std from "std";

pub err EnumColError {
    OutOfRange,
}

/// Zero-heap O(1) bitset for enums or compact tags with ordinal in `0..63`.
pub struct EnumSet64 {
    pub bits: u64 = 0,

    pub static fn empty(): EnumSet64 {
        return EnumSet64{ .bits = 0 };
    }

    pub static fn fromBits(raw: u64): EnumSet64 {
        return EnumSet64{ .bits = raw };
    }

    pub fn insert(*_, idx: u6) {
        _.bits |= (@as(u64, 1) << idx);
    }

    pub fn remove(*_, idx: u6) {
        _.bits &= ~(@as(u64, 1) << idx);
    }

    pub fn toggle(*_, idx: u6) {
        _.bits ^= (@as(u64, 1) << idx);
    }

    pub fn contains(idx: u6): bool {
        return (_.bits & (@as(u64, 1) << idx)) != 0;
    }

    pub fn isEmpty(): bool {
        return _.bits == 0;
    }

    pub fn count(): u32 {
        return @popCount(_.bits);
    }

    pub fn first(): u6? {
        if (_.bits == 0) return null;
        return @intCast(@ctz(_.bits));
    }

    pub fn popFirst(*_): u6? {
        if (_.bits == 0) return null;
        const idx: u6 = @intCast(@ctz(_.bits));
        _.bits &= (_.bits - 1);
        return idx;
    }

    pub fn unionWith(other: EnumSet64): EnumSet64 {
        return EnumSet64{ .bits = _.bits | other.bits };
    }

    pub fn intersectWith(other: EnumSet64): EnumSet64 {
        return EnumSet64{ .bits = _.bits & other.bits };
    }

    pub fn differenceWith(other: EnumSet64): EnumSet64 {
        return EnumSet64{ .bits = _.bits & ~other.bits };
    }

    pub fn isSubsetOf(other: EnumSet64): bool {
        return (_.bits & ~other.bits) == 0;
    }

    pub fn eql(other: EnumSet64): bool {
        return _.bits == other.bits;
    }

    pub fn clear(*_) {
        _.bits = 0;
    }
}

/// Zero-heap O(1) fixed-slot map indexed by enum ordinal `0..63`.
pub struct EnumMap64 {
    pub present: EnumSet64 = EnumSet64{ .bits = 0 },
    pub values: [64]i64 = undefined,

    pub static fn empty(): EnumMap64 {
        let m = EnumMap64{
            .present = EnumSet64.empty(),
            .values = undefined,
        };
        let i: usize = 0;
        while (i < 64) : (i += 1) {
            m.values[i] = 0;
        }
        return m;
    }

    pub fn put(*_, idx: u6, val: i64) {
        _.values[@as(usize, idx)] = val;
        _.present.insert(idx);
    }

    pub fn get(idx: u6): i64? {
        if (!_.present.contains(idx)) return null;
        return _.values[@as(usize, idx)];
    }

    pub fn remove(*_, idx: u6): i64? {
        if (!_.present.contains(idx)) return null;
        _.present.remove(idx);
        return _.values[@as(usize, idx)];
    }

    pub fn len(): u32 {
        return _.present.count();
    }

    pub fn contains(idx: u6): bool {
        return _.present.contains(idx);
    }
}
