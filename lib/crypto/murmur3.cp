// Source: cp/lib/murmur3/murmur3.cp (Pure CP MurmurHash3 x86_32 & x64_128)
const std = @import("std");

const C1_32: u32 = 0xcc9e2d51;
const C2_32: u32 = 0x1b873593;

const C1_128: u64 = 0x87c37b91114253d5;
const C2_128: u64 = 0x4cf5ad432745937f;

fn fmix32(h_in: u32): u32 {
    let h = h_in;
    h ^= h >> 16;
    h = h *% 0x85ebca6b;
    h ^= h >> 13;
    h = h *% 0xc2b2ae35;
    h ^= h >> 16;
    return h;
}

fn fmix64(k_in: u64): u64 {
    let k = k_in;
    k ^= k >> 33;
    k = k *% 0xff51afd7ed558ccd;
    k ^= k >> 33;
    k = k *% 0xc4ceb9fe1a85ec53;
    k ^= k >> 33;
    return k;
}

pub fn murmur3_32(data: []const u8, seed: u32): u32 {
    let st = Murmur3_32Stream.init(seed);
    st.update(data);
    return st.final();
}

pub fn murmur3_128(data: []const u8, seed: u32): [16]u8 {
    let st = Murmur3_128Stream.init(seed);
    st.update(data);
    return st.final();
}

pub const Murmur3_32Stream = struct {
    h1: u32,
    total_len: usize,
    buf: [4]u8,
    buf_len: usize,

    pub fn init(seed: u32): Murmur3_32Stream {
        return Murmur3_32Stream{
            .h1 = seed,
            .total_len = 0,
            .buf = [ 0, 0, 0, 0 ],
            .buf_len = 0,
        };
    }

    fn mixBlock(*_, k_in: u32) {
        let k1 = k_in *% C1_32;
        k1 = std.math.rotl(u32, k1, 15);
        k1 = k1 *% C2_32;
        _.h1 ^= k1;
        _.h1 = std.math.rotl(u32, _.h1, 13);
        _.h1 = (_.h1 *% 5) +% 0xe6546b64;
    }

    pub fn update(*_, data: []const u8) {
        _.total_len = _.total_len +% data.len;
        let pos: usize = 0;
        if (_.buf_len > 0) {
            const need = 4 - _.buf_len;
            const take = @min(need, data.len);
            @memcpy(_.buf[_.buf_len .. _.buf_len + take], data[0..take]);
            _.buf_len += take;
            pos += take;
            if (_.buf_len == 4) {
                _.mixBlock(std.mem.readInt(u32, &_.buf, .little));
                _.buf_len = 0;
            }
        }
        while (pos + 4 <= data.len) : (pos += 4) {
            _.mixBlock(std.mem.readInt(u32, data[pos .. pos + 4], .little));
        }
        if (pos < data.len) {
            const rem = data.len - pos;
            @memcpy(_.buf[0..rem], data[pos..]);
            _.buf_len = rem;
        }
    }

    pub fn final(*_): u32 {
        if (_.buf_len > 0) {
            let k1: u32 = 0;
            if (_.buf_len >= 3) { k1 ^= (@as(u32, _.buf[2]) << 16); }
            if (_.buf_len >= 2) { k1 ^= (@as(u32, _.buf[1]) << 8); }
            k1 ^= @as(u32, _.buf[0]);
            k1 = k1 *% C1_32;
            k1 = std.math.rotl(u32, k1, 15);
            k1 = k1 *% C2_32;
            _.h1 ^= k1;
        }
        _.h1 ^= @as(u32, @truncate(_.total_len));
        return fmix32(_.h1);
    }
};

pub const Murmur3_128Stream = struct {
    h1: u64,
    h2: u64,
    total_len: usize,
    buf: [16]u8,
    buf_len: usize,

    pub fn init(seed: u32): Murmur3_128Stream {
        const s: u64 = seed;
        return Murmur3_128Stream{
            .h1 = s,
            .h2 = s,
            .total_len = 0,
            .buf = [ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ],
            .buf_len = 0,
        };
    }

    fn mixBlock(*_, k1_in: u64, k2_in: u64) {
        let k1 = k1_in *% C1_128;
        k1 = std.math.rotl(u64, k1, 31);
        k1 = k1 *% C2_128;
        _.h1 ^= k1;
        _.h1 = std.math.rotl(u64, _.h1, 27);
        _.h1 = _.h1 +% _.h2;
        _.h1 = (_.h1 *% 5) +% 0x52dce729;

        let k2 = k2_in *% C2_128;
        k2 = std.math.rotl(u64, k2, 33);
        k2 = k2 *% C1_128;
        _.h2 ^= k2;
        _.h2 = std.math.rotl(u64, _.h2, 31);
        _.h2 = _.h2 +% _.h1;
        _.h2 = (_.h2 *% 5) +% 0x38495ab5;
    }

    pub fn update(*_, data: []const u8) {
        _.total_len = _.total_len +% data.len;
        let pos: usize = 0;
        if (_.buf_len > 0) {
            const need = 16 - _.buf_len;
            const take = @min(need, data.len);
            @memcpy(_.buf[_.buf_len .. _.buf_len + take], data[0..take]);
            _.buf_len += take;
            pos += take;
            if (_.buf_len == 16) {
                const k1 = std.mem.readInt(u64, _.buf[0..8], .little);
                const k2 = std.mem.readInt(u64, _.buf[8..16], .little);
                _.mixBlock(k1, k2);
                _.buf_len = 0;
            }
        }
        while (pos + 16 <= data.len) : (pos += 16) {
            const k1 = std.mem.readInt(u64, data[pos .. pos + 8], .little);
            const k2 = std.mem.readInt(u64, data[pos + 8 .. pos + 16], .little);
            _.mixBlock(k1, k2);
        }
        if (pos < data.len) {
            const rem = data.len - pos;
            @memcpy(_.buf[0..rem], data[pos..]);
            _.buf_len = rem;
        }
    }

    pub fn final(*_): [16]u8 {
        if (_.buf_len > 0) {
            let k1: u64 = 0;
            let k2: u64 = 0;
            if (_.buf_len > 8) {
                let i: usize = _.buf_len - 1;
                while (i >= 8) : (i -= 1) {
                    const sh: u6 = @intCast((i - 8) * 8);
                    k2 ^= (@as(u64, _.buf[i]) << sh);
                    if (i == 8) { break; }
                }
                k2 = k2 *% C2_128;
                k2 = std.math.rotl(u64, k2, 33);
                k2 = k2 *% C1_128;
                _.h2 ^= k2;
            }
            const lim = @min(_.buf_len, 8);
            let j: usize = 0;
            while (j < lim) : (j += 1) {
                const sh: u6 = @intCast(j * 8);
                k1 ^= (@as(u64, _.buf[j]) << sh);
            }
            k1 = k1 *% C1_128;
            k1 = std.math.rotl(u64, k1, 31);
            k1 = k1 *% C2_128;
            _.h1 ^= k1;
        }

        const len64: u64 = @intCast(_.total_len);
        _.h1 ^= len64;
        _.h2 ^= len64;
        _.h1 = _.h1 +% _.h2;
        _.h2 = _.h2 +% _.h1;
        _.h1 = fmix64(_.h1);
        _.h2 = fmix64(_.h2);
        _.h1 = _.h1 +% _.h2;
        _.h2 = _.h2 +% _.h1;

        let out: [16]u8 = undefined;
        std.mem.writeInt(u64, out[0..8], _.h1, .little);
        std.mem.writeInt(u64, out[8..16], _.h2, .little);
        return out;
    }
};
