// @cap/ahash — Zero-allocation randomized high-speed hasher with 4-key RandomState & folded-multiply rounds for CP
// Clean-room MIT implementation inspired by Rust `ahash` (946M downloads).
// Accelerated with hardware u128 folded-multiply, hardware rotl64 & wrapping arithmetic (+%, *%),
// 16-lane @Vector(16, u8) chunked loads, and bulk @memcpy tail buffering.

type AhashVec16 = @Vector(16, u8);

const AHASH_MULTIPLE: u64 = ((0x63641362 as u64) << 32) | (0x23846793 as u64);
const AHASH_PI0: u64 = ((0x243F6A88 as u64) << 32) | (0x85A308D3 as u64);
const AHASH_PI1: u64 = ((0x13198A2E as u64) << 32) | (0x03707344 as u64);
const AHASH_PI2: u64 = ((0xA4093822 as u64) << 32) | (0x299F31D0 as u64);
const AHASH_PI3: u64 = ((0x082EFA98 as u64) << 32) | (0xEC4E6C89 as u64);

fn rotl64(x: u64, r_in: u32) -> u64 {
    const r: u32 = r_in & (63 as u32);
    if (r == (0 as u32)) {
        return x;
    }
    const ls: u6 = @intCast(r);
    const rs: u6 = @intCast((64 as u32) - r);
    return (x << ls) | (x >> rs);
}

fn foldedMultiply(x: u64, y: u64) -> u64 {
    const wide: u128 = (x as u128) * (y as u128);
    return (wide as u64) ^ ((wide >> 64) as u64);
}

fn ahashReadLe64(b: []const u8, off: usize) -> u64 {
    const lo: u64 = (b[off] as u64)
        | ((b[off + 1] as u64) << 8)
        | ((b[off + 2] as u64) << 16)
        | ((b[off + 3] as u64) << 24);
    const hi: u64 = (b[off + 4] as u64)
        | ((b[off + 5] as u64) << 8)
        | ((b[off + 6] as u64) << 16)
        | ((b[off + 7] as u64) << 24);
    return lo | (hi << 32);
}

fn packTail(tail: [8]u8, tail_len: usize) -> u64 {
    let v: u64 = 0;
    let i: usize = 0;
    while (i < tail_len && i < 8) {
        const sh: u6 = @intCast(i * 8);
        v |= (tail[i] as u64) << sh;
        i += 1;
    }
    return v;
}

pub struct AHasher {
    pub buffer: u64,
    pub pad: u64,
    pub extra_keys: [2]u64,
    pub tail: [8]u8,
    pub tail_len: usize,
    pub total_len: u64,

    pub static fn init(k0: u64, k1: u64, k2: u64, k3: u64) -> AHasher {
        let ek: [2]u64 = undefined;
        ek[0] = k2 ^ AHASH_PI2;
        ek[1] = k3 ^ AHASH_PI3;

        const t: [8]u8 = undefined;
        const b0 = foldedMultiply(k0 ^ AHASH_PI0, AHASH_MULTIPLE);
        const p0 = foldedMultiply(k1 ^ AHASH_PI1, AHASH_MULTIPLE);

        return AHasher{
            buffer: b0,
            pad: p0,
            extra_keys: ek,
            tail: t,
            tail_len: 0 as usize,
            total_len: 0 as u64,
        };
    }

    pub fn writeU8(*_, v: u8) {
        _.tail[_.tail_len] = v;
        _.tail_len += 1;
        _.total_len += 1 as u64;
        if (_.tail_len == 8) {
            const word = ahashReadLe64(_.tail[0..8], 0);
            _.buffer = rotl64(foldedMultiply(word +% _.buffer, AHASH_MULTIPLE), 23 as u32);
            _.pad = _.pad +% (word ^ _.extra_keys[0]);
            _.tail_len = 0 as usize;
        }
    }

    pub fn writeU32(*_, v: u32) {
        _.writeU8((v & 0xFF) as u8);
        _.writeU8(((v >> 8) & 0xFF) as u8);
        _.writeU8(((v >> 16) & 0xFF) as u8);
        _.writeU8(((v >> 24) & 0xFF) as u8);
    }

    pub fn writeU64(*_, v: u64) {
        if (_.tail_len == 0) {
            _.buffer = rotl64(foldedMultiply(v +% _.buffer, AHASH_MULTIPLE), 23 as u32);
            _.pad = _.pad +% (v ^ _.extra_keys[0]);
            _.total_len += 8 as u64;
            return;
        }
        let cur = v;
        let i: usize = 0;
        while (i < 8) {
            _.writeU8((cur & 0xFF) as u8);
            cur >>= 8;
            i += 1;
        }
    }

