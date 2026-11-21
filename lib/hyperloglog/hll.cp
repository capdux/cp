// Source: cp/lib/hyperloglog/hll.cp (Pure CP HyperLogLog 64-bit Cardinality Estimator)
const std = @import("std");

pub const HllError = error{
    InvalidPrecision,
    PrecisionMismatch,
    InvalidSerializedData,
};

/// Default precision p = 14 => m = 16384 registers (~16 KB, ~0.81% std error)
pub const DEFAULT_PRECISION: u6 = 14;

fn hash64(data: []const u8): u64 {
    return std.hash.Wyhash.hash(0x9e3779b97f4a7c15, data);
}

fn alphaM(m: usize): f64 {
    switch (m) {
        16 => return 0.673,
        32 => return 0.697,
        64 => return 0.709,
        else => {
            const mf: f64 = @floatFromInt(m);
            return 0.7213 / (1.0 + 1.079 / mf);
        },
    }
}

pub const HyperLogLog = struct {
    precision: u6,
    m: usize,
    registers: []u8,

    pub fn init(alloc: std.mem.Allocator, precision: u6): !HyperLogLog {
        if (precision < 4 or precision > 18) {
            return HllError.InvalidPrecision;
        }
        const shift: u6 = precision;
        const m: usize = @as(usize, 1) << shift;
        const regs = try alloc.alloc(u8, m);
        @memset(regs, 0);
        return HyperLogLog{
            .precision = precision,
            .m = m,
            .registers = regs,
        };
    }

    pub fn deinit(self: *HyperLogLog, alloc: std.mem.Allocator) {
        alloc.free(self.registers);
    }

    pub fn addBytes(self: *HyperLogLog, data: []const u8) {
        const h = hash64(data);
        const shift: u6 = @intCast(64 - @as(u7, self.precision));
        const idx: usize = @intCast(h >> shift);
        const w = h << self.precision;
        const max_rem: u8 = @intCast(64 - @as(u7, self.precision) + 1);
        const lz: u8 = @intCast(@clz(w));
        const rho: u8 = if (w == 0) max_rem else @min(lz + 1, max_rem);
        if (rho > self.registers[idx]) {
            self.registers[idx] = rho;
        }
    }

    pub fn addU64(self: *HyperLogLog, val: u64) {
        let buf: [8]u8 = undefined;
        std.mem.writeInt(u64, &buf, val, .little);
        self.addBytes(&buf);
    }

    pub fn merge(self: *HyperLogLog, other: HyperLogLog): HllError!void {
        if (self.precision != other.precision) {
            return HllError.PrecisionMismatch;
        }
        let i: usize = 0;
        while (i < self.m) : (i += 1) {
            if (other.registers[i] > self.registers[i]) {
                self.registers[i] = other.registers[i];
            }
        }
    }

    pub fn count(self: HyperLogLog): u64 {
        let sum: f64 = 0.0;
        let zeros: usize = 0;
        for (reg of self.registers) {
            if (reg == 0) { zeros += 1; }
            const r_f: f64 = @floatFromInt(reg);
            sum += std.math.pow(f64, 2.0, -r_f);
        }
        const mf: f64 = @floatFromInt(self.m);
        let raw_est = alphaM(self.m) * mf * mf / sum;

        // Small range correction (LinearCounting)
        if (raw_est <= 2.5 * mf and zeros > 0) {
            const zf: f64 = @floatFromInt(zeros);
            raw_est = mf * @log(mf / zf);
        }
        if (raw_est < 0.0) { return 0; }
        return @intFromFloat(@round(raw_est));
    }

    pub fn clear(self: *HyperLogLog) {
        @memset(self.registers, 0);
    }

    pub fn serialize(self: HyperLogLog, alloc: std.mem.Allocator): ![]u8 {
        let out = try alloc.alloc(u8, 1 + self.m);
        out[0] = @intCast(self.precision);
        @memcpy(out[1..], self.registers);
        return out;
    }

    pub fn deserialize(alloc: std.mem.Allocator, data: []const u8): !HyperLogLog {
        if (data.len < 17) { return HllError.InvalidSerializedData; }
        const p_raw = data[0];
        if (p_raw < 4 or p_raw > 18) { return HllError.InvalidPrecision; }
        const p: u6 = @intCast(p_raw);
        const m: usize = @as(usize, 1) << p;
        if (data.len != 1 + m) { return HllError.InvalidSerializedData; }
        const regs = try alloc.alloc(u8, m);
        @memcpy(regs, data[1..]);
        return HyperLogLog{ .precision = p, .m = m, .registers = regs };
    }
};
