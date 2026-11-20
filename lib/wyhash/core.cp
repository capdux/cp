// Source: bun/src/wyhash/wyhash.cp -> cp/lib/wyhash/core.cp (Wyhash v4 core primitives & WyhashStream)

const PRIMES: [u64; 5] = [
    0xa0761d6478bd642f,
    0xe7037ed1a0b428db,
    0x8ebc6af09c88c6e3,
    0x589965cc75374cc3,
    0x1d8e4e27c47d124f,
];

fn readBytes1(data: string): u64 {
    return data[0] as u64;
}

fn readBytes2(data: string): u64 {
    return (data[0] as u64) | ((data[1] as u64) << 8);
}

fn readBytes4(data: string): u64 {
    return (data[0] as u64)
        | ((data[1] as u64) << 8)
        | ((data[2] as u64) << 16)
        | ((data[3] as u64) << 24);
}

fn readBytes8(data: string): u64 {
    return (data[0] as u64)
        | ((data[1] as u64) << 8)
        | ((data[2] as u64) << 16)
        | ((data[3] as u64) << 24)
        | ((data[4] as u64) << 32)
        | ((data[5] as u64) << 40)
        | ((data[6] as u64) << 48)
        | ((data[7] as u64) << 56);
}

fn read8BytesSwapped(data: string): u64 {
    return (readBytes4(data) << 32) | readBytes4(data[4..]);
}

pub fn wyhashMum(a: u64, b: u64): u64 {
    let r: u128 = (a as u128) * (b as u128);
    r = (r >> 64) ^ r;
    return r as u64;
}

fn mix0(a: u64, b: u64, seed: u64): u64 {
    return wyhashMum(a ^ seed ^ PRIMES[0], b ^ seed ^ PRIMES[1]);
}

fn mix1(a: u64, b: u64, seed: u64): u64 {
    return wyhashMum(a ^ seed ^ PRIMES[2], b ^ seed ^ PRIMES[3]);
}

pub fn wyhashFinal4(a: u64, b: u64): u64 {
    let m0 = wyhashMum(a ^ PRIMES[0], b ^ PRIMES[1]);
    let m1 = wyhashMum(a ^ PRIMES[2], b ^ PRIMES[3]);
    return wyhashMum(m0 ^ m1, PRIMES[4]);
}

pub fn wyhash64(a: u64, b: u64): u64 {
    return wyhashFinal4(a, b);
}

/// Stateless Wyhash v4 core state machine (32-byte block rounds + tail finalization).
pub struct WyhashStateless {
    seed: u64,
    msg_len: usize,

    pub static fn init(seed: u64): WyhashStateless {
        return WyhashStateless {
            seed: seed,
            msg_len: 0,
        };
    }

    pub fn round(*_, b: string) {
        _.seed = mix0(
            readBytes8(b[0..]),
            readBytes8(b[8..]),
            _.seed,
        ) ^ mix1(
            readBytes8(b[16..]),
            readBytes8(b[24..]),
            _.seed,
        );
    }

    pub fn update(*_, b: string) {
        let off: usize = 0;
        while (off < b.len) {
            _.round(b[off..off + 32]);
            off += 32;
        }
        _.msg_len += b.len;
    }

