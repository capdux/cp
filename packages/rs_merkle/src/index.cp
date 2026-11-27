// @cap/merkle — Zero-allocation RFC 6962 domain-separated SHA-256 Merkle tree & inclusion proof engine for CP
// Clean-room MIT implementation inspired by Rust `rs_merkle` and npm `merkletreejs`.

const MAX_LEAVES: usize = 64;
const MAX_PROOF_DEPTH: usize = 8;

pub struct Hash32 {
    pub bytes: [32]u8,

    pub fn equals(other: Hash32) -> bool {
        let diff: u8 = 0;
        let i: usize = 0;
        while (i < 32) {
            diff = diff | (_.bytes[i] ^ other.bytes[i]);
            i += 1;
        }
        return diff == (0 as u8);
    }

    pub fn toHex(out: []u8) -> string {
        let pos: usize = 0;
        let i: usize = 0;
        while (i < 32 && pos + 2 <= out.len) {
            const b = _.bytes[i];
            out[pos] = hexLower(b / (16 as u8));
            out[pos + 1] = hexLower(b % (16 as u8));
            pos += 2;
            i += 1;
        }
        return out[0..pos];
    }
}

fn hexLower(nibble: u8) -> u8 {
    if (nibble < (10 as u8)) {
        return '0' + nibble;
    }
    return 'a' + (nibble - (10 as u8));
}

fn zeroHash() -> Hash32 {
    let b: [32]u8 = undefined;
    let i: usize = 0;
    while (i < 32) {
        b[i] = 0 as u8;
        i += 1;
    }
    return Hash32{ bytes: b };
}

// --- FIPS 180-4 SHA-256 (zero heap, overflow-safe) ---

fn add32(a: u32, b: u32) -> u32 {
    return (((a as u64) + (b as u64)) & (0xFFFFFFFF as u64)) as u32;
}

fn pow2_64(n: u32) -> u64 {
    let v: u64 = 1;
    let i: u32 = 0;
    while (i < n) {
        v = v * (2 as u64);
        i += 1 as u32;
    }
    return v;
}

fn rotr32(x: u32, n: u32) -> u32 {
    const d = pow2_64(n);
    const m = pow2_64((32 as u32) - n);
    const xu = x as u64;
    const lo = xu / d;
    const hi = (xu % d) * m;
    return ((lo | hi) & (0xFFFFFFFF as u64)) as u32;
}

fn shr32(x: u32, n: u32) -> u32 {
    return ((x as u64) / pow2_64(n)) as u32;
}

fn bitNot32(x: u32) -> u32 {
    return x ^ (0xFFFFFFFF as u32);
}

fn sha256K(i: usize) -> u32 {
    const table: [64]u32 = [
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
    ];
    return table[i];
}

struct Sha256State {
    h: [8]u32,
    buf: [64]u8,
    buf_len: usize,
    total_len: u64,
}

fn sha256Init() -> Sha256State {
    let h: [8]u32 = [
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19,
    ];
    let buf: [64]u8 = undefined;
    let i: usize = 0;
    while (i < 64) {
        buf[i] = 0 as u8;
        i += 1;
    }
    return Sha256State{
        h: h,
        buf: buf,
        buf_len: 0 as usize,
        total_len: 0 as u64,
    };
}

