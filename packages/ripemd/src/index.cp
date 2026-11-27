// @cap/ripemd — Pure CP zero-allocation RIPEMD-160 cryptographic hash
// Accelerated with comptime constant tables, hardware wrapping arithmetic,
// zero-copy 64-byte block compression, bulk @memcpy/@memset, and 16-lane @Vector(16, u8) SIMD hex encoding.

import { hexCharLower } from "cap:ascii";
import { writeU32LE } from "cap:bin";

type RmdVec16 = @Vector(16, u8);

pub const RIPEMD160_DIGEST_LEN: usize = 20;
pub const RIPEMD160_BLOCK_LEN: usize = 64;

const RMD_RL: [80]usize = [
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
    7, 4, 13, 1, 10, 6, 15, 3, 12, 0, 9, 5, 2, 14, 11, 8,
    3, 10, 14, 4, 9, 15, 8, 1, 2, 7, 0, 6, 13, 11, 5, 12,
    1, 9, 11, 10, 0, 8, 12, 4, 13, 3, 7, 15, 14, 5, 6, 2,
    4, 0, 5, 9, 7, 12, 2, 10, 14, 1, 3, 8, 11, 6, 15, 13
];

const RMD_RR: [80]usize = [
    5, 14, 7, 0, 9, 2, 11, 4, 13, 6, 15, 8, 1, 10, 3, 12,
    6, 11, 3, 7, 0, 13, 5, 10, 14, 15, 8, 12, 4, 9, 1, 2,
    15, 5, 1, 3, 7, 14, 6, 9, 11, 8, 12, 2, 10, 0, 4, 13,
    8, 6, 4, 1, 3, 11, 15, 0, 5, 12, 2, 13, 9, 7, 10, 14,
    12, 15, 10, 4, 1, 5, 8, 7, 6, 2, 13, 14, 0, 3, 9, 11
];

const RMD_SL: [80]u5 = [
    11, 14, 15, 12, 5, 8, 7, 9, 11, 13, 14, 15, 6, 7, 9, 8,
    7, 6, 8, 13, 11, 9, 7, 15, 7, 12, 15, 9, 11, 7, 13, 12,
    11, 13, 6, 7, 14, 9, 13, 15, 14, 8, 13, 6, 5, 12, 7, 5,
    11, 12, 14, 15, 14, 15, 9, 8, 9, 14, 5, 6, 8, 6, 5, 12,
    9, 15, 5, 11, 6, 8, 13, 12, 5, 12, 13, 14, 11, 8, 5, 6
];

const RMD_SR: [80]u5 = [
    8, 9, 9, 11, 13, 15, 15, 5, 7, 7, 8, 11, 14, 14, 12, 6,
    9, 13, 15, 7, 12, 8, 9, 11, 7, 7, 12, 7, 6, 15, 13, 11,
    9, 7, 15, 11, 8, 6, 6, 14, 12, 13, 5, 14, 13, 13, 7, 5,
    15, 5, 8, 11, 14, 14, 6, 14, 6, 9, 12, 9, 12, 5, 15, 8,
    8, 5, 12, 9, 12, 5, 14, 6, 8, 13, 6, 5, 15, 13, 11, 11
];

fn rmdRol(x: u32, n: u5): u32 {
    const rshift: u5 = @intCast((32 as u32) - (n as u32));
    return (x << n) | (x >> rshift);
}

fn rmdReadLe32(b: []const u8, off: usize): u32 {
    return (b[off] as u32)
        | ((b[off + 1] as u32) << 8)
        | ((b[off + 2] as u32) << 16)
        | ((b[off + 3] as u32) << 24);
}

