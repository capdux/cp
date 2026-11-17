// Source: Derived from Bun & Zig std.math.big
// lib/bigint/index.cp
// High-Performance Arbitrary-Precision BigInt and Web3/EVM uint256
// Absorbed from Zig std.math.big.int.Managed and native u256

const std = @import("std");

const Managed = std.math.big.int.Managed;
const Allocator = std.mem.Allocator;

pub struct BigInt {
    inner: Managed,

    pub static fn fromString(str: string, radix: u8 = 10): !BigInt {
        let m = try Managed.init(std.heap.page_allocator);
        try m.setString(radix, str);
        return BigInt{ .inner = m };
    }

    pub static fn fromInt(val: i64): !BigInt {
        let m = try Managed.initSet(std.heap.page_allocator, val);
        return BigInt{ .inner = m };
    }

    pub static fn fromUint(val: u64): !BigInt {
        let m = try Managed.initSet(std.heap.page_allocator, val);
        return BigInt{ .inner = m };
    }

    pub fn deinit(*_) {
        _.inner.deinit();
    }

    pub fn toString(*_, radix: u8 = 10): string {
        return _.inner.toString(std.heap.page_allocator, radix, .lower) catch unreachable;
    }

    pub fn add(*_, other: *const BigInt): !BigInt {
        let res = try Managed.init(std.heap.page_allocator);
        try Managed.add(&res, &_.inner, &other.inner);
        return BigInt{ .inner = res };
    }

    pub fn sub(*_, other: *const BigInt): !BigInt {
        let res = try Managed.init(std.heap.page_allocator);
        try Managed.sub(&res, &_.inner, &other.inner);
        return BigInt{ .inner = res };
    }

    pub fn mul(*_, other: *const BigInt): !BigInt {
        let res = try Managed.init(std.heap.page_allocator);
        try Managed.mul(&res, &_.inner, &other.inner);
        return BigInt{ .inner = res };
    }

    pub fn divTrunc(*_, other: *const BigInt): !BigInt {
        let q = try Managed.init(std.heap.page_allocator);
        let r = try Managed.init(std.heap.page_allocator);
        defer r.deinit();
        try Managed.divTrunc(&q, &r, &_.inner, &other.inner);
        return BigInt{ .inner = q };
    }

    pub fn rem(*_, other: *const BigInt): !BigInt {
        let q = try Managed.init(std.heap.page_allocator);
        let r = try Managed.init(std.heap.page_allocator);
        defer q.deinit();
        try Managed.divTrunc(&q, &r, &_.inner, &other.inner);
        return BigInt{ .inner = r };
    }

    pub fn order(*_, other: *const BigInt): i32 {
        const ord = Managed.order(_.inner, other.inner);
        return switch (ord) {
            .lt => -1,
            .eq => 0,
            .gt => 1,
        };
    }

    pub fn eq(*_, other: *const BigInt): bool {
        return Managed.eql(_.inner, other.inner);
    }

    pub fn eqZero(*_): bool {
        return Managed.eqlZero(_.inner);
    }

    pub fn isNegative(*_): bool {
        return !_.inner.isPositive() and !Managed.eqlZero(_.inner);
    }

    pub fn bitCount(*_): usize {
        return _.inner.bitCountAbs();
    }
}

pub struct BigInt256 {
    pub limbs: [4]u64,

    pub static fn zero(): BigInt256 {
        let res = BigInt256{ .limbs = undefined };
        res.limbs[0] = 0;
        res.limbs[1] = 0;
        res.limbs[2] = 0;
        res.limbs[3] = 0;
        return res;
    }

    pub static fn fromU64(v: u64): BigInt256 {
        let res = BigInt256{ .limbs = undefined };
        res.limbs[0] = v;
        res.limbs[1] = 0;
        res.limbs[2] = 0;
        res.limbs[3] = 0;
        return res;
    }

    pub static fn fromHex(s: string): !BigInt256 {
        let raw = s;
        if (std.mem.startsWith(u8, raw, "0x") or std.mem.startsWith(u8, raw, "0X")) {
            raw = raw[2..];
        }
        const val = try std.fmt.parseInt(u256, raw, 16);
        return BigInt256{ .limbs = @as([4]u64, @bitCast(val)) };
    }

    pub fn toHex(*_): string {
        const val = @as(u256, @bitCast(_.limbs));
        return std.fmt.allocPrint(std.heap.page_allocator, "{x}", .{val}) catch unreachable;
    }

    pub fn add(*_, other: BigInt256): BigInt256 {
        const self_u = @as(u256, @bitCast(_.limbs));
        const other_u = @as(u256, @bitCast(other.limbs));
        return BigInt256{ .limbs = @as([4]u64, @bitCast(self_u +% other_u)) };
    }

    pub fn sub(*_, other: BigInt256): BigInt256 {
        const self_u = @as(u256, @bitCast(_.limbs));
        const other_u = @as(u256, @bitCast(other.limbs));
        return BigInt256{ .limbs = @as([4]u64, @bitCast(self_u -% other_u)) };
    }

    pub fn mul(*_, other: BigInt256): BigInt256 {
        const self_u = @as(u256, @bitCast(_.limbs));
        const other_u = @as(u256, @bitCast(other.limbs));
        return BigInt256{ .limbs = @as([4]u64, @bitCast(self_u *% other_u)) };
    }

    pub fn cmp(*_, other: BigInt256): i32 {
        const self_u = @as(u256, @bitCast(_.limbs));
        const other_u = @as(u256, @bitCast(other.limbs));
        if (self_u < other_u) return -1;
        if (self_u > other_u) return 1;
        return 0;
    }

    pub fn eq(*_, other: BigInt256): bool {
        return @as(u256, @bitCast(_.limbs)) == @as(u256, @bitCast(other.limbs));
    }
}
