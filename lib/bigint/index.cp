// Source: /data/dev/bun/src/jsc/bindings/JSBigInt.cpp
// lib/bigint/index.cp
// High-Performance Arbitrary-Precision BigInt and Web3/EVM uint256
// Backed by Zig std.math.big.int.Managed and native 256-bit arithmetic

const std = @import("std");

const Managed = std.math.big.int.Managed;

pub struct BigInt {
    pub inner: Managed;

    pub static fn fromString(str: string, radix: u8 = 10): !BigInt {
        let m = Managed.init(std.heap.page_allocator)?;
        m.setString(radix, str)?;
        return BigInt{ inner: m };
    }

    pub static fn fromInt(val: i64): !BigInt {
        const m = Managed.initSet(std.heap.page_allocator, val)?;
        return BigInt{ inner: m };
    }

    pub static fn fromUint(val: u64): !BigInt {
        const m = Managed.initSet(std.heap.page_allocator, val)?;
        return BigInt{ inner: m };
    }

    pub fn deinit(*_) {
        _.inner.deinit();
    }

    pub fn toString(*_, radix: u8 = 10): string {
        return _.inner.toString(std.heap.page_allocator, radix, .lower) catch unreachable;
    }

    pub fn add(*_, other: *const BigInt): !BigInt {
        zig {
            var res = try Managed.init(std.heap.page_allocator);
            try res.add(&self.inner, &other.inner);
            return BigInt{ .inner = res };
        }
    }

    pub fn sub(*_, other: *const BigInt): !BigInt {
        zig {
            var res = try Managed.init(std.heap.page_allocator);
            try res.sub(&self.inner, &other.inner);
            return BigInt{ .inner = res };
        }
    }

    pub fn mul(*_, other: *const BigInt): !BigInt {
        zig {
            var res = try Managed.init(std.heap.page_allocator);
            try res.mul(&self.inner, &other.inner);
            return BigInt{ .inner = res };
        }
    }

    pub fn divTrunc(*_, other: *const BigInt): !BigInt {
        zig {
            var q = try Managed.init(std.heap.page_allocator);
            var r = try Managed.init(std.heap.page_allocator);
            defer r.deinit();
            try Managed.divTrunc(&q, &r, &self.inner, &other.inner);
            return BigInt{ .inner = q };
        }
    }

    pub fn rem(*_, other: *const BigInt): !BigInt {
        zig {
            var q = try Managed.init(std.heap.page_allocator);
            var r = try Managed.init(std.heap.page_allocator);
            defer q.deinit();
            try Managed.divTrunc(&q, &r, &self.inner, &other.inner);
            return BigInt{ .inner = r };
        }
    }

    pub fn order(*_, other: *const BigInt): i32 {
        zig {
            return switch (self.inner.toConst().order(other.inner.toConst())) {
                .lt => -1,
                .eq => 0,
                .gt => 1,
            };
        }
    }

    pub fn eq(*_, other: *const BigInt): bool {
        zig {
            return self.inner.toConst().eql(other.inner.toConst());
        }
    }

    pub fn eqZero(*_): bool {
        zig {
            return self.inner.eqlZero();
        }
    }

    pub fn isNegative(*_): bool {
        zig {
            return !self.inner.isPositive() and !self.inner.eqlZero();
        }
    }

    pub fn bitCount(*_): usize {
        return _.inner.bitCountAbs();
    }
}

pub struct BigInt256 {
    pub limbs: [4]u64;

    pub static fn zero(): BigInt256 {
        let res = BigInt256{ limbs: undefined };
        res.limbs[0] = 0;
        res.limbs[1] = 0;
        res.limbs[2] = 0;
        res.limbs[3] = 0;
        return res;
    }

    pub static fn fromU64(v: u64): BigInt256 {
        let res = BigInt256{ limbs: undefined };
        res.limbs[0] = v;
        res.limbs[1] = 0;
        res.limbs[2] = 0;
        res.limbs[3] = 0;
        return res;
    }

    pub static fn fromHex(s: string): !BigInt256 {
        let raw = s;
        if (std.mem.startsWith(u8, raw, "0x") || std.mem.startsWith(u8, raw, "0X")) {
            raw = raw[2..];
        }
        const val = std.fmt.parseInt(u256, raw, 16)?;
        return BigInt256{ limbs: @as([4]u64, @bitCast(val)) };
    }

    pub fn toHex(): string {
        const val = @as(u256, @bitCast(_.limbs));
        return std.fmt.allocPrint(std.heap.page_allocator, "{x}", .{val}) catch unreachable;
    }

    pub fn add(other: BigInt256): BigInt256 {
        const self_u = @as(u256, @bitCast(_.limbs));
        const other_u = @as(u256, @bitCast(other.limbs));
        return BigInt256{ limbs: @as([4]u64, @bitCast(self_u +% other_u)) };
    }

    pub fn sub(other: BigInt256): BigInt256 {
        const self_u = @as(u256, @bitCast(_.limbs));
        const other_u = @as(u256, @bitCast(other.limbs));
        return BigInt256{ limbs: @as([4]u64, @bitCast(self_u -% other_u)) };
    }

    pub fn mul(other: BigInt256): BigInt256 {
        const self_u = @as(u256, @bitCast(_.limbs));
        const other_u = @as(u256, @bitCast(other.limbs));
        return BigInt256{ limbs: @as([4]u64, @bitCast(self_u *% other_u)) };
    }

    pub fn cmp(other: BigInt256): i32 {
        const self_u = @as(u256, @bitCast(_.limbs));
        const other_u = @as(u256, @bitCast(other.limbs));
        if (self_u < other_u) return -1;
        if (self_u > other_u) return 1;
        return 0;
    }

    pub fn eq(other: BigInt256): bool {
        return @as(u256, @bitCast(_.limbs)) == @as(u256, @bitCast(other.limbs));
    }
}
