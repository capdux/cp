// @cap/ctr — Zero-allocation NIST SP 800-38A CTR stream cipher modes
// Ported from crates.io:ctr (https://github.com/RustCrypto/block-modes)

import { Aes128, Aes256 } from "../../aes/src/index.cp";

fn ctrZeroBlock(): [16]u8 {
    const z: u8 = 0;
    const blk: [16]u8 = [z, z, z, z, z, z, z, z, z, z, z, z, z, z, z, z];
    return blk;
}

fn ctrAddBe128(iv: [16]u8, delta: u64): [16]u8 {
    let out: [16]u8 = ctrZeroBlock();
    let j: usize = 0;
    while (j < 16) {
        out[j] = iv[j];
        j += 1;
    }
    let carry: u64 = delta;
    let k: usize = 0;
    while (k < 16 && carry > 0) {
        const idx: usize = 15 - k;
        const sum: u64 = (out[idx] as u64) + (carry & 0xFF);
        out[idx] = (sum & 0xFF) as u8;
        carry = (carry >> 8) + (sum >> 8);
        k += 1;
    }
    return out;
}

fn ctrAddLe128(iv: [16]u8, delta: u64): [16]u8 {
    let out: [16]u8 = ctrZeroBlock();
    let j: usize = 0;
    while (j < 16) {
        out[j] = iv[j];
        j += 1;
    }
    let carry: u64 = delta;
    let k: usize = 0;
    while (k < 16 && carry > 0) {
        const sum: u64 = (out[k] as u64) + (carry & 0xFF);
        out[k] = (sum & 0xFF) as u8;
        carry = (carry >> 8) + (sum >> 8);
        k += 1;
    }
    return out;
}

/// AES-128 in CTR mode with 128-bit big-endian counter (NIST SP 800-38A).
pub struct Aes128Ctr128Be {
    cipher: Aes128,
    iv: [16]u8,
    block_idx: u64,
    buf: [16]u8,
    buf_pos: usize,

    static fn new(key: [16]u8, iv: [16]u8): Aes128Ctr128Be {
        return Aes128Ctr128Be{
            cipher: Aes128.new(key),
            iv: iv,
            block_idx: 0,
            buf: ctrZeroBlock(),
            buf_pos: 0,
        };
    }

    fn currentPos(): u64 {
        if (_.buf_pos == 0) {
            return _.block_idx << 4;
        }
        return ((_.block_idx - 1) << 4) + (_.buf_pos as u64);
    }

    fn seek(*_, byte_pos: u64) {
        _.block_idx = byte_pos >> 4;
        const off: usize = (byte_pos & 15) as usize;
        if (off == 0) {
            _.buf_pos = 0;
        } else {
            const ctr = ctrAddBe128(_.iv, _.block_idx);
            _.buf = _.cipher.encryptBlock(ctr);
            _.block_idx += 1;
            _.buf_pos = off;
        }
    }

    fn applyKeystream(*_, data: []u8) {
        let i: usize = 0;
        if (_.buf_pos > 0) {
            while (i < data.len && _.buf_pos < 16) {
                data[i] ^= _.buf[_.buf_pos];
                i += 1;
                _.buf_pos += 1;
            }
            if (_.buf_pos == 16) {
                _.buf_pos = 0;
            }
        }
        while (i < data.len) {
            const ctr = ctrAddBe128(_.iv, _.block_idx);
            const ks = _.cipher.encryptBlock(ctr);
            _.block_idx += 1;
            if (data.len - i >= 16) {
                let k: usize = 0;
                while (k < 16) {
                    data[i + k] ^= ks[k];
                    k += 1;
                }
                i += 16;
            } else {
                _.buf = ks;
                let k: usize = 0;
                while (i < data.len) {
                    data[i] ^= ks[k];
                    i += 1;
                    k += 1;
                }
                _.buf_pos = k;
            }
        }
    }
}

/// AES-128 in CTR mode with 128-bit little-endian counter.
pub struct Aes128Ctr128Le {
    cipher: Aes128,
    iv: [16]u8,
    block_idx: u64,
    buf: [16]u8,
    buf_pos: usize,

    static fn new(key: [16]u8, iv: [16]u8): Aes128Ctr128Le {
        return Aes128Ctr128Le{
            cipher: Aes128.new(key),
            iv: iv,
            block_idx: 0,
            buf: ctrZeroBlock(),
            buf_pos: 0,
        };
    }

    fn currentPos(): u64 {
        if (_.buf_pos == 0) {
            return _.block_idx << 4;
        }
        return ((_.block_idx - 1) << 4) + (_.buf_pos as u64);
    }

    fn seek(*_, byte_pos: u64) {
        _.block_idx = byte_pos >> 4;
        const off: usize = (byte_pos & 15) as usize;
        if (off == 0) {
            _.buf_pos = 0;
        } else {
            const ctr = ctrAddLe128(_.iv, _.block_idx);
            _.buf = _.cipher.encryptBlock(ctr);
            _.block_idx += 1;
            _.buf_pos = off;
        }
    }

