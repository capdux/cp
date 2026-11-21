// Source: Absorbed from /data/dev/bun/src/jsc/bindings/JSBigInt.cpp | Synced: 2026-11-20T06:14:59Z
// lib/bigint/index.cp
// High-Performance Arbitrary-Precision BigInt and Web3/EVM uint256
// Backed by Zig std.math.big.int.Managed and native 256-bit arithmetic

const std = @import("std");

const Managed = std.math.big.int.Managed;

pub struct BigIntDivMod {
    pub quotient: BigInt;
    pub remainder: BigInt;

    pub fn deinit(*_) {
        _.quotient.deinit();
        _.remainder.deinit();
    }
}

pub struct BigInt {
    pub inner: Managed;

    pub static fn fromString(str: string, radix: u8 = 10): !BigInt {
        if (str.len == 0) {
            return error.InvalidCharacter;
        }
        let raw = str;
        let is_neg = false;
        if (raw[0] == '-') {
            is_neg = true;
            raw = raw[1..];
        } else if (raw[0] == '+') {
            raw = raw[1..];
        }
        if (radix == 16 && (std.mem.startsWith(u8, raw, "0x") || std.mem.startsWith(u8, raw, "0X"))) {
            raw = raw[2..];
        } else if (radix == 2 && (std.mem.startsWith(u8, raw, "0b") || std.mem.startsWith(u8, raw, "0B"))) {
            raw = raw[2..];
        } else if (radix == 8 && (std.mem.startsWith(u8, raw, "0o") || std.mem.startsWith(u8, raw, "0O"))) {
            raw = raw[2..];
        }
        if (raw.len == 0) {
            return error.InvalidCharacter;
        }
        let m = Managed.init(std.heap.page_allocator)?;
        errdefer m.deinit();
        m.setString(radix, raw)?;
        if (is_neg && !m.eqlZero()) {
            m.negate();
        }
        return BigInt{ inner: m };
    }

    pub static fn fromDecimal(str: string): !BigInt {
        return BigInt.fromString(str, 10);
    }

    pub static fn fromHex(str: string): !BigInt {
        return BigInt.fromString(str, 16);
    }

    pub static fn fromBinary(str: string): !BigInt {
        return BigInt.fromString(str, 2);
    }

    pub static fn fromInt(val: i64): !BigInt {
        const m = Managed.initSet(std.heap.page_allocator, val)?;
        return BigInt{ inner: m };
    }

    pub static fn fromUint(val: u64): !BigInt {
        const m = Managed.initSet(std.heap.page_allocator, val)?;
        return BigInt{ inner: m };
    }

    pub fn clone(*_): !BigInt {
        const c = _.inner.clone()?;
        return BigInt{ inner: c };
    }

    pub fn deinit(*_) {
        _.inner.deinit();
    }

    pub fn toString(*_, radix: u8 = 10): string {
        return _.inner.toString(std.heap.page_allocator, radix, .lower) catch unreachable;
    }

    pub fn toDecimal(*_): string {
        return _.toString(10);
    }

    pub fn toHex(*_): string {
        return _.toString(16);
    }

    pub fn toHexPrefixed(*_): string {
        const raw = _.toHex();
        if (raw.len > 0 && raw[0] == '-') {
            return std.fmt.allocPrint(std.heap.page_allocator, "-0x{s}", .{raw[1..]}) catch unreachable;
        }
        return std.fmt.allocPrint(std.heap.page_allocator, "0x{s}", .{raw}) catch unreachable;
    }

    pub fn toBinary(*_): string {
        return _.toString(2);
    }

    pub fn toBinaryPrefixed(*_): string {
        const raw = _.toBinary();
        if (raw.len > 0 && raw[0] == '-') {
            return std.fmt.allocPrint(std.heap.page_allocator, "-0b{s}", .{raw[1..]}) catch unreachable;
        }
        return std.fmt.allocPrint(std.heap.page_allocator, "0b{s}", .{raw}) catch unreachable;
    }

