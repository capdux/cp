// @cap/twox-hash — Pure CP zero-allocation XXHash32 & XXHash64
// Accelerated with 4-lane @Vector(4, u32) & @Vector(4, u64) SIMD stripe accumulators,
// hardware wrapping arithmetic (+%, -%, *%), zero-copy block processing, and bulk @memcpy.

import { readU32LE, readU64LE } from "cap:bin";

type XxhVec4U32 = @Vector(4, u32);
type XxhVec4U64 = @Vector(4, u64);

const XXH32_P1: u32 = 2654435761; // 0x9E3779B1
const XXH32_P2: u32 = 2246822519; // 0x85EBCA77
const XXH32_P3: u32 = 3266489917; // 0xC2B2AE3D
const XXH32_P4: u32 = 668265263;  // 0x27D4EB2F
const XXH32_P5: u32 = 374761393;  // 0x165667B1

const XXH64_P1: u64 = ((0x9E3779B1 as u64) << 32) | (0x85EBCA87 as u64);
const XXH64_P2: u64 = ((0xC2B2AE3D as u64) << 32) | (0x27D4EB4F as u64);
const XXH64_P3: u64 = ((0x165667B1 as u64) << 32) | (0x9E3779F9 as u64);
const XXH64_P4: u64 = ((0x85EBCA77 as u64) << 32) | (0xC2B2AE63 as u64);
const XXH64_P5: u64 = ((0x27D4EB2F as u64) << 32) | (0x165667C5 as u64);

fn xxhRol32(x: u32, r: u5): u32 {
    const rs: u5 = @intCast((32 as u32) - (r as u32));
    return (x << r) | (x >> rs);
}

fn xxhRol64(x: u64, r: u6): u64 {
    const rs: u6 = @intCast((64 as u32) - (r as u32));
    return (x << r) | (x >> rs);
}

fn xxhReadLe32Fast(b: []const u8, off: usize): u32 {
    return (b[off] as u32)
        | ((b[off + 1] as u32) << 8)
        | ((b[off + 2] as u32) << 16)
        | ((b[off + 3] as u32) << 24);
}

fn xxhReadLe64Fast(b: []const u8, off: usize): u64 {
    const lo: u64 = xxhReadLe32Fast(b, off) as u64;
    const hi: u64 = xxhReadLe32Fast(b, off + 4) as u64;
    return lo | (hi << 32);
}

fn xxh32StripeSimd(v: XxhVec4U32, b: []const u8, off: usize): XxhVec4U32 {
    const lanes: XxhVec4U32 = [
        xxhReadLe32Fast(b, off),
        xxhReadLe32Fast(b, off + 4),
        xxhReadLe32Fast(b, off + 8),
        xxhReadLe32Fast(b, off + 12),
    ];
    const vp1: XxhVec4U32 = @splat(XXH32_P1);
    const vp2: XxhVec4U32 = @splat(XXH32_P2);
    const a: XxhVec4U32 = v +% (lanes *% vp2);
    const sh13: @Vector(4, u5) = @splat(13 as u5);
    const sh19: @Vector(4, u5) = @splat(19 as u5);
    const rot: XxhVec4U32 = (a << sh13) | (a >> sh19);
    return rot *% vp1;
}

fn xxh64Round(acc: u64, lane: u64): u64 {
    const a: u64 = acc +% (lane *% XXH64_P2);
    return xxhRol64(a, 31) *% XXH64_P1;
}

fn xxh64StripeSimd(v: XxhVec4U64, b: []const u8, off: usize): XxhVec4U64 {
    const lanes: XxhVec4U64 = [
        xxhReadLe64Fast(b, off),
        xxhReadLe64Fast(b, off + 8),
        xxhReadLe64Fast(b, off + 16),
        xxhReadLe64Fast(b, off + 24),
    ];
    const vp1: XxhVec4U64 = @splat(XXH64_P1);
    const vp2: XxhVec4U64 = @splat(XXH64_P2);
    const a: XxhVec4U64 = v +% (lanes *% vp2);
    const sh31: @Vector(4, u6) = @splat(31 as u6);
    const sh33: @Vector(4, u6) = @splat(33 as u6);
    const rot: XxhVec4U64 = (a << sh31) | (a >> sh33);
    return rot *% vp1;
}