    fn applyKeystream(*_, data: []u8) {
        let i: usize = 0;
        if (_.buf_pos > 0) {
            while (i < data.len && _.buf_pos < 16) {
                data[i] ^= _.buf[_.buf_pos];
                i += 1;
                _.buf_pos += 1;
            }
            if (_.buf_pos == 16) {
                _.buf_pos = 0;
            }
        }
        while (i < data.len) {
            const ctr = ctrAddLe128(_.iv, _.block_idx);
            const ks = _.cipher.encryptBlock(ctr);
            _.block_idx += 1;
            if (data.len - i >= 16) {
                let k: usize = 0;
                while (k < 16) {
                    data[i + k] ^= ks[k];
                    k += 1;
                }
                i += 16;
            } else {
                _.buf = ks;
                let k: usize = 0;
                while (i < data.len) {
                    data[i] ^= ks[k];
                    i += 1;
                    k += 1;
                }
                _.buf_pos = k;
            }
        }
    }
}

/// AES-256 in CTR mode with 128-bit big-endian counter (NIST SP 800-38A).
pub struct Aes256Ctr128Be {
    cipher: Aes256,
    iv: [16]u8,
    block_idx: u64,
    buf: [16]u8,
    buf_pos: usize,

    static fn new(key: [32]u8, iv: [16]u8): Aes256Ctr128Be {
        return Aes256Ctr128Be{
            cipher: Aes256.new(key),
            iv: iv,
            block_idx: 0,
            buf: ctrZeroBlock(),
            buf_pos: 0,
        };
    }

    fn currentPos(): u64 {
        if (_.buf_pos == 0) {
            return _.block_idx << 4;
        }
        return ((_.block_idx - 1) << 4) + (_.buf_pos as u64);
    }

    fn seek(*_, byte_pos: u64) {
        _.block_idx = byte_pos >> 4;
        const off: usize = (byte_pos & 15) as usize;
        if (off == 0) {
            _.buf_pos = 0;
        } else {
            const ctr = ctrAddBe128(_.iv, _.block_idx);
            _.buf = _.cipher.encryptBlock(ctr);
            _.block_idx += 1;
            _.buf_pos = off;
        }
    }

    fn applyKeystream(*_, data: []u8) {
        let i: usize = 0;
        if (_.buf_pos > 0) {
            while (i < data.len && _.buf_pos < 16) {
                data[i] ^= _.buf[_.buf_pos];
                i += 1;
                _.buf_pos += 1;
            }
            if (_.buf_pos == 16) {
                _.buf_pos = 0;
            }
        }
        while (i < data.len) {
            const ctr = ctrAddBe128(_.iv, _.block_idx);
            const ks = _.cipher.encryptBlock(ctr);
            _.block_idx += 1;
            if (data.len - i >= 16) {
                let k: usize = 0;
                while (k < 16) {
                    data[i + k] ^= ks[k];
                    k += 1;
                }
                i += 16;
            } else {
                _.buf = ks;
                let k: usize = 0;
                while (i < data.len) {
                    data[i] ^= ks[k];
                    i += 1;
                    k += 1;
                }
                _.buf_pos = k;
            }
        }
    }
}

/// AES-256 in CTR mode with 128-bit little-endian counter.
pub struct Aes256Ctr128Le {
    cipher: Aes256,
    iv: [16]u8,
    block_idx: u64,
    buf: [16]u8,
    buf_pos: usize,

    static fn new(key: [32]u8, iv: [16]u8): Aes256Ctr128Le {
        return Aes256Ctr128Le{
            cipher: Aes256.new(key),
            iv: iv,
            block_idx: 0,
            buf: ctrZeroBlock(),
            buf_pos: 0,
        };
    }

    fn currentPos(): u64 {
        if (_.buf_pos == 0) {
            return _.block_idx << 4;
        }
        return ((_.block_idx - 1) << 4) + (_.buf_pos as u64);
    }

    fn seek(*_, byte_pos: u64) {
        _.block_idx = byte_pos >> 4;
        const off: usize = (byte_pos & 15) as usize;
        if (off == 0) {
            _.buf_pos = 0;
        } else {
            const ctr = ctrAddLe128(_.iv, _.block_idx);
            _.buf = _.cipher.encryptBlock(ctr);
            _.block_idx += 1;
            _.buf_pos = off;
        }
    }

    fn applyKeystream(*_, data: []u8) {
        let i: usize = 0;
        if (_.buf_pos > 0) {
            while (i < data.len && _.buf_pos < 16) {
                data[i] ^= _.buf[_.buf_pos];
                i += 1;
                _.buf_pos += 1;
            }
            if (_.buf_pos == 16) {
                _.buf_pos = 0;
            }
        }
        while (i < data.len) {
            const ctr = ctrAddLe128(_.iv, _.block_idx);
            const ks = _.cipher.encryptBlock(ctr);
            _.block_idx += 1;
            if (data.len - i >= 16) {
                let k: usize = 0;
                while (k < 16) {
                    data[i + k] ^= ks[k];
                    k += 1;
                }
                i += 16;
            } else {
                _.buf = ks;
                let k: usize = 0;
                while (i < data.len) {
                    data[i] ^= ks[k];
                    i += 1;
                    k += 1;
                }
                _.buf_pos = k;
            }
        }
    }
}