    pub fn add(*_, other: *const BigInt): !BigInt {
        zig {
            var res = try Managed.init(std.heap.page_allocator);
            errdefer res.deinit();
            try res.add(&self.inner, &other.inner);
            return BigInt{ .inner = res };
        }
    }

    pub fn sub(*_, other: *const BigInt): !BigInt {
        zig {
            var res = try Managed.init(std.heap.page_allocator);
            errdefer res.deinit();
            try res.sub(&self.inner, &other.inner);
            return BigInt{ .inner = res };
        }
    }

    pub fn mul(*_, other: *const BigInt): !BigInt {
        zig {
            var res = try Managed.init(std.heap.page_allocator);
            errdefer res.deinit();
            try res.mul(&self.inner, &other.inner);
            return BigInt{ .inner = res };
        }
    }

    pub fn divTrunc(*_, other: *const BigInt): !BigInt {
        zig {
            if (other.inner.eqlZero()) return error.DivisionByZero;
            var q = try Managed.init(std.heap.page_allocator);
            errdefer q.deinit();
            var r = try Managed.init(std.heap.page_allocator);
            defer r.deinit();
            try Managed.divTrunc(&q, &r, &self.inner, &other.inner);
            return BigInt{ .inner = q };
        }
    }

    pub fn rem(*_, other: *const BigInt): !BigInt {
        zig {
            if (other.inner.eqlZero()) return error.DivisionByZero;
            var q = try Managed.init(std.heap.page_allocator);
            defer q.deinit();
            var r = try Managed.init(std.heap.page_allocator);
            errdefer r.deinit();
            try Managed.divTrunc(&q, &r, &self.inner, &other.inner);
            return BigInt{ .inner = r };
        }
    }

    /// Truncating division returning both quotient and remainder (`a == q * b + r`).
    pub fn divMod(*_, other: *const BigInt): !BigIntDivMod {
        zig {
            if (other.inner.eqlZero()) return error.DivisionByZero;
            var q = try Managed.init(std.heap.page_allocator);
            errdefer q.deinit();
            var r = try Managed.init(std.heap.page_allocator);
            errdefer r.deinit();
            try Managed.divTrunc(&q, &r, &self.inner, &other.inner);
            return BigIntDivMod{
                .quotient = BigInt{ .inner = q },
                .remainder = BigInt{ .inner = r },
            };
        }
    }

    /// Mathematical floored modulo (`divFloor` remainder). For positive `other`, result is in `0..other-1`.
    pub fn mod(*_, other: *const BigInt): !BigInt {
        zig {
            if (other.inner.eqlZero()) return error.DivisionByZero;
            var q = try Managed.init(std.heap.page_allocator);
            defer q.deinit();
            var r = try Managed.init(std.heap.page_allocator);
            errdefer r.deinit();
            try Managed.divFloor(&q, &r, &self.inner, &other.inner);
            return BigInt{ .inner = r };
        }
    }

    pub fn abs(*_): !BigInt {
        zig {
            var res = try self.inner.clone();
            res.abs();
            return BigInt{ .inner = res };
        }
    }

    pub fn neg(*_): !BigInt {
        zig {
            var res = try self.inner.clone();
            if (!res.eqlZero()) {
                res.negate();
            }
            return BigInt{ .inner = res };
        }
    }

    pub fn bitAnd(*_, other: *const BigInt): !BigInt {
        zig {
            var res = try Managed.init(std.heap.page_allocator);
            errdefer res.deinit();
            try res.bitAnd(&self.inner, &other.inner);
            return BigInt{ .inner = res };
        }
    }

    pub fn bitOr(*_, other: *const BigInt): !BigInt {
        zig {
            var res = try Managed.init(std.heap.page_allocator);
            errdefer res.deinit();
            try res.bitOr(&self.inner, &other.inner);
            return BigInt{ .inner = res };
        }
    }

    pub fn bitXor(*_, other: *const BigInt): !BigInt {
        zig {
            var res = try Managed.init(std.heap.page_allocator);
            errdefer res.deinit();
            try res.bitXor(&self.inner, &other.inner);
            return BigInt{ .inner = res };
        }
    }