fn rmdCompress(state: [5]u32, block: []const u8, off: usize): [5]u32 {
    let x: [16]u32 = undefined;
    let wi: usize = 0;
    while (wi < 16) {
        x[wi] = rmdReadLe32(block, off + wi * 4);
        wi += 1;
    }

    let al: u32 = state[0];
    let bl: u32 = state[1];
    let cl: u32 = state[2];
    let dl: u32 = state[3];
    let el: u32 = state[4];

    let ar: u32 = state[0];
    let br: u32 = state[1];
    let cr: u32 = state[2];
    let dr: u32 = state[3];
    let er: u32 = state[4];

    let j: usize = 0;
    while (j < 16) {
        const fl: u32 = bl ^ cl ^ dl;
        const tl: u32 = rmdRol(al +% fl +% x[RMD_RL[j]], RMD_SL[j]) +% el;
        al = el; el = dl; dl = (cl << 10) | (cl >> 22); cl = bl; bl = tl;

        const fr: u32 = br ^ (cr | (~dr));
        const tr: u32 = rmdRol(ar +% fr +% x[RMD_RR[j]] +% (0x50A28BE6 as u32), RMD_SR[j]) +% er;
        ar = er; er = dr; dr = (cr << 10) | (cr >> 22); cr = br; br = tr;
        j += 1;
    }
    while (j < 32) {
        const fl: u32 = (bl & cl) | ((~bl) & dl);
        const tl: u32 = rmdRol(al +% fl +% x[RMD_RL[j]] +% (0x5A827999 as u32), RMD_SL[j]) +% el;
        al = el; el = dl; dl = (cl << 10) | (cl >> 22); cl = bl; bl = tl;

        const fr: u32 = (br & dr) | (cr & (~dr));
        const tr: u32 = rmdRol(ar +% fr +% x[RMD_RR[j]] +% (0x5C4DD124 as u32), RMD_SR[j]) +% er;
        ar = er; er = dr; dr = (cr << 10) | (cr >> 22); cr = br; br = tr;
        j += 1;
    }
    while (j < 48) {
        const fl: u32 = (bl | (~cl)) ^ dl;
        const tl: u32 = rmdRol(al +% fl +% x[RMD_RL[j]] +% (0x6ED9EBA1 as u32), RMD_SL[j]) +% el;
        al = el; el = dl; dl = (cl << 10) | (cl >> 22); cl = bl; bl = tl;

        const fr: u32 = (br | (~cr)) ^ dr;
        const tr: u32 = rmdRol(ar +% fr +% x[RMD_RR[j]] +% (0x6D703EF3 as u32), RMD_SR[j]) +% er;
        ar = er; er = dr; dr = (cr << 10) | (cr >> 22); cr = br; br = tr;
        j += 1;
    }
    while (j < 64) {
        const fl: u32 = (bl & dl) | (cl & (~dl));
        const tl: u32 = rmdRol(al +% fl +% x[RMD_RL[j]] +% (0x8F1BBCDC as u32), RMD_SL[j]) +% el;
        al = el; el = dl; dl = (cl << 10) | (cl >> 22); cl = bl; bl = tl;

        const fr: u32 = (br & cr) | ((~br) & dr);
        const tr: u32 = rmdRol(ar +% fr +% x[RMD_RR[j]] +% (0x7A6D76E9 as u32), RMD_SR[j]) +% er;
        ar = er; er = dr; dr = (cr << 10) | (cr >> 22); cr = br; br = tr;
        j += 1;
    }
    while (j < 80) {
        const fl: u32 = bl ^ (cl | (~dl));
        const tl: u32 = rmdRol(al +% fl +% x[RMD_RL[j]] +% (0xA953FD4E as u32), RMD_SL[j]) +% el;
        al = el; el = dl; dl = (cl << 10) | (cl >> 22); cl = bl; bl = tl;

        const fr: u32 = br ^ cr ^ dr;
        const tr: u32 = rmdRol(ar +% fr +% x[RMD_RR[j]], RMD_SR[j]) +% er;
        ar = er; er = dr; dr = (cr << 10) | (cr >> 22); cr = br; br = tr;
        j += 1;
    }

    const next_state: [5]u32 = [
        state[1] +% cl +% dr,
        state[2] +% dl +% er,
        state[3] +% el +% ar,
        state[4] +% al +% br,
        state[0] +% bl +% cr
    ];
    return next_state;
}

pub struct Ripemd160 {
    pub state: [5]u32,
    pub buf: [64]u8,
    pub buf_len: usize,
    pub total_len: u64,

    pub static fn new(): Ripemd160 {
        const init_state: [5]u32 = [1732584193, 4023233417, 2562383102, 271733878, 3285377520];
        let zero_buf: [64]u8 = undefined;
        return Ripemd160 {
            state: init_state,
            buf: zero_buf,
            buf_len: 0,
            total_len: 0,
        };
    }

    pub fn reset(*_) {
        _.state[0] = 1732584193;
        _.state[1] = 4023233417;
        _.state[2] = 2562383102;
        _.state[3] = 271733878;
        _.state[4] = 3285377520;
        _.buf_len = 0;
        _.total_len = 0;
    }

