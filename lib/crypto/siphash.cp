// Source: cp/lib/siphash/siphash.cp (Pure CP SipHash-2-4 & SipHash-1-3 64/128-bit PRF)
const std = @import("std");

pub const SIPHASH_KEY_LEN: usize = 16;

const V0_INIT: u64 = 0x736f6d6570736575;
const V1_INIT: u64 = 0x646f72616e646f6d;
const V2_INIT: u64 = 0x6c7967656e657261;
const V3_INIT: u64 = 0x7465646279746573;

fn rotl64(x: u64, b: u6): u64 {
    return std.math.rotl(u64, x, b);
}

fn sipRound(v0: *u64, v1: *u64, v2: *u64, v3: *u64) {
    v0.* = v0.* +% v1.*;
    v1.* = rotl64(v1.*, 13);
    v1.* ^= v0.*;
    v0.* = rotl64(v0.*, 32);

    v2.* = v2.* +% v3.*;
    v3.* = rotl64(v3.*, 16);
    v3.* ^= v2.*;

    v0.* = v0.* +% v3.*;
    v3.* = rotl64(v3.*, 21);
    v3.* ^= v0.*;

    v2.* = v2.* +% v1.*;
    v1.* = rotl64(v1.*, 17);
    v1.* ^= v2.*;
    v2.* = rotl64(v2.*, 32);
}

pub const SipHashStream = struct {
    v0: u64,
    v1: u64,
    v2: u64,
    v3: u64,
    c_rounds: u8,
    d_rounds: u8,
    is_128: bool,
    total_len: usize,
    buf: [8]u8,
    buf_len: usize,

    pub fn init(key: [16]u8, c_rounds: u8, d_rounds: u8, is_128: bool): SipHashStream {
        const k0 = std.mem.readInt(u64, key[0..8], .little);
        const k1 = std.mem.readInt(u64, key[8..16], .little);
        let v2 = V2_INIT ^ k0;
        if (is_128) {
            v2 ^= 0xee;
        }
        return SipHashStream{
            .v0 = V0_INIT ^ k0,
            .v1 = V1_INIT ^ k1,
            .v2 = v2,
            .v3 = V3_INIT ^ k1,
            .c_rounds = c_rounds,
            .d_rounds = d_rounds,
            .is_128 = is_128,
            .total_len = 0,
            .buf = [ 0, 0, 0, 0, 0, 0, 0, 0 ],
            .buf_len = 0,
        };
    }

    pub fn init24(key: [16]u8): SipHashStream {
        return SipHashStream.init(key, 2, 4, false);
    }

    pub fn init13(key: [16]u8): SipHashStream {
        return SipHashStream.init(key, 1, 3, false);
    }

    pub fn init24_128(key: [16]u8): SipHashStream {
        return SipHashStream.init(key, 2, 4, true);
    }

    fn compressBlock(*_, m: u64) {
        _.v3 ^= m;
        let i: u8 = 0;
        while (i < _.c_rounds) : (i += 1) {
            sipRound(&_.v0, &_.v1, &_.v2, &_.v3);
        }
        _.v0 ^= m;
    }

    pub fn update(*_, data: []const u8) {
        _.total_len = _.total_len +% data.len;
        let pos: usize = 0;
        if (_.buf_len > 0) {
            const need = 8 - _.buf_len;
            const take = @min(need, data.len);
            @memcpy(_.buf[_.buf_len .. _.buf_len + take], data[0..take]);
            _.buf_len += take;
            pos += take;
            if (_.buf_len == 8) {
                const m = std.mem.readInt(u64, &_.buf, .little);
                _.compressBlock(m);
                _.buf_len = 0;
            }
        }
        while (pos + 8 <= data.len) : (pos += 8) {
            const m = std.mem.readInt(u64, data[pos .. pos + 8], .little);
            _.compressBlock(m);
        }
        if (pos < data.len) {
            const rem = data.len - pos;
            @memcpy(_.buf[0..rem], data[pos..]);
            _.buf_len = rem;
        }
    }

    fn finalizeCommon(*_) {
        let last: [8]u8 = [ 0, 0, 0, 0, 0, 0, 0, 0 ];
        if (_.buf_len > 0) {
            @memcpy(last[0.._.buf_len], _.buf[0.._.buf_len]);
        }
        last[7] = @truncate(_.total_len & 0xFF);
        const m = std.mem.readInt(u64, &last, .little);
        _.compressBlock(m);
    }

    pub fn final64(*_): u64 {
        _.finalizeCommon();
        _.v2 ^= 0xff;
        let i: u8 = 0;
        while (i < _.d_rounds) : (i += 1) {
            sipRound(&_.v0, &_.v1, &_.v2, &_.v3);
        }
        return _.v0 ^ _.v1 ^ _.v2 ^ _.v3;
    }

    pub fn final128(*_): [16]u8 {
        _.finalizeCommon();
        _.v2 ^= 0xee;
        let i: u8 = 0;
        while (i < _.d_rounds) : (i += 1) {
            sipRound(&_.v0, &_.v1, &_.v2, &_.v3);
        }
        const lo = _.v0 ^ _.v1 ^ _.v2 ^ _.v3;
        _.v1 ^= 0xdd;
        let j: u8 = 0;
        while (j < _.d_rounds) : (j += 1) {
            sipRound(&_.v0, &_.v1, &_.v2, &_.v3);
        }
        const hi = _.v0 ^ _.v1 ^ _.v2 ^ _.v3;
        let out: [16]u8 = undefined;
        std.mem.writeInt(u64, out[0..8], lo, .little);
        std.mem.writeInt(u64, out[8..16], hi, .little);
        return out;
    }
};

pub fn siphash24(key: [16]u8, data: []const u8): u64 {
    let st = SipHashStream.init24(key);
    st.update(data);
    return st.final64();
}

pub fn siphash13(key: [16]u8, data: []const u8): u64 {
    let st = SipHashStream.init13(key);
    st.update(data);
    return st.final64();
}

pub fn siphash24_128(key: [16]u8, data: []const u8): [16]u8 {
    let st = SipHashStream.init24_128(key);
    st.update(data);
    return st.final128();
}

pub fn verifySipHash24(key: [16]u8, data: []const u8, expected: u64): bool {
    const actual = siphash24(key, data);
    let a: [8]u8 = undefined;
    let b: [8]u8 = undefined;
    std.mem.writeInt(u64, &a, actual, .little);
    std.mem.writeInt(u64, &b, expected, .little);
    return std.crypto.timing_safe.eql([8]u8, a, b);
}
