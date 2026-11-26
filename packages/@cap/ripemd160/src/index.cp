pub const RIPEMD160_DIGEST_LEN: usize = 20;
pub const RIPEMD160_BLOCK_LEN: usize = 64;

fn rmdAdd(a: u32, b: u32): u32 {
    return (((a as u64) + (b as u64)) & 4294967295) as u32;
}

fn rmdAdd3(a: u32, b: u32, c: u32): u32 {
    return (((a as u64) + (b as u64) + (c as u64)) & 4294967295) as u32;
}

fn rmdAdd4(a: u32, b: u32, c: u32, d: u32): u32 {
    return (((a as u64) + (b as u64) + (c as u64) + (d as u64)) & 4294967295) as u32;
}

fn rmdRol(x: u32, n: u32): u32 {
    return (x << n) | (x >> (32 - n));
}

fn rmdNot(x: u32): u32 {
    return x ^ 4294967295;
}

fn rmdFunc(round: usize, x: u32, y: u32, z: u32): u32 {
    if (round < 16) {
        return x ^ y ^ z;
    }
    if (round < 32) {
        return (x & y) | (rmdNot(x) & z);
    }
    if (round < 48) {
        return (x | rmdNot(y)) ^ z;
    }
    if (round < 64) {
        return (x & z) | (y & rmdNot(z));
    }
    return x ^ (y | rmdNot(z));
}

fn rmdKl(round: usize): u32 {
    if (round < 16) { return 0; }
    if (round < 32) { return 1518500249; } // 0x5A827999
    if (round < 48) { return 1859775393; } // 0x6ED9EBA1
    if (round < 64) { return 2400959708; } // 0x8F1BBCDC
    return 2840853838;                     // 0xA953FD4E
}

fn rmdKr(round: usize): u32 {
    if (round < 16) { return 1352829926; } // 0x50A28BE6
    if (round < 32) { return 1548603684; } // 0x5C4DD124
    if (round < 48) { return 1836072691; } // 0x6D703EF3
    if (round < 64) { return 2053994217; } // 0x7A6D76E9
    return 0;
}

fn rmdReadU32Le(buf: []u8, off: usize): u32 {
    return (buf[off] as u32)
        | ((buf[off + 1] as u32) << 8)
        | ((buf[off + 2] as u32) << 16)
        | ((buf[off + 3] as u32) << 24);
}

fn rmdWriteU32Le(v: u32, out: []u8, off: usize) {
    out[off] = (v & 255) as u8;
    out[off + 1] = ((v >> 8) & 255) as u8;
    out[off + 2] = ((v >> 16) & 255) as u8;
    out[off + 3] = ((v >> 24) & 255) as u8;
}

fn rmdHexNibble(v: u8): u8 {
    if (v < 10) {
        return 48 + v;
    }
    return 87 + v; // 'a' (97) - 10 = 87
}