    fn updateSlice(*_, data: []const u8) {
        if (data.len == 0) {
            return;
        }
        _.total_len += data.len as u64;
        let pos: usize = 0;

        if (_.buf_len > 0) {
            const need = 64 - _.buf_len;
            const take = if (data.len < need) data.len else need;
            @memcpy(_.buf[_.buf_len .. _.buf_len + take], data[0..take]);
            _.buf_len += take;
            pos += take;
            if (_.buf_len == 64) {
                _.state = rmdCompress(_.state, _.buf[0..64], 0);
                _.buf_len = 0;
            }
        }

        while (pos + 64 <= data.len) {
            _.state = rmdCompress(_.state, data, pos);
            pos += 64;
        }

        const rem = data.len - pos;
        if (rem > 0) {
            @memcpy(_.buf[0..rem], data[pos..data.len]);
            _.buf_len = rem;
        }
    }

    pub fn update(*_, data: []u8) {
        _.updateSlice(data);
    }

    pub fn updateStr(*_, s: string) {
        _.updateSlice(s);
    }

    pub fn finalize(*_, out20: []u8) {
        if (out20.len < RIPEMD160_DIGEST_LEN) {
            return;
        }
        let st: [5]u32 = [_.state[0], _.state[1], _.state[2], _.state[3], _.state[4]];
        let pad: [64]u8 = undefined;
        if (_.buf_len > 0) {
            @memcpy(pad[0.._.buf_len], _.buf[0.._.buf_len]);
        }
        let pos: usize = _.buf_len;
        pad[pos] = 128;
        pos += 1;
        if (pos > 56) {
            if (pos < 64) {
                @memset(pad[pos..64], 0);
            }
            st = rmdCompress(st, pad[0..64], 0);
            pos = 0;
        }
        if (pos < 56) {
            @memset(pad[pos..56], 0);
        }
        const bit_len: u64 = _.total_len << 3;
        pad[56] = (bit_len & 255) as u8;
        pad[57] = ((bit_len >> 8) & 255) as u8;
        pad[58] = ((bit_len >> 16) & 255) as u8;
        pad[59] = ((bit_len >> 24) & 255) as u8;
        pad[60] = ((bit_len >> 32) & 255) as u8;
        pad[61] = ((bit_len >> 40) & 255) as u8;
        pad[62] = ((bit_len >> 48) & 255) as u8;
        pad[63] = ((bit_len >> 56) & 255) as u8;
        st = rmdCompress(st, pad[0..64], 0);

        let oi: usize = 0;
        while (oi < 5) {
            writeU32LE(out20, oi * 4, st[oi]);
            oi += 1;
        }
    }

    pub fn finalizeHex(*_, out40: []u8): usize {
        if (out40.len < 40) {
            return 0;
        }
        let digest: [20]u8 = undefined;
        _.finalize(digest[0..20]);

        // 16-lane SIMD hex encode for first 16 bytes
        const v: RmdVec16 = digest[0..16].*;
        const v_zero: RmdVec16 = @splat(48 as u8);
        const v_ten: RmdVec16 = @splat(10 as u8);
        const v_alpha: RmdVec16 = @splat(39 as u8);
        const v_nul: RmdVec16 = @splat(0 as u8);
        const v_mask: RmdVec16 = @splat(0x0F as u8);
        const v_sh4: @Vector(16, u3) = @splat(4 as u3);
        const hi_n: RmdVec16 = v >> v_sh4;
        const lo_n: RmdVec16 = v & v_mask;
        const hi_c: [16]u8 = hi_n + v_zero + @select(u8, hi_n >= v_ten, v_alpha, v_nul);
        const lo_c: [16]u8 = lo_n + v_zero + @select(u8, lo_n >= v_ten, v_alpha, v_nul);

        let i: usize = 0;
        while (i < 16) {
            out40[i * 2] = hi_c[i];
            out40[i * 2 + 1] = lo_c[i];
            i += 1;
        }
        while (i < 20) {
            const b: u8 = digest[i];
            out40[i * 2] = hexCharLower((b >> 4) & 15);
            out40[i * 2 + 1] = hexCharLower(b & 15);
            i += 1;
        }
        return 40;
    }
}

pub fn ripemd160Bytes(data: []u8, out20: []u8) {
    let h = Ripemd160.new();
    h.update(data);
    h.finalize(out20);
}

pub fn ripemd160Hex(s: string, out40: []u8): usize {
    let h = Ripemd160.new();
    h.updateStr(s);
    return h.finalizeHex(out40);
}