fn xxh64MergeRound(acc: u64, val: u64): u64 {
    const v: u64 = xxh64Round(0, val);
    const x: u64 = acc ^ v;
    return (x *% XXH64_P1) +% XXH64_P4;
}

pub struct XxHash32 {
    pub seed: u32,
    pub v1: u32,
    pub v2: u32,
    pub v3: u32,
    pub v4: u32,
    pub buf: [16]u8,
    pub buf_len: usize,
    pub total_len: u64,

    pub static fn withSeed(seed: u32): XxHash32 {
        let zbuf: [16]u8 = undefined;
        return XxHash32 {
            seed: seed,
            v1: seed +% XXH32_P1 +% XXH32_P2,
            v2: seed +% XXH32_P2,
            v3: seed,
            v4: seed -% XXH32_P1,
            buf: zbuf,
            buf_len: 0,
            total_len: 0,
        };
    }

    pub static fn oneshot(seed: u32, data: []u8): u32 {
        let h = XxHash32.withSeed(seed);
        h.write(data);
        return h.finish();
    }

    fn writeSlice(*_, data: []const u8) {
        if (data.len == 0) {
            return;
        }
        _.total_len += data.len as u64;
        let pos: usize = 0;

        if (_.buf_len > 0) {
            const need = 16 - _.buf_len;
            const take = if (data.len < need) data.len else need;
            @memcpy(_.buf[_.buf_len .. _.buf_len + take], data[0..take]);
            _.buf_len += take;
            pos += take;
            if (_.buf_len == 16) {
                const v_in: XxhVec4U32 = [_.v1, _.v2, _.v3, _.v4];
                const v_out: [4]u32 = xxh32StripeSimd(v_in, _.buf[0..16], 0);
                _.v1 = v_out[0];
                _.v2 = v_out[1];
                _.v3 = v_out[2];
                _.v4 = v_out[3];
                _.buf_len = 0;
            }
        }

        if (pos + 16 <= data.len) {
            let v: XxhVec4U32 = [_.v1, _.v2, _.v3, _.v4];
            while (pos + 16 <= data.len) {
                v = xxh32StripeSimd(v, data, pos);
                pos += 16;
            }
            const arr: [4]u32 = v;
            _.v1 = arr[0];
            _.v2 = arr[1];
            _.v3 = arr[2];
            _.v4 = arr[3];
        }

        const rem = data.len - pos;
        if (rem > 0) {
            @memcpy(_.buf[0..rem], data[pos..data.len]);
            _.buf_len = rem;
        }
    }

    pub fn write(*_, data: []u8) {
        _.writeSlice(data);
    }

    pub fn writeStr(*_, s: string) {
        _.writeSlice(s);
    }

    pub fn finish(): u32 {
        let h: u32 = 0;
        if (_.total_len >= 16) {
            h = xxhRol32(_.v1, 1) +% xxhRol32(_.v2, 7) +% xxhRol32(_.v3, 12) +% xxhRol32(_.v4, 18);
        } else {
            h = _.seed +% XXH32_P5;
        }
        h = h +% ((_.total_len & 0xFFFFFFFF) as u32);

        let pos: usize = 0;
        while (pos + 4 <= _.buf_len) {
            const lane: u32 = readU32LE(_.buf[0.._.buf_len], pos) ?? 0;
            h = h +% (lane *% XXH32_P3);
            h = xxhRol32(h, 17) *% XXH32_P4;
            pos += 4;
        }
        while (pos < _.buf_len) {
            h = h +% ((_.buf[pos] as u32) *% XXH32_P5);
            h = xxhRol32(h, 11) *% XXH32_P1;
            pos += 1;
        }

        h ^= h >> 15;
        h = h *% XXH32_P2;
        h ^= h >> 13;
        h = h *% XXH32_P3;
        h ^= h >> 16;
        return h;
    }
}

