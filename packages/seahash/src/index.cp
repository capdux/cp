import { readU64LE, writeU64LE } from "cap:bin";

fn seaU64(hi: u32, lo: u32): u64 {
    return ((hi as u64) << 32) | (lo as u64);
}

fn seaMul64(a: u64, b: u64): u64 {
    let al: u64 = a & 0xFFFFFFFF;
    let ah: u64 = a >> 32;
    let bl: u64 = b & 0xFFFFFFFF;
    let bh: u64 = b >> 32;
    let p0: u64 = al * bl;
    let p1: u64 = (al * bh) & 0xFFFFFFFF;
    let p2: u64 = (ah * bl) & 0xFFFFFFFF;
    let hi: u64 = ((p0 >> 32) + p1 + p2) & 0xFFFFFFFF;
    return (hi << 32) | (p0 & 0xFFFFFFFF);
}

pub fn seahashDiffuse(x_in: u64): u64 {
    let m: u64 = seaU64(0x6eed0e9d, 0xa4d94a4f);
    let x: u64 = seaMul64(x_in, m);
    let a: u64 = x >> 32;
    let b: u64 = x >> 60;
    x = x ^ (a >> b);
    x = seaMul64(x, m);
    return x;
}

pub struct SeaHasher {
    pub a: u64,
    pub b: u64,
    pub c: u64,
    pub d: u64,
    pub buf: [8]u8,
    pub buf_len: usize,
    pub written: u64,

    pub static fn new(): SeaHasher {
        return SeaHasher.withSeeds(
            seaU64(0x16f11fe8, 0x9b0d677c),
            seaU64(0xb480a793, 0xd8e6c86c),
            seaU64(0x6fe2e5aa, 0xf078ebc9),
            seaU64(0x14f994a4, 0xc5259381)
        );
    }

    pub static fn withSeeds(k1: u64, k2: u64, k3: u64, k4: u64): SeaHasher {
        let zbuf: [8]u8 = undefined;
        return SeaHasher {
            a: k1,
            b: k2,
            c: k3,
            d: k4,
            buf: zbuf,
            buf_len: 0,
            written: 0,
        };
    }

    pub fn writeU64(*_, word: u64) {
        if (_.buf_len == 0) {
            let next_a: u64 = seahashDiffuse(_.a ^ word);
            _.a = _.b;
            _.b = _.c;
            _.c = _.d;
            _.d = next_a;
            _.written = _.written + 8;
            return;
        }
        let tmp: [8]u8 = undefined;
        * = writeU64LE(tmp[0..8], 0, word);
        _.writeBytes(tmp[0..8]);
    }

    pub fn writeBytes(*_, buf: []u8) {
        let i: usize = 0;
        while (i < buf.len) {
            _.buf[_.buf_len] = buf[i];
            _.buf_len = _.buf_len + 1;
            _.written = _.written + 1;
            if (_.buf_len == 8) {
                let w: u64 = readU64LE(_.buf[0..8], 0) ?? 0;
                let next_a: u64 = seahashDiffuse(_.a ^ w);
                _.a = _.b;
                _.b = _.c;
                _.c = _.d;
                _.d = next_a;
                _.buf_len = 0;
            }
            i = i + 1;
        }
    }

    pub fn writeStr(*_, s: string) {
        let i: usize = 0;
        while (i < s.len) {
            _.buf[_.buf_len] = s[i];
            _.buf_len = _.buf_len + 1;
            _.written = _.written + 1;
            if (_.buf_len == 8) {
                let w: u64 = readU64LE(_.buf[0..8], 0) ?? 0;
                let next_a: u64 = seahashDiffuse(_.a ^ w);
                _.a = _.b;
                _.b = _.c;
                _.c = _.d;
                _.d = next_a;
                _.buf_len = 0;
            }
            i = i + 1;
        }
    }

    pub fn finish(): u64 {
        let fa: u64 = _.a;
        let fb: u64 = _.b;
        let fc: u64 = _.c;
        let fd: u64 = _.d;
        if (_.buf_len > 0) {
            let tail: u64 = 0;
            let ti: usize = 0;
            while (ti < _.buf_len) {
                tail = tail | ((_.buf[ti] as u64) << ((ti * 8) as u64));
                ti = ti + 1;
            }
            let next_a: u64 = seahashDiffuse(fa ^ tail);
            fa = fb;
            fb = fc;
            fc = fd;
            fd = next_a;
        }
        return seahashDiffuse(fa ^ fb ^ fc ^ fd ^ _.written);
    }
}

pub fn seahashBytes(buf: []u8): u64 {
    let h = SeaHasher.new();
    h.writeBytes(buf);
    return h.finish();
}

pub fn seahashSeeded(buf: []u8, k1: u64, k2: u64, k3: u64, k4: u64): u64 {
    let h = SeaHasher.withSeeds(k1, k2, k3, k4);
    h.writeBytes(buf);
    return h.finish();
}