    pub fn writeBytes(*_, data: []const u8) {
        if (data.len == 0) {
            return;
        }
        let i: usize = 0;
        if (_.tail_len > 0) {
            const need = (8 as usize) - _.tail_len;
            const take = if (data.len < need) data.len else need;
            @memcpy(_.tail[_.tail_len .. _.tail_len + take], data[0..take]);
            _.tail_len += take;
            _.total_len += take as u64;
            i += take;
            if (_.tail_len == 8) {
                const word = ahashReadLe64(_.tail[0..8], 0);
                _.buffer = rotl64(foldedMultiply(word +% _.buffer, AHASH_MULTIPLE), 23 as u32);
                _.pad = _.pad +% (word ^ _.extra_keys[0]);
                _.tail_len = 0 as usize;
            }
        }

        const ek0 = _.extra_keys[0];
        let buf = _.buffer;
        let pad = _.pad;

        // 16-lane @Vector(16, u8) chunked load (2x 64-bit words per iteration)
        while (i + 16 <= data.len) {
            const chunk_v: AhashVec16 = data[i .. i + 16][0..16].*;
            const chunk: [16]u8 = chunk_v;
            const w0 = ahashReadLe64(chunk[0..16], 0);
            buf = rotl64(foldedMultiply(w0 +% buf, AHASH_MULTIPLE), 23 as u32);
            pad = pad +% (w0 ^ ek0);
            const w1 = ahashReadLe64(chunk[0..16], 8);
            buf = rotl64(foldedMultiply(w1 +% buf, AHASH_MULTIPLE), 23 as u32);
            pad = pad +% (w1 ^ ek0);
            _.total_len += 16 as u64;
            i += 16;
        }

        while (i + 8 <= data.len) {
            const word = ahashReadLe64(data, i);
            buf = rotl64(foldedMultiply(word +% buf, AHASH_MULTIPLE), 23 as u32);
            pad = pad +% (word ^ ek0);
            _.total_len += 8 as u64;
            i += 8;
        }

        _.buffer = buf;
        _.pad = pad;

        const rem = data.len - i;
        if (rem > 0) {
            @memcpy(_.tail[0..rem], data[i..data.len]);
            _.tail_len = rem;
            _.total_len += rem as u64;
        }
    }

    pub fn finish() -> u64 {
        let buf = _.buffer;
        let p = _.pad;

        if (_.tail_len > 0) {
            const word = packTail(_.tail, _.tail_len);
            buf = rotl64(foldedMultiply(word +% buf, AHASH_MULTIPLE), 23 as u32);
            p = p +% (word ^ _.extra_keys[0]);
        }

        // Mix total_len and extra_keys with data-dependent rotation
        buf = foldedMultiply(buf ^ _.total_len ^ _.extra_keys[0], AHASH_MULTIPLE);
        const rot = ((buf & (63 as u64)) as u32);
        buf = rotl64(buf, rot);
        buf = foldedMultiply(buf ^ p ^ _.extra_keys[1], AHASH_MULTIPLE);
        return buf;
    }
}

pub struct RandomState {
    pub k0: u64,
    pub k1: u64,
    pub k2: u64,
    pub k3: u64,

    pub static fn withSeeds(k0: u64, k1: u64, k2: u64, k3: u64) -> RandomState {
        return RandomState{
            k0: k0,
            k1: k1,
            k2: k2,
            k3: k3,
        };
    }

    pub static fn defaultState() -> RandomState {
        return RandomState{
            k0: AHASH_PI0,
            k1: AHASH_PI1,
            k2: AHASH_PI2,
            k3: AHASH_PI3,
        };
    }

    pub fn buildHasher() -> AHasher {
        return AHasher.init(_.k0, _.k1, _.k2, _.k3);
    }

    pub fn hashOneBytes(data: []const u8) -> u64 {
        let h = _.buildHasher();
        h.writeBytes(data);
        return h.finish();
    }

    pub fn hashOneU64(v: u64) -> u64 {
        let h = _.buildHasher();
        h.writeU64(v);
        return h.finish();
    }
}

pub fn hashBytes(data: []const u8, k0: u64, k1: u64) -> u64 {
    let h = AHasher.init(k0, k1, AHASH_PI2, AHASH_PI3);
    h.writeBytes(data);
    return h.finish();
}

pub fn hashU64(v: u64, k0: u64, k1: u64) -> u64 {
    let h = AHasher.init(k0, k1, AHASH_PI2, AHASH_PI3);
    h.writeU64(v);
    return h.finish();
}

export {
    RandomState,
    AHasher,
    hashBytes,
    hashU64,
};