pub struct XxHash64 {
    pub seed: u64,
    pub v1: u64,
    pub v2: u64,
    pub v3: u64,
    pub v4: u64,
    pub buf: [32]u8,
    pub buf_len: usize,
    pub total_len: u64,

    pub static fn withSeed(seed: u64): XxHash64 {
        let zbuf: [32]u8 = undefined;
        return XxHash64 {
            seed: seed,
            v1: seed +% XXH64_P1 +% XXH64_P2,
            v2: seed +% XXH64_P2,
            v3: seed,
            v4: seed -% XXH64_P1,
            buf: zbuf,
            buf_len: 0,
            total_len: 0,
        };
    }

    pub static fn oneshot(seed: u64, data: []u8): u64 {
        let h = XxHash64.withSeed(seed);
        h.write(data);
        return h.finish();
    }

    fn writeSlice(*_, data: []const u8) {
        if (data.len == 0) {
            return;
        }
        _.total_len += data.len as u64;
        let pos: usize = 0;

        if (_.buf_len > 0) {
            const need = 32 - _.buf_len;
            const take = if (data.len < need) data.len else need;
            @memcpy(_.buf[_.buf_len .. _.buf_len + take], data[0..take]);
            _.buf_len += take;
            pos += take;
            if (_.buf_len == 32) {
                const v_in: XxhVec4U64 = [_.v1, _.v2, _.v3, _.v4];
                const v_out: [4]u64 = xxh64StripeSimd(v_in, _.buf[0..32], 0);
                _.v1 = v_out[0];
                _.v2 = v_out[1];
                _.v3 = v_out[2];
                _.v4 = v_out[3];
                _.buf_len = 0;
            }
        }

        if (pos + 32 <= data.len) {
            let v: XxhVec4U64 = [_.v1, _.v2, _.v3, _.v4];
            while (pos + 32 <= data.len) {
                v = xxh64StripeSimd(v, data, pos);
                pos += 32;
            }
            const arr: [4]u64 = v;
            _.v1 = arr[0];
            _.v2 = arr[1];
            _.v3 = arr[2];
            _.v4 = arr[3];
        }

        const rem = data.len - pos;
        if (rem > 0) {
            @memcpy(_.buf[0..rem], data[pos..data.len]);
            _.buf_len = rem;
        }
    }

    pub fn write(*_, data: []u8) {
        _.writeSlice(data);
    }

    pub fn writeStr(*_, s: string) {
        _.writeSlice(s);
    }

    pub fn finish(): u64 {
        let h: u64 = 0;
        if (_.total_len >= 32) {
            h = xxhRol64(_.v1, 1) +% xxhRol64(_.v2, 7) +% xxhRol64(_.v3, 12) +% xxhRol64(_.v4, 18);
            h = xxh64MergeRound(h, _.v1);
            h = xxh64MergeRound(h, _.v2);
            h = xxh64MergeRound(h, _.v3);
            h = xxh64MergeRound(h, _.v4);
        } else {
            h = _.seed +% XXH64_P5;
        }
        h = h +% _.total_len;

        let pos: usize = 0;
        while (pos + 8 <= _.buf_len) {
            const k1: u64 = xxh64Round(0, readU64LE(_.buf[0.._.buf_len], pos) ?? 0);
            h ^= k1;
            h = (xxhRol64(h, 27) *% XXH64_P1) +% XXH64_P4;
            pos += 8;
        }
        if (pos + 4 <= _.buf_len) {
            const lane32: u32 = readU32LE(_.buf[0.._.buf_len], pos) ?? 0;
            h ^= (lane32 as u64) *% XXH64_P1;
            h = (xxhRol64(h, 23) *% XXH64_P2) +% XXH64_P3;
            pos += 4;
        }
        while (pos < _.buf_len) {
            h ^= (_.buf[pos] as u64) *% XXH64_P5;
            h = xxhRol64(h, 11) *% XXH64_P1;
            pos += 1;
        }

        h ^= h >> 33;
        h = h *% XXH64_P2;
        h ^= h >> 29;
        h = h *% XXH64_P3;
        h ^= h >> 32;
        return h;
    }
}
