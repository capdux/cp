import { readU32LE, readU64LE } from "cap:bin";

fn xxhU64(hi: u32, lo: u32): u64 {
    return ((hi as u64) << 32) | (lo as u64);
}

fn xxhAdd32(a: u32, b: u32): u32 {
    return (((a as u64) + (b as u64)) & 4294967295) as u32;
}

fn xxhSub32(a: u32, b: u32): u32 {
    return xxhAdd32(a, xxhAdd32(b ^ 4294967295, 1));
}

fn xxhMul32(a: u32, b: u32): u32 {
    return (((a as u64) * (b as u64)) & 4294967295) as u32;
}

fn xxhRol32(x: u32, r: u32): u32 {
    return (x << r) | (x >> (32 - r));
}

fn xxhAdd64(a: u64, b: u64): u64 {
    let lo: u64 = (a & 4294967295) + (b & 4294967295);
    let carry: u64 = lo >> 32;
    let hi: u64 = ((a >> 32) + (b >> 32) + carry) & 4294967295;
    return (hi << 32) | (lo & 4294967295);
}

fn xxhSub64(a: u64, b: u64): u64 {
    let all_ones: u64 = xxhU64(4294967295, 4294967295);
    return xxhAdd64(a, xxhAdd64(b ^ all_ones, 1));
}

fn xxhMul64(a: u64, b: u64): u64 {
    let al: u64 = a & 4294967295;
    let ah: u64 = a >> 32;
    let bl: u64 = b & 4294967295;
    let bh: u64 = b >> 32;
    let p0: u64 = al * bl;
    let p1: u64 = (al * bh) & 4294967295;
    let p2: u64 = (ah * bl) & 4294967295;
    let hi: u64 = ((p0 >> 32) + p1 + p2) & 4294967295;
    return (hi << 32) | (p0 & 4294967295);
}

fn xxhRol64(x: u64, r: u64): u64 {
    return (x << r) | (x >> (64 - r));
}

fn xxh32Round(acc: u32, lane: u32): u32 {
    let p1: u32 = 2654435761; // 0x9E3779B1
    let p2: u32 = 2246822519; // 0x85EBCA77
    let a: u32 = xxhAdd32(acc, xxhMul32(lane, p2));
    return xxhMul32(xxhRol32(a, 13), p1);
}

fn xxh64Prime1(): u64 { return xxhU64(2654435761, 2246822535); } // 0x9E3779B185EBCA87
fn xxh64Prime2(): u64 { return xxhU64(3266489917, 668265295); }  // 0xC2B2AE3D27D4EB4F
fn xxh64Prime3(): u64 { return xxhU64(374761393, 2654435833); }  // 0x165667B19E3779F9
fn xxh64Prime4(): u64 { return xxhU64(2246822519, 3266489955); } // 0x85EBCA77C2B2AE63
fn xxh64Prime5(): u64 { return xxhU64(668265263, 374761413); }   // 0x27D4EB2F165667C5

fn xxh64Round(acc: u64, lane: u64): u64 {
    let a: u64 = xxhAdd64(acc, xxhMul64(lane, xxh64Prime2()));
    return xxhMul64(xxhRol64(a, 31), xxh64Prime1());
}

