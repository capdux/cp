// @cap/nanoid — Zero-heap URL-safe & custom-alphabet ID generator.
// Clean-room MIT port of npm `nanoid` v5 (100M+/week).

pub const URL_ALPHABET: string = "useandom-26T198340PX75pxJACKVERYMINDBUSHWOLF_GQZbfghjklqvwyzrict";
pub const DEFAULT_SIZE: usize = 21;

fn nidStepState(state: u64): u64 {
    let x: u64 = if (state == 0) { 0x5e3779b97f4a7c15 } else { state };
    let a: u64 = x ^ (x << 13);
    let b: u64 = a ^ (a >> 7);
    let c: u64 = b ^ (b << 17);
    return c;
}

fn nidBitmask(alphabet_len: usize): u8 {
    if (alphabet_len <= 1) {
        return 0;
    }
    let max_idx: usize = alphabet_len - 1;
    let mask: usize = 1;
    while (mask < max_idx) {
        mask = (mask << 1) | 1;
    }
    if (mask > 255) {
        return 255;
    }
    return mask as u8;
}

fn nidContainsByte(alphabet: string, ch: u8): bool {
    let i: usize = 0;
    while (i < alphabet.len) {
        if (alphabet[i] == ch) {
            return true;
        }
        i = i + 1;
    }
    return false;
}

/// Deterministic byte-to-NanoID mapping using `URL_ALPHABET[bytes[i] & 63]`,
/// 100% equivalent to official nanoid's core URL-safe mapping.
pub fn nanoidFromBytes(bytes: []u8, size: usize, out: []u8): usize {
    let target = if (size < out.len) { size } else { out.len };
    let n = if (bytes.len < target) { bytes.len } else { target };
    let i: usize = 0;
    while (i < n) {
        let idx: usize = (bytes[i] & 63) as usize;
        out[i] = URL_ALPHABET[idx];
        i = i + 1;
    }
    return n;
}

/// Unbiased custom-alphabet ID mapping from raw random bytes using bitmask rejection sampling.
/// Bytes whose masked value is `>= alphabet.len` are skipped to eliminate modulo bias.
pub fn customAlphabetFromBytes(alphabet: string, bytes: []u8, size: usize, out: []u8): usize {
    if (alphabet.len == 0 || alphabet.len > 256 || size == 0 || out.len == 0) {
        return 0;
    }
    let target = if (size < out.len) { size } else { out.len };
    let mask = nidBitmask(alphabet.len);
    let written: usize = 0;
    let bi: usize = 0;

    while (written < target && bi < bytes.len) {
        let idx: usize = (bytes[bi] & mask) as usize;
        if (idx < alphabet.len) {
            out[written] = alphabet[idx];
            written = written + 1;
        }
        bi = bi + 1;
    }
    return written;
}

/// Stateful zero-heap PRNG generator for continuous URL-safe or custom-alphabet NanoIDs.
pub struct NanoIdGenerator {
    pub state: u64,

    pub static fn new(seed: u64): NanoIdGenerator {
        let init: u64 = if (seed == 0) { 0x5e3779b97f4a7c15 } else { seed };
        return NanoIdGenerator { state: nidStepState(init) };
    }

    pub fn nextInto(*_, size: usize, out: []u8): usize {
        let target = if (size < out.len) { size } else { out.len };
        let written: usize = 0;

        while (written < target) {
            _.state = nidStepState(_.state);
            let word = _.state;
            let k: usize = 0;
            while (k < 8 && written < target) {
                let shift: u6 = ((7 - k) * 8) as u6;
                let byte_val: u8 = ((word >> shift) & 0xff) as u8;
                out[written] = URL_ALPHABET[(byte_val & 63) as usize];
                written = written + 1;
                k = k + 1;
            }
        }
        return written;
    }

    pub fn nextCustomInto(*_, alphabet: string, size: usize, out: []u8): usize {
        if (alphabet.len == 0 || alphabet.len > 256 || size == 0 || out.len == 0) {
            return 0;
        }
        let target = if (size < out.len) { size } else { out.len };
        let mask = nidBitmask(alphabet.len);
        let written: usize = 0;

        while (written < target) {
            _.state = nidStepState(_.state);
            let word = _.state;
            let k: usize = 0;
            while (k < 8 && written < target) {
                let shift: u6 = ((7 - k) * 8) as u6;
                let byte_val: u8 = ((word >> shift) & 0xff) as u8;
                let idx: usize = (byte_val & mask) as usize;
                if (idx < alphabet.len) {
                    out[written] = alphabet[idx];
                    written = written + 1;
                }
                k = k + 1;
            }
        }
        return written;
    }
}

/// Deterministically generates a URL-safe NanoID of `size` bytes from `seed` into `out`.
pub fn nanoidSeeded(seed: u64, size: usize, out: []u8): usize {
    let gen = NanoIdGenerator.new(seed);
    return gen.nextInto(size, out);
}

/// Validates that `id` is non-empty and every byte belongs to `alphabet`
/// (defaults to `URL_ALPHABET` when `alphabet` is empty).
pub fn isValidNanoId(id: string, alphabet: string): bool {
    if (id.len == 0) {
        return false;
    }
    let effective_alpha = if (alphabet.len == 0) { URL_ALPHABET } else { alphabet };
    let i: usize = 0;
    while (i < id.len) {
        if (!nidContainsByte(effective_alpha, id[i])) {
            return false;
        }
        i = i + 1;
    }
    return true;
}