fn sha256Compress(st: *Sha256State) {
    let w: [64]u32 = undefined;
    let i: usize = 0;
    while (i < 16) {
        const b0 = st.buf[i * 4] as u32;
        const b1 = st.buf[(i * 4) + 1] as u32;
        const b2 = st.buf[(i * 4) + 2] as u32;
        const b3 = st.buf[(i * 4) + 3] as u32;
        w[i] = (b0 * (16777216 as u32)) | (b1 * (65536 as u32)) | (b2 * (256 as u32)) | b3;
        i += 1;
    }
    while (i < 64) {
        const s0 = rotr32(w[i - 15], 7 as u32) ^ rotr32(w[i - 15], 18 as u32) ^ shr32(w[i - 15], 3 as u32);
        const s1 = rotr32(w[i - 2], 17 as u32) ^ rotr32(w[i - 2], 19 as u32) ^ shr32(w[i - 2], 10 as u32);
        w[i] = add32(add32(w[i - 16], s0), add32(w[i - 7], s1));
        i += 1;
    }

    let a = st.h[0];
    let b = st.h[1];
    let c = st.h[2];
    let d = st.h[3];
    let e = st.h[4];
    let f = st.h[5];
    let g = st.h[6];
    let hh = st.h[7];

    let r: usize = 0;
    while (r < 64) {
        const S1 = rotr32(e, 6 as u32) ^ rotr32(e, 11 as u32) ^ rotr32(e, 25 as u32);
        const ch = (e & f) ^ (bitNot32(e) & g);
        const temp1 = add32(add32(add32(hh, S1), ch), add32(sha256K(r), w[r]));
        const S0 = rotr32(a, 2 as u32) ^ rotr32(a, 13 as u32) ^ rotr32(a, 22 as u32);
        const maj = (a & b) ^ (a & c) ^ (b & c);
        const temp2 = add32(S0, maj);

        hh = g;
        g = f;
        f = e;
        e = add32(d, temp1);
        d = c;
        c = b;
        b = a;
        a = add32(temp1, temp2);
        r += 1;
    }

    st.h[0] = add32(st.h[0], a);
    st.h[1] = add32(st.h[1], b);
    st.h[2] = add32(st.h[2], c);
    st.h[3] = add32(st.h[3], d);
    st.h[4] = add32(st.h[4], e);
    st.h[5] = add32(st.h[5], f);
    st.h[6] = add32(st.h[6], g);
    st.h[7] = add32(st.h[7], hh);
}

fn sha256UpdateByte(st: *Sha256State, byte: u8) {
    st.buf[st.buf_len] = byte;
    st.buf_len += 1;
    st.total_len += 1 as u64;
    if (st.buf_len == 64) {
        sha256Compress(st);
        st.buf_len = 0 as usize;
    }
}

fn sha256UpdateSlice(st: *Sha256State, data: string) {
    let i: usize = 0;
    while (i < data.len) {
        sha256UpdateByte(st, data[i]);
        i += 1;
    }
}

fn sha256Finalize(st: *Sha256State) -> Hash32 {
    const bitLen = st.total_len * (8 as u64);
    sha256UpdateByte(st, 0x80 as u8);
    while (st.buf_len != 56) {
        sha256UpdateByte(st, 0 as u8);
    }
    // Append 64-bit big-endian bit length
    let shift: u32 = 56;
    let k: usize = 0;
    while (k < 8) {
        const b = ((bitLen / pow2_64(shift)) & (0xFF as u64)) as u8;
        st.buf[56 + k] = b;
        if (shift >= (8 as u32)) {
            shift -= 8 as u32;
        }
        k += 1;
    }
    sha256Compress(st);

    let out: [32]u8 = undefined;
    let i: usize = 0;
    while (i < 8) {
        const v = st.h[i];
        out[i * 4] = ((v / (16777216 as u32)) & (0xFF as u32)) as u8;
        out[(i * 4) + 1] = ((v / (65536 as u32)) & (0xFF as u32)) as u8;
        out[(i * 4) + 2] = ((v / (256 as u32)) & (0xFF as u32)) as u8;
        out[(i * 4) + 3] = (v & (0xFF as u32)) as u8;
        i += 1;
    }
    return Hash32{ bytes: out };
}

pub fn sha256(data: string) -> Hash32 {
    let st = sha256Init();
    sha256UpdateSlice(&st, data);
    return sha256Finalize(&st);
}

// RFC 6962 domain-separated hashing:
// Leaf: 0x00 || data
// Node: 0x01 || left || right
pub fn hashLeaf(data: string) -> Hash32 {
    let st = sha256Init();
    sha256UpdateByte(&st, 0x00 as u8);
    sha256UpdateSlice(&st, data);
    return sha256Finalize(&st);
}

pub fn hashNode(left: Hash32, right: Hash32) -> Hash32 {
    let st = sha256Init();
    sha256UpdateByte(&st, 0x01 as u8);
    let i: usize = 0;
    while (i < 32) {
        sha256UpdateByte(&st, left.bytes[i]);
        i += 1;
    }
    let j: usize = 0;
    while (j < 32) {
        sha256UpdateByte(&st, right.bytes[j]);
        j += 1;
    }
    return sha256Finalize(&st);
}

