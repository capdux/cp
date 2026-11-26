// @cap/ghash-gf128 — Zero-allocation NIST SP 800-38D GF(2^128) GHASH universal hash
// Polynomial: x^128 + x^7 + x^2 + x + 1 (R = 0xE100000000000000)
// MIT License

const R_HI: u64 = (0xE1000000 as u64) << 32;

fn bitToMask64(bit: u64): u64 {
    let b: u64 = bit & 1;
    let m: u64 = b | (b << 1);
    m = m | (m << 2);
    m = m | (m << 4);
    m = m | (m << 8);
    m = m | (m << 16);
    m = m | (m << 32);
    return m;
}

fn readBeU64(buf: []const u8, off: usize): u64 {
    let b0: u64 = (buf[off] as u64) << 56;
    let b1: u64 = (buf[off + 1] as u64) << 48;
    let b2: u64 = (buf[off + 2] as u64) << 40;
    let b3: u64 = (buf[off + 3] as u64) << 32;
    let b4: u64 = (buf[off + 4] as u64) << 24;
    let b5: u64 = (buf[off + 5] as u64) << 16;
    let b6: u64 = (buf[off + 6] as u64) << 8;
    let b7: u64 = buf[off + 7] as u64;
    return b0 | b1 | b2 | b3 | b4 | b5 | b6 | b7;
}

fn writeBeU64(val: u64, out: []u8, off: usize) {
    out[off] = ((val >> 56) & 0xFF) as u8;
    out[off + 1] = ((val >> 48) & 0xFF) as u8;
    out[off + 2] = ((val >> 40) & 0xFF) as u8;
    out[off + 3] = ((val >> 32) & 0xFF) as u8;
    out[off + 4] = ((val >> 24) & 0xFF) as u8;
    out[off + 5] = ((val >> 16) & 0xFF) as u8;
    out[off + 6] = ((val >> 8) & 0xFF) as u8;
    out[off + 7] = (val & 0xFF) as u8;
}

pub struct Block128 {
    pub hi: u64,
    pub lo: u64,

    pub static fn zero(): Block128 {
        return Block128 { hi: 0, lo: 0 };
    }

    pub static fn one(): Block128 {
        // GF(2^128) multiplicative identity in GCM bit order is 0x8000...0000
        return Block128 { hi: (1 as u64) << 63, lo: 0 };
    }

    pub static fn fromBytes(b: []const u8): Block128 {
        if (b.len < 16) {
            let tmp: [16]u8 = [0; 16];
            let i: usize = 0;
            while (i < b.len) {
                tmp[i] = b[i];
                i = i + 1;
            }
            return Block128 {
                hi: readBeU64(tmp[0..16], 0),
                lo: readBeU64(tmp[0..16], 8),
            };
        }
        return Block128 {
            hi: readBeU64(b, 0),
            lo: readBeU64(b, 8),
        };
    }

    pub fn toBytes(out: []u8) {
        if (out.len < 16) {
            return;
        }
        writeBeU64(_.hi, out, 0);
        writeBeU64(_.lo, out, 8);
    }

    pub fn xor(other: Block128): Block128 {
        return Block128 {
            hi: _.hi ^ other.hi,
            lo: _.lo ^ other.lo,
        };
    }

    pub fn eq(other: Block128): bool {
        return _.hi == other.hi && _.lo == other.lo;
    }
}

pub fn gf128Mul(x: Block128, y: Block128): Block128 {
    let z_hi: u64 = 0;
    let z_lo: u64 = 0;
    let v_hi: u64 = y.hi;
    let v_lo: u64 = y.lo;

    // Rounds 0..63 from x.hi (MSB to LSB)
    let i: u32 = 0;
    while (i < 64) {
        let shift: u32 = (63 as u32) - i;
        let bit: u64 = (x.hi >> shift) & 1;
        let mask: u64 = bitToMask64(bit);
        z_hi = z_hi ^ (v_hi & mask);
        z_lo = z_lo ^ (v_lo & mask);

        let lsb_mask: u64 = bitToMask64(v_lo & 1);
        v_lo = (v_lo >> 1) | ((v_hi & 1) << 63);
        v_hi = (v_hi >> 1) ^ (R_HI & lsb_mask);
        i = i + 1;
    }

    // Rounds 64..127 from x.lo (MSB to LSB)
    let j: u32 = 0;
    while (j < 64) {
        let shift: u32 = (63 as u32) - j;
        let bit: u64 = (x.lo >> shift) & 1;
        let mask: u64 = bitToMask64(bit);
        z_hi = z_hi ^ (v_hi & mask);
        z_lo = z_lo ^ (v_lo & mask);

        let lsb_mask: u64 = bitToMask64(v_lo & 1);
        v_lo = (v_lo >> 1) | ((v_hi & 1) << 63);
        v_hi = (v_hi >> 1) ^ (R_HI & lsb_mask);
        j = j + 1;
    }

    return Block128 { hi: z_hi, lo: z_lo };
}

pub struct Ghash {
    pub h: Block128,
    pub y: Block128,

    pub static fn new(hash_key_16: []const u8): Ghash {
        return Ghash {
            h: Block128.fromBytes(hash_key_16),
            y: Block128.zero(),
        };
    }

    pub fn reset(*_) {
        _.y = Block128.zero();
    }

    pub fn updateBlock(*_, block_16: []const u8) {
        let blk: Block128 = Block128.fromBytes(block_16);
        _.y = gf128Mul(_.y.xor(blk), _.h);
    }

    pub fn updatePadded(*_, data: []const u8) {
        let pos: usize = 0;
        while (pos + 16 <= data.len) {
            _.updateBlock(data[pos..(pos + 16)]);
            pos = pos + 16;
        }
        if (pos < data.len) {
            let tail: [16]u8 = [0; 16];
            let rem: usize = data.len - pos;
            let k: usize = 0;
            while (k < rem) {
                tail[k] = data[pos + k];
                k = k + 1;
            }
            _.updateBlock(tail[0..16]);
        }
    }

    pub fn updateLengths(*_, aad_bytes: u64, ct_bytes: u64) {
        let len_block: Block128 = Block128 {
            hi: aad_bytes << 3,
            lo: ct_bytes << 3,
        };
        _.y = gf128Mul(_.y.xor(len_block), _.h);
    }

    pub fn finalize(out_16: []u8) {
        _.y.toBytes(out_16);
    }
}