fn rmdCompress(state: [5]u32, block: []u8): [5]u32 {
    let rl: [80]usize = [
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
        7, 4, 13, 1, 10, 6, 15, 3, 12, 0, 9, 5, 2, 14, 11, 8,
        3, 10, 14, 4, 9, 15, 8, 1, 2, 7, 0, 6, 13, 11, 5, 12,
        1, 9, 11, 10, 0, 8, 12, 4, 13, 3, 7, 15, 14, 5, 6, 2,
        4, 0, 5, 9, 7, 12, 2, 10, 14, 1, 3, 8, 11, 6, 15, 13
    ];
    let rr: [80]usize = [
        5, 14, 7, 0, 9, 2, 11, 4, 13, 6, 15, 8, 1, 10, 3, 12,
        6, 11, 3, 7, 0, 13, 5, 10, 14, 15, 8, 12, 4, 9, 1, 2,
        15, 5, 1, 3, 7, 14, 6, 9, 11, 8, 12, 2, 10, 0, 4, 13,
        8, 6, 4, 1, 3, 11, 15, 0, 5, 12, 2, 13, 9, 7, 10, 14,
        12, 15, 10, 4, 1, 5, 8, 7, 6, 2, 13, 14, 0, 3, 9, 11
    ];
    let sl: [80]u32 = [
        11, 14, 15, 12, 5, 8, 7, 9, 11, 13, 14, 15, 6, 7, 9, 8,
        7, 6, 8, 13, 11, 9, 7, 15, 7, 12, 15, 9, 11, 7, 13, 12,
        11, 13, 6, 7, 14, 9, 13, 15, 14, 8, 13, 6, 5, 12, 7, 5,
        11, 12, 14, 15, 14, 15, 9, 8, 9, 14, 5, 6, 8, 6, 5, 12,
        9, 15, 5, 11, 6, 8, 13, 12, 5, 12, 13, 14, 11, 8, 5, 6
    ];
    let sr: [80]u32 = [
        8, 9, 9, 11, 13, 15, 15, 5, 7, 7, 8, 11, 14, 14, 12, 6,
        9, 13, 15, 7, 12, 8, 9, 11, 7, 7, 12, 7, 6, 15, 13, 11,
        9, 7, 15, 11, 8, 6, 6, 14, 12, 13, 5, 14, 13, 13, 7, 5,
        15, 5, 8, 11, 14, 14, 6, 14, 6, 9, 12, 9, 12, 5, 15, 8,
        8, 5, 12, 9, 12, 5, 14, 6, 8, 13, 6, 5, 15, 13, 11, 11
    ];

    let x: [16]u32 = [0; 16];
    let wi: usize = 0;
    while (wi < 16) {
        x[wi] = rmdReadU32Le(block, wi * 4);
        wi = wi + 1;
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
    while (j < 80) {
        let fl: u32 = rmdFunc(j, bl, cl, dl);
        let tl: u32 = rmdAdd(rmdRol(rmdAdd4(al, fl, x[rl[j]], rmdKl(j)), sl[j]), el);
        al = el;
        el = dl;
        dl = rmdRol(cl, 10);
        cl = bl;
        bl = tl;

        let fr: u32 = rmdFunc(79 - j, br, cr, dr);
        let tr: u32 = rmdAdd(rmdRol(rmdAdd4(ar, fr, x[rr[j]], rmdKr(j)), sr[j]), er);
        ar = er;
        er = dr;
        dr = rmdRol(cr, 10);
        cr = br;
        br = tr;

        j = j + 1;
    }

    let t: u32 = rmdAdd3(state[1], cl, dr);
    let next_state: [5]u32 = [
        t,
        rmdAdd3(state[2], dl, er),
        rmdAdd3(state[3], el, ar),
        rmdAdd3(state[4], al, br),
        rmdAdd3(state[0], bl, cr)
    ];
    return next_state;
}

pub struct Ripemd160 {
    pub state: [5]u32,
    pub buf: [64]u8,
    pub buf_len: usize,
    pub total_len: u64,

    pub static fn new(): Ripemd160 {
        let init_state: [5]u32 = [1732584193, 4023233417, 2562383102, 271733878, 3285377520];
        let zero_buf: [64]u8 = [0; 64];
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
        let i: usize = 0;
        while (i < 64) {
            _.buf[i] = 0;
            i = i + 1;
        }
        _.buf_len = 0;
        _.total_len = 0;
    }

    pub fn update(*_, data: []u8) {
        let i: usize = 0;
        while (i < data.len) {
            _.buf[_.buf_len] = data[i];
            _.buf_len = _.buf_len + 1;
            _.total_len = _.total_len + 1;
            if (_.buf_len == 64) {
                _.state = rmdCompress(_.state, _.buf[0..64]);
                _.buf_len = 0;
            }
            i = i + 1;
        }
    }

    pub fn updateStr(*_, s: string) {
        let i: usize = 0;
        while (i < s.len) {
            _.buf[_.buf_len] = s[i];
            _.buf_len = _.buf_len + 1;
            _.total_len = _.total_len + 1;
            if (_.buf_len == 64) {
                _.state = rmdCompress(_.state, _.buf[0..64]);
                _.buf_len = 0;
            }
            i = i + 1;
        }
    }

    pub fn finalize(*_, out20: []u8) {
        if (out20.len < RIPEMD160_DIGEST_LEN) {
            return;
        }
        let st: [5]u32 = [_.state[0], _.state[1], _.state[2], _.state[3], _.state[4]];
        let pad: [64]u8 = [0; 64];
        let pi: usize = 0;
        while (pi < _.buf_len) {
            pad[pi] = _.buf[pi];
            pi = pi + 1;
        }
        let pos: usize = _.buf_len;
        pad[pos] = 128;
        pos = pos + 1;
        if (pos > 56) {
            while (pos < 64) {
                pad[pos] = 0;
                pos = pos + 1;
            }
            st = rmdCompress(st, pad[0..64]);
            pos = 0;
        }
        while (pos < 56) {
            pad[pos] = 0;
            pos = pos + 1;
        }
        let bit_len: u64 = _.total_len << 3;
        let bi: usize = 0;
        while (bi < 8) {
            pad[56 + bi] = ((bit_len >> ((bi * 8) as u64)) & 255) as u8;
            bi = bi + 1;
        }
        st = rmdCompress(st, pad[0..64]);

        let oi: usize = 0;
        while (oi < 5) {
            rmdWriteU32Le(st[oi], out20, oi * 4);
            oi = oi + 1;
        }
    }

    pub fn finalizeHex(*_, out40: []u8): usize {
        if (out40.len < 40) {
            return 0;
        }
        let digest: [20]u8 = [0; 20];
        _.finalize(digest[0..20]);
        let i: usize = 0;
        while (i < 20) {
            let b: u8 = digest[i];
            out40[i * 2] = rmdHexNibble((b >> 4) & 15);
            out40[i * 2 + 1] = rmdHexNibble(b & 15);
            i = i + 1;
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