fn xxh64MergeRound(acc: u64, val: u64): u64 {
    let v: u64 = xxh64Round(0, val);
    let x: u64 = acc ^ v;
    return xxhAdd64(xxhMul64(x, xxh64Prime1()), xxh64Prime4());
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
        let p1: u32 = 2654435761;
        let p2: u32 = 2246822519;
        let zbuf: [16]u8 = undefined;
        return XxHash32 {
            seed: seed,
            v1: xxhAdd32(xxhAdd32(seed, p1), p2),
            v2: xxhAdd32(seed, p2),
            v3: seed,
            v4: xxhSub32(seed, p1),
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

    pub fn write(*_, data: []u8) {
        let i: usize = 0;
        while (i < data.len) {
            _.buf[_.buf_len] = data[i];
            _.buf_len = _.buf_len + 1;
            _.total_len = _.total_len + 1;
            if (_.buf_len == 16) {
                _.v1 = xxh32Round(_.v1, readU32LE(_.buf[0..16], 0) ?? 0);
                _.v2 = xxh32Round(_.v2, readU32LE(_.buf[0..16], 4) ?? 0);
                _.v3 = xxh32Round(_.v3, readU32LE(_.buf[0..16], 8) ?? 0);
                _.v4 = xxh32Round(_.v4, readU32LE(_.buf[0..16], 12) ?? 0);
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
            _.total_len = _.total_len + 1;
            if (_.buf_len == 16) {
                _.v1 = xxh32Round(_.v1, readU32LE(_.buf[0..16], 0) ?? 0);
                _.v2 = xxh32Round(_.v2, readU32LE(_.buf[0..16], 4) ?? 0);
                _.v3 = xxh32Round(_.v3, readU32LE(_.buf[0..16], 8) ?? 0);
                _.v4 = xxh32Round(_.v4, readU32LE(_.buf[0..16], 12) ?? 0);
                _.buf_len = 0;
            }
            i = i + 1;
        }
    }

    pub fn finish(): u32 {
        let p1: u32 = 2654435761;
        let p2: u32 = 2246822519;
        let p3: u32 = 3266489917;
        let p4: u32 = 668265263;
        let p5: u32 = 374761393;

        let h: u32 = 0;
        if (_.total_len >= 16) {
            h = xxhAdd32(
                xxhAdd32(xxhRol32(_.v1, 1), xxhRol32(_.v2, 7)),
                xxhAdd32(xxhRol32(_.v3, 12), xxhRol32(_.v4, 18))
            );
        } else {
            h = xxhAdd32(_.seed, p5);
        }
        h = xxhAdd32(h, (_.total_len & 4294967295) as u32);

        let pos: usize = 0;
        while (pos + 4 <= _.buf_len) {
            let lane: u32 = readU32LE(_.buf[0.._.buf_len], pos) ?? 0;
            h = xxhAdd32(h, xxhMul32(lane, p3));
            h = xxhMul32(xxhRol32(h, 17), p4);
            pos = pos + 4;
        }
        while (pos < _.buf_len) {
            h = xxhAdd32(h, xxhMul32(_.buf[pos] as u32, p5));
            h = xxhMul32(xxhRol32(h, 11), p1);
            pos = pos + 1;
        }

        h = h ^ (h >> 15);
        h = xxhMul32(h, p2);
        h = h ^ (h >> 13);
        h = xxhMul32(h, p3);
        h = h ^ (h >> 16);
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
        let p1: u64 = xxh64Prime1();
        let p2: u64 = xxh64Prime2();
        let zbuf: [32]u8 = undefined;
        return XxHash64 {
            seed: seed,
            v1: xxhAdd64(xxhAdd64(seed, p1), p2),
            v2: xxhAdd64(seed, p2),
            v3: seed,
            v4: xxhSub64(seed, p1),
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

    pub fn write(*_, data: []u8) {
        let i: usize = 0;
        while (i < data.len) {
            _.buf[_.buf_len] = data[i];
            _.buf_len = _.buf_len + 1;
            _.total_len = _.total_len + 1;
            if (_.buf_len == 32) {
                _.v1 = xxh64Round(_.v1, readU64LE(_.buf[0..32], 0) ?? 0);
                _.v2 = xxh64Round(_.v2, readU64LE(_.buf[0..32], 8) ?? 0);
                _.v3 = xxh64Round(_.v3, readU64LE(_.buf[0..32], 16) ?? 0);
                _.v4 = xxh64Round(_.v4, readU64LE(_.buf[0..32], 24) ?? 0);
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
            _.total_len = _.total_len + 1;
            if (_.buf_len == 32) {
                _.v1 = xxh64Round(_.v1, readU64LE(_.buf[0..32], 0) ?? 0);
                _.v2 = xxh64Round(_.v2, readU64LE(_.buf[0..32], 8) ?? 0);
                _.v3 = xxh64Round(_.v3, readU64LE(_.buf[0..32], 16) ?? 0);
                _.v4 = xxh64Round(_.v4, readU64LE(_.buf[0..32], 24) ?? 0);
                _.buf_len = 0;
            }
            i = i + 1;
        }
    }

    pub fn finish(): u64 {
        let p1: u64 = xxh64Prime1();
        let p2: u64 = xxh64Prime2();
        let p3: u64 = xxh64Prime3();
        let p4: u64 = xxh64Prime4();
        let p5: u64 = xxh64Prime5();

        let h: u64 = 0;
        if (_.total_len >= 32) {
            h = xxhAdd64(
                xxhAdd64(xxhRol64(_.v1, 1), xxhRol64(_.v2, 7)),
                xxhAdd64(xxhRol64(_.v3, 12), xxhRol64(_.v4, 18))
            );
            h = xxh64MergeRound(h, _.v1);
            h = xxh64MergeRound(h, _.v2);
            h = xxh64MergeRound(h, _.v3);
            h = xxh64MergeRound(h, _.v4);
        } else {
            h = xxhAdd64(_.seed, p5);
        }
        h = xxhAdd64(h, _.total_len);

        let pos: usize = 0;
        while (pos + 8 <= _.buf_len) {
            let k1: u64 = xxh64Round(0, readU64LE(_.buf[0.._.buf_len], pos) ?? 0);
            h = h ^ k1;
            h = xxhAdd64(xxhMul64(xxhRol64(h, 27), p1), p4);
            pos = pos + 8;
        }
        if (pos + 4 <= _.buf_len) {
            let lane32: u32 = readU32LE(_.buf[0.._.buf_len], pos) ?? 0;
            h = h ^ xxhMul64(lane32 as u64, p1);
            h = xxhAdd64(xxhMul64(xxhRol64(h, 23), p2), p3);
            pos = pos + 4;
        }
        while (pos < _.buf_len) {
            h = h ^ xxhMul64(_.buf[pos] as u64, p5);
            h = xxhMul64(xxhRol64(h, 11), p1);
            pos = pos + 1;
        }

        h = h ^ (h >> 33);
        h = xxhMul64(h, p2);
        h = h ^ (h >> 29);
        h = xxhMul64(h, p3);
        h = h ^ (h >> 32);
        return h;
    }
}
