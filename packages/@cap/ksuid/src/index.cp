// @cap/ksuid — Zero-heap Segment KSUID 20-byte / 27-char Base62 codec
// MIT License (clean-room CP implementation)

pub const KSUID_EPOCH: u64 = 1400000000;
pub const KSUID_BYTE_LEN: usize = 20;
pub const KSUID_STR_LEN: usize = 27;

pub struct Ksuid {
    pub bytes: [20]u8,
}

fn b62Char(v: u8) -> u8 {
    const alphabet = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    return alphabet[v as usize];
}

fn b62Val(c: u8) -> i32 {
    if (c >= '0' as u8 && c <= '9' as u8) {
        return (c - ('0' as u8)) as i32;
    }
    if (c >= 'A' as u8 && c <= 'Z' as u8) {
        return (c - ('A' as u8) + 10) as i32;
    }
    if (c >= 'a' as u8 && c <= 'z' as u8) {
        return (c - ('a' as u8) + 36) as i32;
    }
    return -1;
}

pub fn fromParts(unix_secs: u64, payload: [16]u8) -> Ksuid {
    let k = Ksuid { bytes: undefined };
    let ts: u32 = 0;
    if (unix_secs >= KSUID_EPOCH) {
        const diff: u64 = unix_secs - KSUID_EPOCH;
        if (diff <= 0xFFFFFFFF) {
            ts = diff as u32;
        } else {
            ts = 0xFFFFFFFF;
        }
    }
    k.bytes[0] = ((ts >> 24) & 0xFF) as u8;
    k.bytes[1] = ((ts >> 16) & 0xFF) as u8;
    k.bytes[2] = ((ts >> 8) & 0xFF) as u8;
    k.bytes[3] = (ts & 0xFF) as u8;

    let i: usize = 0;
    while (i < 16) {
        k.bytes[4 + i] = payload[i];
        i += 1;
    }
    return k;
}

pub fn timestampSecs(k: Ksuid) -> u64 {
    const ts: u32 = ((k.bytes[0] as u32) << 24)
        | ((k.bytes[1] as u32) << 16)
        | ((k.bytes[2] as u32) << 8)
        | (k.bytes[3] as u32);
    return KSUID_EPOCH + (ts as u64);
}

pub fn encode(k: Ksuid, out: []u8) -> usize {
    if (out.len < KSUID_STR_LEN) {
        return 0;
    }

    // Load 20 big-endian bytes into 5 u32 limbs (MSB first)
    let limbs: [5]u32 = undefined;
    let i: usize = 0;
    while (i < 5) {
        const b0: u32 = (k.bytes[i * 4] as u32) << 24;
        const b1: u32 = (k.bytes[i * 4 + 1] as u32) << 16;
        const b2: u32 = (k.bytes[i * 4 + 2] as u32) << 8;
        const b3: u32 = k.bytes[i * 4 + 3] as u32;
        limbs[i] = b0 | b1 | b2 | b3;
        i += 1;
    }

    let idx: usize = KSUID_STR_LEN;
    while (idx > 0) {
        idx -= 1;
        let rem: u64 = 0;
        let j: usize = 0;
        while (j < 5) {
            const cur: u64 = (rem << 32) | (limbs[j] as u64);
            limbs[j] = (cur / 62) as u32;
            rem = cur % 62;
            j += 1;
        }
        out[idx] = b62Char(rem as u8);
    }

    return KSUID_STR_LEN;
}

pub fn decode(s: string, out: *Ksuid) -> bool {
    if (s.len != KSUID_STR_LEN) {
        return false;
    }

    let limbs: [5]u32 = [0, 0, 0, 0, 0];
    let i: usize = 0;
    while (i < KSUID_STR_LEN) {
        const d: i32 = b62Val(s[i]);
        if (d < 0) {
            return false;
        }
        let carry: u64 = d as u64;
        let j: usize = 5;
        while (j > 0) {
            j -= 1;
            const cur: u64 = (limbs[j] as u64) * 62 + carry;
            limbs[j] = (cur & 0xFFFFFFFF) as u32;
            carry = cur >> 32;
        }
        if (carry != 0) {
            return false;
        }
        i += 1;
    }

    let res = Ksuid { bytes: undefined };
    let k: usize = 0;
    while (k < 5) {
        const v: u32 = limbs[k];
        res.bytes[k * 4] = ((v >> 24) & 0xFF) as u8;
        res.bytes[k * 4 + 1] = ((v >> 16) & 0xFF) as u8;
        res.bytes[k * 4 + 2] = ((v >> 8) & 0xFF) as u8;
        res.bytes[k * 4 + 3] = (v & 0xFF) as u8;
        k += 1;
    }
    out.* = res;
    return true;
}

pub fn isValidKsuid(s: string) -> bool {
    let tmp: Ksuid = undefined;
    return decode(s, &tmp);
}

pub fn compare(a: Ksuid, b: Ksuid) -> i32 {
    let i: usize = 0;
    while (i < KSUID_BYTE_LEN) {
        if (a.bytes[i] < b.bytes[i]) {
            return -1;
        }
        if (a.bytes[i] > b.bytes[i]) {
            return 1;
        }
        i += 1;
    }
    return 0;
}