    pub fn final(*_, b: string): u64 {
        let s = _.seed;
        let rem_len = b.len;
        let rem_key = b[0..rem_len];

        _.seed = switch (rem_len) {
            0 => s,
            1 => mix0(readBytes1(rem_key), PRIMES[4], s),
            2 => mix0(readBytes2(rem_key), PRIMES[4], s),
            3 => mix0((readBytes2(rem_key) << 8) | readBytes1(rem_key[2..]), PRIMES[4], s),
            4 => mix0(readBytes4(rem_key), PRIMES[4], s),
            5 => mix0((readBytes4(rem_key) << 8) | readBytes1(rem_key[4..]), PRIMES[4], s),
            6 => mix0((readBytes4(rem_key) << 16) | readBytes2(rem_key[4..]), PRIMES[4], s),
            7 => mix0((readBytes4(rem_key) << 24) | (readBytes2(rem_key[4..]) << 8) | readBytes1(rem_key[6..]), PRIMES[4], s),
            8 => mix0(read8BytesSwapped(rem_key), PRIMES[4], s),
            9 => mix0(read8BytesSwapped(rem_key), readBytes1(rem_key[8..]), s),
            10 => mix0(read8BytesSwapped(rem_key), readBytes2(rem_key[8..]), s),
            11 => mix0(read8BytesSwapped(rem_key), (readBytes2(rem_key[8..]) << 8) | readBytes1(rem_key[10..]), s),
            12 => mix0(read8BytesSwapped(rem_key), readBytes4(rem_key[8..]), s),
            13 => mix0(read8BytesSwapped(rem_key), (readBytes4(rem_key[8..]) << 8) | readBytes1(rem_key[12..]), s),
            14 => mix0(read8BytesSwapped(rem_key), (readBytes4(rem_key[8..]) << 16) | readBytes2(rem_key[12..]), s),
            15 => mix0(read8BytesSwapped(rem_key), (readBytes4(rem_key[8..]) << 24) | (readBytes2(rem_key[12..]) << 8) | readBytes1(rem_key[14..]), s),
            16 => mix0(read8BytesSwapped(rem_key), read8BytesSwapped(rem_key[8..]), s),
            17 => mix0(read8BytesSwapped(rem_key), read8BytesSwapped(rem_key[8..]), s) ^ mix1(readBytes1(rem_key[16..]), PRIMES[4], s),
            18 => mix0(read8BytesSwapped(rem_key), read8BytesSwapped(rem_key[8..]), s) ^ mix1(readBytes2(rem_key[16..]), PRIMES[4], s),
            19 => mix0(read8BytesSwapped(rem_key), read8BytesSwapped(rem_key[8..]), s) ^ mix1((readBytes2(rem_key[16..]) << 8) | readBytes1(rem_key[18..]), PRIMES[4], s),
            20 => mix0(read8BytesSwapped(rem_key), read8BytesSwapped(rem_key[8..]), s) ^ mix1(readBytes4(rem_key[16..]), PRIMES[4], s),
            21 => mix0(read8BytesSwapped(rem_key), read8BytesSwapped(rem_key[8..]), s) ^ mix1((readBytes4(rem_key[16..]) << 8) | readBytes1(rem_key[20..]), PRIMES[4], s),
            22 => mix0(read8BytesSwapped(rem_key), read8BytesSwapped(rem_key[8..]), s) ^ mix1((readBytes4(rem_key[16..]) << 16) | readBytes2(rem_key[20..]), PRIMES[4], s),
            23 => mix0(read8BytesSwapped(rem_key), read8BytesSwapped(rem_key[8..]), s) ^ mix1((readBytes4(rem_key[16..]) << 24) | (readBytes2(rem_key[20..]) << 8) | readBytes1(rem_key[22..]), PRIMES[4], s),
            24 => mix0(read8BytesSwapped(rem_key), read8BytesSwapped(rem_key[8..]), s) ^ mix1(read8BytesSwapped(rem_key[16..]), PRIMES[4], s),
            25 => mix0(read8BytesSwapped(rem_key), read8BytesSwapped(rem_key[8..]), s) ^ mix1(read8BytesSwapped(rem_key[16..]), readBytes1(rem_key[24..]), s),
            26 => mix0(read8BytesSwapped(rem_key), read8BytesSwapped(rem_key[8..]), s) ^ mix1(read8BytesSwapped(rem_key[16..]), readBytes2(rem_key[24..]), s),
            27 => mix0(read8BytesSwapped(rem_key), read8BytesSwapped(rem_key[8..]), s) ^ mix1(read8BytesSwapped(rem_key[16..]), (readBytes2(rem_key[24..]) << 8) | readBytes1(rem_key[26..]), s),
            28 => mix0(read8BytesSwapped(rem_key), read8BytesSwapped(rem_key[8..]), s) ^ mix1(read8BytesSwapped(rem_key[16..]), readBytes4(rem_key[24..]), s),
            29 => mix0(read8BytesSwapped(rem_key), read8BytesSwapped(rem_key[8..]), s) ^ mix1(read8BytesSwapped(rem_key[16..]), (readBytes4(rem_key[24..]) << 8) | readBytes1(rem_key[28..]), s),
            30 => mix0(read8BytesSwapped(rem_key), read8BytesSwapped(rem_key[8..]), s) ^ mix1(read8BytesSwapped(rem_key[16..]), (readBytes4(rem_key[24..]) << 16) | readBytes2(rem_key[28..]), s),
            31 => mix0(read8BytesSwapped(rem_key), read8BytesSwapped(rem_key[8..]), s) ^ mix1(read8BytesSwapped(rem_key[16..]), (readBytes4(rem_key[24..]) << 24) | (readBytes2(rem_key[28..]) << 8) | readBytes1(rem_key[30..]), s),
            else => s,
        };

        _.msg_len += b.len;
        return wyhashMum(_.seed ^ (_.msg_len as u64), PRIMES[4]);
    }

    pub static fn hash(seed: u64, input: string): u64 {
        let aligned_len = input.len - (input.len % 32);
        let c = WyhashStateless.init(seed);
        c.update(input[0..aligned_len]);
        return c.final(input[aligned_len..]);
    }
}

/// Streaming incremental Wyhash v4 hasher with 32-byte internal buffer.
/// Any chunked update sequence produces bit-exact output identical to one-shot `wyhash(seed, data)`.
pub struct WyhashStream {
    state: WyhashStateless,
    buf: [u8; 32],
    buf_len: usize,

    pub static fn init(seed: u64): WyhashStream {
        return WyhashStream {
            state: WyhashStateless.init(seed),
            buf: [0; 32],
            buf_len: 0,
        };
    }

    pub fn update(*_, chunk: string) {
        let off: usize = 0;

        if (_.buf_len != 0 && _.buf_len + chunk.len >= 32) {
            off += 32 - _.buf_len;
            let i: usize = 0;
            while (i < off) {
                _.buf[_.buf_len + i] = chunk[i];
                i += 1;
            }
            _.state.update(_.buf[0..32]);
            _.buf_len = 0;
        }

        let remain_len = chunk.len - off;
        let aligned_len = remain_len - (remain_len % 32);
        _.state.update(chunk[off..off + aligned_len]);

        let rem_slice = chunk[off + aligned_len..chunk.len];
        let j: usize = 0;
        while (j < rem_slice.len) {
            _.buf[_.buf_len + j] = rem_slice[j];
            j += 1;
        }
        _.buf_len += rem_slice.len;
    }

    pub fn final(*_): u64 {
        let rem_key = _.buf[0.._.buf_len];
        return _.state.final(rem_key);
    }

    pub static fn hash(seed: u64, input: string): u64 {
        return WyhashStateless.hash(seed, input);
    }
}

pub const Wyhash = WyhashStream;

pub fn wyhash(seed: u64, input: string): u64 {
    return WyhashStateless.hash(seed, input);
}