    pub fn shl(*_, shift: usize): !BigInt {
        zig {
            var res = try Managed.init(std.heap.page_allocator);
            errdefer res.deinit();
            try res.shiftLeft(&self.inner, shift);
            return BigInt{ .inner = res };
        }
    }

    pub fn shr(*_, shift: usize): !BigInt {
        zig {
            var res = try Managed.init(std.heap.page_allocator);
            errdefer res.deinit();
            try res.shiftRight(&self.inner, shift);
            return BigInt{ .inner = res };
        }
    }

    pub fn pow(*_, exp: u32): !BigInt {
        zig {
            var res = try Managed.init(std.heap.page_allocator);
            errdefer res.deinit();
            try res.pow(&self.inner, exp);
            return BigInt{ .inner = res };
        }
    }

    pub fn gcd(*_, other: *const BigInt): !BigInt {
        zig {
            var res = try Managed.init(std.heap.page_allocator);
            errdefer res.deinit();
            try res.gcd(&self.inner, &other.inner);
            return BigInt{ .inner = res };
        }
    }

    /// Computes `(self ^ exp) mod modulus` via square-and-multiply binary exponentiation.
    pub fn modPow(*_, exp: *const BigInt, modulus: *const BigInt): !BigInt {
        zig {
            if (modulus.inner.eqlZero() or !modulus.inner.isPositive()) {
                return error.InvalidModulus;
            }
            if (!exp.inner.isPositive() and !exp.inner.eqlZero()) {
                return error.NegativeExponent;
            }
            var one = try Managed.initSet(std.heap.page_allocator, @as(u64, 1));
            defer one.deinit();
            if (modulus.inner.eql(one)) {
                return BigInt{ .inner = try Managed.initSet(std.heap.page_allocator, @as(u64, 0)) };
            }

            var result = try Managed.initSet(std.heap.page_allocator, @as(u64, 1));
            errdefer result.deinit();

            var base = try Managed.init(std.heap.page_allocator);
            defer base.deinit();
            var q = try Managed.init(std.heap.page_allocator);
            defer q.deinit();
            var prod = try Managed.init(std.heap.page_allocator);
            defer prod.deinit();

            // base = self mod modulus (non-negative)
            try Managed.divFloor(&q, &base, &self.inner, &modulus.inner);

            if (exp.inner.eqlZero()) {
                return BigInt{ .inner = result };
            }

            const total_bits = exp.inner.bitCountAbs();
            const limb_bits = @bitSizeOf(std.math.big.Limb);
            var bit_pos: usize = 0;
            while (bit_pos < total_bits) : (bit_pos += 1) {
                const limb_idx = bit_pos / limb_bits;
                const bit_in_limb = bit_pos % limb_bits;
                const bit_set = (exp.inner.limbs[limb_idx] & (@as(std.math.big.Limb, 1) << @intCast(bit_in_limb))) != 0;
                if (bit_set) {
                    try prod.mul(&result, &base);
                    try Managed.divFloor(&q, &result, &prod, &modulus.inner);
                }
                if (bit_pos + 1 < total_bits) {
                    try prod.sqr(&base);
                    try Managed.divFloor(&q, &base, &prod, &modulus.inner);
                }
            }

            return BigInt{ .inner = result };
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

    pub fn cmp(*_, other: *const BigInt): i32 {
        return _.order(other);
    }

    pub fn eq(*_, other: *const BigInt): bool {
        zig {
            return self.inner.toConst().eql(other.inner.toConst());
        }
    }

    pub fn ne(*_, other: *const BigInt): bool {
        return !_.eq(other);
    }

    pub fn lt(*_, other: *const BigInt): bool {
        return _.order(other) < 0;
    }

    pub fn le(*_, other: *const BigInt): bool {
        return _.order(other) <= 0;
    }

    pub fn gt(*_, other: *const BigInt): bool {
        return _.order(other) > 0;
    }

    pub fn ge(*_, other: *const BigInt): bool {
        return _.order(other) >= 0;
    }

    pub fn eqZero(*_): bool {
        zig {
            return self.inner.eqlZero();
        }
    }

    pub fn isZero(*_): bool {
        return _.eqZero();
    }

    pub fn isNegative(*_): bool {
        zig {
            return !self.inner.isPositive() and !self.inner.eqlZero();
        }
    }

    pub fn isPositive(*_): bool {
        zig {
            return self.inner.isPositive() and !self.inner.eqlZero();
        }
    }

    pub fn isOdd(*_): bool {
        return _.inner.isOdd();
    }

    pub fn isEven(*_): bool {
        return _.inner.isEven();
    }

    pub fn bitCount(*_): usize {
        return _.inner.bitCountAbs();
    }
}

pub struct BigInt256DivMod {
    pub quotient: BigInt256;
    pub remainder: BigInt256;
}

pub struct BigInt256Overflow {
    pub value: BigInt256;
    pub overflow: bool;
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

    pub static fn one(): BigInt256 {
        return BigInt256.fromU64(1);
    }

    pub static fn max(): BigInt256 {
        let res = BigInt256{ limbs: undefined };
        const all_ones: u64 = std.math.maxInt(u64);
        res.limbs[0] = all_ones;
        res.limbs[1] = all_ones;
        res.limbs[2] = all_ones;
        res.limbs[3] = all_ones;
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
        if (raw.len == 0) {
            return error.InvalidCharacter;
        }
        const val = std.fmt.parseInt(u256, raw, 16)?;
        return BigInt256{ limbs: @as([4]u64, @bitCast(val)) };
    }

    pub static fn fromDecimal(s: string): !BigInt256 {
        if (s.len == 0) {
            return error.InvalidCharacter;
        }
        const val = std.fmt.parseInt(u256, s, 10)?;
        return BigInt256{ limbs: @as([4]u64, @bitCast(val)) };
    }

    pub static fn fromBinary(s: string): !BigInt256 {
        let raw = s;
        if (std.mem.startsWith(u8, raw, "0b") || std.mem.startsWith(u8, raw, "0B")) {
            raw = raw[2..];
        }
        if (raw.len == 0) {
            return error.InvalidCharacter;
        }
        const val = std.fmt.parseInt(u256, raw, 2)?;
        return BigInt256{ limbs: @as([4]u64, @bitCast(val)) };
    }

    pub fn isZero(): bool {
        return @as(u256, @bitCast(_.limbs)) == 0;
    }

    pub fn toHex(): string {
        const val = @as(u256, @bitCast(_.limbs));
        return std.fmt.allocPrint(std.heap.page_allocator, "{x}", .{val}) catch unreachable;
    }

    pub fn toHexPrefixed(): string {
        const val = @as(u256, @bitCast(_.limbs));
        return std.fmt.allocPrint(std.heap.page_allocator, "0x{x}", .{val}) catch unreachable;
    }

    pub fn toDecimal(): string {
        const val = @as(u256, @bitCast(_.limbs));
        return std.fmt.allocPrint(std.heap.page_allocator, "{d}", .{val}) catch unreachable;
    }

    pub fn toBinary(): string {
        const val = @as(u256, @bitCast(_.limbs));
        return std.fmt.allocPrint(std.heap.page_allocator, "{b}", .{val}) catch unreachable;
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

    pub fn addOverflow(other: BigInt256): BigInt256Overflow {
        zig {
            const self_u = @as(u256, @bitCast(self.limbs));
            const other_u = @as(u256, @bitCast(other.limbs));
            const res = @addWithOverflow(self_u, other_u);
            return BigInt256Overflow{
                .value = BigInt256{ .limbs = @as([4]u64, @bitCast(res[0])) },
                .overflow = res[1] != 0,
            };
        }
    }

    pub fn addChecked(other: BigInt256): !BigInt256 {
        const r = _.addOverflow(other);
        if (r.overflow) {
            return error.Overflow;
        }
        return r.value;
    }

    pub fn subOverflow(other: BigInt256): BigInt256Overflow {
        zig {
            const self_u = @as(u256, @bitCast(self.limbs));
            const other_u = @as(u256, @bitCast(other.limbs));
            const res = @subWithOverflow(self_u, other_u);
            return BigInt256Overflow{
                .value = BigInt256{ .limbs = @as([4]u64, @bitCast(res[0])) },
                .overflow = res[1] != 0,
            };
        }
    }

    pub fn subChecked(other: BigInt256): !BigInt256 {
        const r = _.subOverflow(other);
        if (r.overflow) {
            return error.Underflow;
        }
        return r.value;
    }

    pub fn mulOverflow(other: BigInt256): BigInt256Overflow {
        zig {
            const self_u = @as(u256, @bitCast(self.limbs));
            const other_u = @as(u256, @bitCast(other.limbs));
            const res = @mulWithOverflow(self_u, other_u);
            return BigInt256Overflow{
                .value = BigInt256{ .limbs = @as([4]u64, @bitCast(res[0])) },
                .overflow = res[1] != 0,
            };
        }
    }

    pub fn mulChecked(other: BigInt256): !BigInt256 {
        const r = _.mulOverflow(other);
        if (r.overflow) {
            return error.Overflow;
        }
        return r.value;
    }

    pub fn divMod(other: BigInt256): !BigInt256DivMod {
        const self_u = @as(u256, @bitCast(_.limbs));
        const other_u = @as(u256, @bitCast(other.limbs));
        if (other_u == 0) {
            return error.DivisionByZero;
        }
        const q = self_u / other_u;
        const r = self_u % other_u;
        return BigInt256DivMod {
            quotient: BigInt256{ limbs: @as([4]u64, @bitCast(q)) },
            remainder: BigInt256{ limbs: @as([4]u64, @bitCast(r)) },
        };
    }

    pub fn div(other: BigInt256): !BigInt256 {
        const dm = _.divMod(other)?;
        return dm.quotient;
    }

    pub fn mod(other: BigInt256): !BigInt256 {
        const dm = _.divMod(other)?;
        return dm.remainder;
    }

    pub fn shl(shift: usize): BigInt256 {
        if (shift >= 256) {
            return BigInt256.zero();
        }
        zig {
            const self_u = @as(u256, @bitCast(self.limbs));
            const shifted = self_u << @intCast(shift);
            return BigInt256{ .limbs = @as([4]u64, @bitCast(shifted)) };
        }
    }

    pub fn shr(shift: usize): BigInt256 {
        if (shift >= 256) {
            return BigInt256.zero();
        }
        zig {
            const self_u = @as(u256, @bitCast(self.limbs));
            const shifted = self_u >> @intCast(shift);
            return BigInt256{ .limbs = @as([4]u64, @bitCast(shifted)) };
        }
    }

    pub fn bitAnd(other: BigInt256): BigInt256 {
        const self_u = @as(u256, @bitCast(_.limbs));
        const other_u = @as(u256, @bitCast(other.limbs));
        return BigInt256{ limbs: @as([4]u64, @bitCast(self_u & other_u)) };
    }

    pub fn bitOr(other: BigInt256): BigInt256 {
        const self_u = @as(u256, @bitCast(_.limbs));
        const other_u = @as(u256, @bitCast(other.limbs));
        return BigInt256{ limbs: @as([4]u64, @bitCast(self_u | other_u)) };
    }

    pub fn bitXor(other: BigInt256): BigInt256 {
        const self_u = @as(u256, @bitCast(_.limbs));
        const other_u = @as(u256, @bitCast(other.limbs));
        return BigInt256{ limbs: @as([4]u64, @bitCast(self_u ^ other_u)) };
    }

    pub fn bitNot(): BigInt256 {
        const self_u = @as(u256, @bitCast(_.limbs));
        return BigInt256{ limbs: @as([4]u64, @bitCast(~self_u)) };
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

    pub fn ne(other: BigInt256): bool {
        return !_.eq(other);
    }

    pub fn lt(other: BigInt256): bool {
        return _.cmp(other) < 0;
    }

    pub fn le(other: BigInt256): bool {
        return _.cmp(other) <= 0;
    }

    pub fn gt(other: BigInt256): bool {
        return _.cmp(other) > 0;
    }

    pub fn ge(other: BigInt256): bool {
        return _.cmp(other) >= 0;
    }
}