pub struct MerkleProof {
    pub siblings: [MAX_PROOF_DEPTH]Hash32,
    pub is_right: [MAX_PROOF_DEPTH]bool,
    pub depth: usize,
}

fn emptyProof() -> MerkleProof {
    let sibs: [MAX_PROOF_DEPTH]Hash32 = undefined;
    let dirs: [MAX_PROOF_DEPTH]bool = undefined;
    let i: usize = 0;
    while (i < MAX_PROOF_DEPTH) {
        sibs[i] = zeroHash();
        dirs[i] = false;
        i += 1;
    }
    return MerkleProof{
        siblings: sibs,
        is_right: dirs,
        depth: 0 as usize,
    };
}

pub struct MerkleTree {
    pub leaves: [MAX_LEAVES]Hash32,
    pub count: usize,

    pub static fn init() -> MerkleTree {
        let arr: [MAX_LEAVES]Hash32 = undefined;
        let i: usize = 0;
        while (i < MAX_LEAVES) {
            arr[i] = zeroHash();
            i += 1;
        }
        return MerkleTree{
            leaves: arr,
            count: 0 as usize,
        };
    }

    pub fn addLeaf(*_, data: string) -> bool {
        if (_.count >= MAX_LEAVES) {
            return false;
        }
        _.leaves[_.count] = hashLeaf(data);
        _.count += 1;
        return true;
    }

    pub fn leafCount() -> usize {
        return _.count;
    }

    pub fn root() -> Hash32? {
        if (_.count == 0) {
            return null;
        }
        let level: [MAX_LEAVES]Hash32 = undefined;
        let n = _.count;
        let k: usize = 0;
        while (k < n) {
            level[k] = _.leaves[k];
            k += 1;
        }
        while (n > 1) {
            let nextN: usize = 0;
            let i: usize = 0;
            while (i + 1 < n) {
                level[nextN] = hashNode(level[i], level[i + 1]);
                nextN += 1;
                i += 2;
            }
            if (i < n) {
                level[nextN] = level[i];
                nextN += 1;
            }
            n = nextN;
        }
        return level[0];
    }

    pub fn generateProof(leaf_index: usize) -> MerkleProof? {
        if (_.count == 0 || leaf_index >= _.count) {
            return null;
        }
        let proof = emptyProof();
        let level: [MAX_LEAVES]Hash32 = undefined;
        let n = _.count;
        let k: usize = 0;
        while (k < n) {
            level[k] = _.leaves[k];
            k += 1;
        }

        let idx = leaf_index;
        while (n > 1) {
            const isOddNode = (idx % (2 as usize)) == (1 as usize);
            if (isOddNode) {
                proof.siblings[proof.depth] = level[idx - 1];
                proof.is_right[proof.depth] = false;
                proof.depth += 1;
            } else if (idx + 1 < n) {
                proof.siblings[proof.depth] = level[idx + 1];
                proof.is_right[proof.depth] = true;
                proof.depth += 1;
            }

            let nextN: usize = 0;
            let i: usize = 0;
            while (i + 1 < n) {
                level[nextN] = hashNode(level[i], level[i + 1]);
                nextN += 1;
                i += 2;
            }
            if (i < n) {
                level[nextN] = level[i];
                nextN += 1;
            }
            idx = idx / (2 as usize);
            n = nextN;
        }
        return proof;
    }
}

pub fn verifyProof(leaf_data: string, proof: MerkleProof, expected_root: Hash32) -> bool {
    if (proof.depth > MAX_PROOF_DEPTH) {
        return false;
    }
    let cur = hashLeaf(leaf_data);
    let i: usize = 0;
    while (i < proof.depth) {
        if (proof.is_right[i]) {
            cur = hashNode(cur, proof.siblings[i]);
        } else {
            cur = hashNode(proof.siblings[i], cur);
        }
        i += 1;
    }
    return cur.equals(expected_root);
}

export {
    Hash32,
    MerkleProof,
    MerkleTree,
    sha256,
    hashLeaf,
    hashNode,
    verifyProof,
};
