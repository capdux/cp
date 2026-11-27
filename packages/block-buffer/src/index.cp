// packages/@cap/block-buffer/src/index.cp
// Zero-allocation fixed-size cryptographic block streaming buffers & padding primitives.
// MIT clean-room pure CP implementation (RustCrypto `block-buffer` parity).
// Accelerated with @memcpy bulk block transfers, 16-lane @Vector(16, u8) SIMD padding fill,
// and undefined uninitialized internal buffers.

type BbVec16 = @Vector(16, u8);

fn bbFillBytes(dst: []u8, val: u8) {
    let i: usize = 0;
    const v: BbVec16 = @splat(val);
    const chunk: [16]u8 = v;
    while (i + 16 <= dst.len) {
        @memcpy(dst[i .. i + 16], chunk[0..16]);
        i += 16;
    }
    while (i < dst.len) {
        dst[i] = val;
        i += 1;
    }
}

fn bbWriteBe64(out: []u8, offset: usize, val: u64) {
    out[offset + 0] = ((val >> (56 as u6)) & (0xFF as u64)) as u8;
    out[offset + 1] = ((val >> (48 as u6)) & (0xFF as u64)) as u8;
    out[offset + 2] = ((val >> (40 as u6)) & (0xFF as u64)) as u8;
    out[offset + 3] = ((val >> (32 as u6)) & (0xFF as u64)) as u8;
    out[offset + 4] = ((val >> (24 as u6)) & (0xFF as u64)) as u8;
    out[offset + 5] = ((val >> (16 as u6)) & (0xFF as u64)) as u8;
    out[offset + 6] = ((val >> (8 as u6)) & (0xFF as u64)) as u8;
    out[offset + 7] = (val & (0xFF as u64)) as u8;
}

fn bbWriteLe64(out: []u8, offset: usize, val: u64) {
    out[offset + 0] = (val & (0xFF as u64)) as u8;
    out[offset + 1] = ((val >> (8 as u6)) & (0xFF as u64)) as u8;
    out[offset + 2] = ((val >> (16 as u6)) & (0xFF as u64)) as u8;
    out[offset + 3] = ((val >> (24 as u6)) & (0xFF as u64)) as u8;
    out[offset + 4] = ((val >> (32 as u6)) & (0xFF as u64)) as u8;
    out[offset + 5] = ((val >> (40 as u6)) & (0xFF as u64)) as u8;
    out[offset + 6] = ((val >> (48 as u6)) & (0xFF as u64)) as u8;
    out[offset + 7] = ((val >> (56 as u6)) & (0xFF as u64)) as u8;
}

/// 64-byte fixed block streaming buffer (SHA-256, SHA-1, MD5, BLAKE2s, ChaCha).
pub struct BlockBuffer64 {
    pub buf: [64]u8,
    pub pos: usize,
    pub total_bytes: u64,

    pub static fn new(): BlockBuffer64 {
        let b: [64]u8 = undefined;
        return BlockBuffer64{
            buf: b,
            pos: 0,
            total_bytes: 0 as u64,
        };
    }

    pub fn remaining(): usize {
        return (64 as usize) - _.pos;
    }

    pub fn getPos(): usize {
        return _.pos;
    }

    pub fn reset(*_) {
        _.pos = 0;
        _.total_bytes = 0 as u64;
    }

    /// Stream `data` through the 64-byte buffer, emitting complete 64-byte blocks into `out_blocks`.
    /// Returns the number of complete 64-byte blocks emitted. Remaining bytes (< 64) stay buffered.
    pub fn feed(*_, data: []const u8, out_blocks: []u8): usize {
        if (data.len == 0) {
            return 0;
        }
        const maxBlocks = out_blocks.len / 64;
        let emitted: usize = 0;
        let inPos: usize = 0;

        // 1. Complete any partially filled internal buffer first.
        if (_.pos > 0) {
            const need = (64 as usize) - _.pos;
            let take = data.len;
            if (take > need) {
                take = need;
            }
            @memcpy(_.buf[_.pos .. _.pos + take], data[0..take]);
            _.pos += take;
            inPos += take;
            _.total_bytes += take as u64;

            if (_.pos == 64) {
                if (emitted < maxBlocks) {
                    const dstOff = emitted * 64;
                    @memcpy(out_blocks[dstOff .. dstOff + 64], _.buf[0..64]);
                    emitted += 1;
                }
                _.pos = 0;
            }
        }

        // 2. Direct-copy full 64-byte blocks straight from `data` to `out_blocks` in one @memcpy.
        if (emitted < maxBlocks && inPos + 64 <= data.len) {
            const availBlocks = (data.len - inPos) / 64;
            const freeBlocks = maxBlocks - emitted;
            const bulkBlocks = if (availBlocks < freeBlocks) { availBlocks } else { freeBlocks };
            if (bulkBlocks > 0) {
                const copyBytes = bulkBlocks * 64;
                const dstOff = emitted * 64;
                @memcpy(out_blocks[dstOff .. dstOff + copyBytes], data[inPos .. inPos + copyBytes]);
                emitted += bulkBlocks;
                inPos += copyBytes;
                _.total_bytes += copyBytes as u64;
            }
        }

        // 3. Buffer remaining tail bytes (< 64).
        if (inPos < data.len) {
            let rem = data.len - inPos;
            const space = (64 as usize) - _.pos;
            if (rem > space) {
                rem = space;
            }
            if (rem > 0) {
                @memcpy(_.buf[_.pos .. _.pos + rem], data[inPos .. inPos + rem]);
                _.pos += rem;
                _.total_bytes += rem as u64;
            }
        }

        return emitted;
    }

    /// Merkle-Damgard big-endian padding (`0x80`, zero pad, 64-bit big-endian bit length).
    /// Emits 1 block (`pos < 56`) or 2 blocks (`pos >= 56`) into `out_blocks` and resets buffer.
    /// Returns number of emitted blocks (`1` or `2`), or `0` if `out_blocks` is too small.
    pub fn padMdBe64(*_, out_blocks: []u8): usize {
        const bitLen: u64 = _.total_bytes << (3 as u6);
        let blocksNeeded: usize = 2;
        if (_.pos < 56) {
            blocksNeeded = 1;
        }
        if (out_blocks.len < blocksNeeded * 64) {
            return 0;
        }

        const totalOut = blocksNeeded * 64;
        if (_.pos > 0) {
            @memcpy(out_blocks[0.._.pos], _.buf[0.._.pos]);
        }
        out_blocks[_.pos] = 0x80 as u8;
        if (_.pos + 1 < totalOut - 8) {
            bbFillBytes(out_blocks[_.pos + 1 .. totalOut - 8], 0 as u8);
        }
        bbWriteBe64(out_blocks, totalOut - 8, bitLen);
        _.reset();
        return blocksNeeded;
    }

    /// Merkle-Damgard little-endian padding (`0x80`, zero pad, 64-bit little-endian bit length; MD5).
    /// Emits 1 or 2 blocks into `out_blocks` and resets buffer. Returns block count or `0` on small buffer.
    pub fn padMdLe64(*_, out_blocks: []u8): usize {
        const bitLen: u64 = _.total_bytes << (3 as u6);
        let blocksNeeded: usize = 2;
        if (_.pos < 56) {
            blocksNeeded = 1;
        }
        if (out_blocks.len < blocksNeeded * 64) {
            return 0;
        }

        const totalOut = blocksNeeded * 64;
        if (_.pos > 0) {
            @memcpy(out_blocks[0.._.pos], _.buf[0.._.pos]);
        }
        out_blocks[_.pos] = 0x80 as u8;
        if (_.pos + 1 < totalOut - 8) {
            bbFillBytes(out_blocks[_.pos + 1 .. totalOut - 8], 0 as u8);
        }
        bbWriteLe64(out_blocks, totalOut - 8, bitLen);
        _.reset();
        return blocksNeeded;
    }

    /// PKCS#7 padding into `out_block[0..64]` (fills remaining `N = 64 - pos` bytes with `N`) and resets buffer.
    pub fn padPkcs7(*_, out_block: []u8): bool {
        if (out_block.len < 64) {
            return false;
        }
        const padByte = ((64 as usize) - _.pos) as u8;
        if (_.pos > 0) {
            @memcpy(out_block[0.._.pos], _.buf[0.._.pos]);
        }
        bbFillBytes(out_block[_.pos..64], padByte);
        _.reset();
        return true;
    }

    /// ISO/IEC 7816-4 padding into `out_block[0..64]` (`0x80` followed by `0x00` bytes) and resets buffer.
    pub fn padIso7816(*_, out_block: []u8): bool {
        if (out_block.len < 64) {
            return false;
        }
        if (_.pos > 0) {
            @memcpy(out_block[0.._.pos], _.buf[0.._.pos]);
        }
        out_block[_.pos] = 0x80 as u8;
        if (_.pos + 1 < 64) {
            bbFillBytes(out_block[_.pos + 1 .. 64], 0 as u8);
        }
        _.reset();
        return true;
    }

    /// Zero padding into `out_block[0..64]` (fills remaining bytes with `0x00`) and resets buffer.
    pub fn padZero(*_, out_block: []u8): bool {
        if (out_block.len < 64) {
            return false;
        }
        if (_.pos > 0) {
            @memcpy(out_block[0.._.pos], _.buf[0.._.pos]);
        }
        if (_.pos < 64) {
            bbFillBytes(out_block[_.pos..64], 0 as u8);
        }
        _.reset();
        return true;
    }
}

/// 128-byte fixed block streaming buffer (SHA-512, SHA-384, BLAKE2b).
pub struct BlockBuffer128 {
    pub buf: [128]u8,
    pub pos: usize,
    pub total_bytes: u64,

    pub static fn new(): BlockBuffer128 {
        let b: [128]u8 = undefined;
        return BlockBuffer128{
            buf: b,
            pos: 0,
            total_bytes: 0 as u64,
        };
    }

    pub fn remaining(): usize {
        return (128 as usize) - _.pos;
    }

    pub fn getPos(): usize {
        return _.pos;
    }

    pub fn reset(*_) {
        _.pos = 0;
        _.total_bytes = 0 as u64;
    }

    /// Stream `data` through the 128-byte buffer, emitting complete 128-byte blocks into `out_blocks`.
    /// Returns the number of complete 128-byte blocks emitted.
    pub fn feed(*_, data: []const u8, out_blocks: []u8): usize {
        if (data.len == 0) {
            return 0;
        }
        const maxBlocks = out_blocks.len / 128;
        let emitted: usize = 0;
        let inPos: usize = 0;

        if (_.pos > 0) {
            const need = (128 as usize) - _.pos;
            let take = data.len;
            if (take > need) {
                take = need;
            }
            @memcpy(_.buf[_.pos .. _.pos + take], data[0..take]);
            _.pos += take;
            inPos += take;
            _.total_bytes += take as u64;

            if (_.pos == 128) {
                if (emitted < maxBlocks) {
                    const dstOff = emitted * 128;
                    @memcpy(out_blocks[dstOff .. dstOff + 128], _.buf[0..128]);
                    emitted += 1;
                }
                _.pos = 0;
            }
        }

        if (emitted < maxBlocks && inPos + 128 <= data.len) {
            const availBlocks = (data.len - inPos) / 128;
            const freeBlocks = maxBlocks - emitted;
            const bulkBlocks = if (availBlocks < freeBlocks) { availBlocks } else { freeBlocks };
            if (bulkBlocks > 0) {
                const copyBytes = bulkBlocks * 128;
                const dstOff = emitted * 128;
                @memcpy(out_blocks[dstOff .. dstOff + copyBytes], data[inPos .. inPos + copyBytes]);
                emitted += bulkBlocks;
                inPos += copyBytes;
                _.total_bytes += copyBytes as u64;
            }
        }

        if (inPos < data.len) {
            let rem = data.len - inPos;
            const space = (128 as usize) - _.pos;
            if (rem > space) {
                rem = space;
            }
            if (rem > 0) {
                @memcpy(_.buf[_.pos .. _.pos + rem], data[inPos .. inPos + rem]);
                _.pos += rem;
                _.total_bytes += rem as u64;
            }
        }

        return emitted;
    }

    /// SHA-512 Merkle-Damgard big-endian 128-bit length padding (`pos < 112` -> 1 block, else 2 blocks).
    pub fn padMdBe64(*_, out_blocks: []u8): usize {
        const bitLenHi: u64 = _.total_bytes >> (61 as u6);
        const bitLenLo: u64 = _.total_bytes << (3 as u6);
        let blocksNeeded: usize = 2;
        if (_.pos < 112) {
            blocksNeeded = 1;
        }
        if (out_blocks.len < blocksNeeded * 128) {
            return 0;
        }

        const totalOut = blocksNeeded * 128;
        if (_.pos > 0) {
            @memcpy(out_blocks[0.._.pos], _.buf[0.._.pos]);
        }
        out_blocks[_.pos] = 0x80 as u8;
        if (_.pos + 1 < totalOut - 16) {
            bbFillBytes(out_blocks[_.pos + 1 .. totalOut - 16], 0 as u8);
        }
        bbWriteBe64(out_blocks, totalOut - 16, bitLenHi);
        bbWriteBe64(out_blocks, totalOut - 8, bitLenLo);
        _.reset();
        return blocksNeeded;
    }

    /// Little-endian 128-bit length padding (`pos < 112` -> 1 block, else 2 blocks).
    pub fn padMdLe64(*_, out_blocks: []u8): usize {
        const bitLenLo: u64 = _.total_bytes << (3 as u6);
        const bitLenHi: u64 = _.total_bytes >> (61 as u6);
        let blocksNeeded: usize = 2;
        if (_.pos < 112) {
            blocksNeeded = 1;
        }
        if (out_blocks.len < blocksNeeded * 128) {
            return 0;
        }

        const totalOut = blocksNeeded * 128;
        if (_.pos > 0) {
            @memcpy(out_blocks[0.._.pos], _.buf[0.._.pos]);
        }
        out_blocks[_.pos] = 0x80 as u8;
        if (_.pos + 1 < totalOut - 16) {
            bbFillBytes(out_blocks[_.pos + 1 .. totalOut - 16], 0 as u8);
        }
        bbWriteLe64(out_blocks, totalOut - 16, bitLenLo);
        bbWriteLe64(out_blocks, totalOut - 8, bitLenHi);
        _.reset();
        return blocksNeeded;
    }

    pub fn padPkcs7(*_, out_block: []u8): bool {
        if (out_block.len < 128) {
            return false;
        }
        const padByte = ((128 as usize) - _.pos) as u8;
        if (_.pos > 0) {
            @memcpy(out_block[0.._.pos], _.buf[0.._.pos]);
        }
        bbFillBytes(out_block[_.pos..128], padByte);
        _.reset();
        return true;
    }

    pub fn padIso7816(*_, out_block: []u8): bool {
        if (out_block.len < 128) {
            return false;
        }
        if (_.pos > 0) {
            @memcpy(out_block[0.._.pos], _.buf[0.._.pos]);
        }
        out_block[_.pos] = 0x80 as u8;
        if (_.pos + 1 < 128) {
            bbFillBytes(out_block[_.pos + 1 .. 128], 0 as u8);
        }
        _.reset();
        return true;
    }

    pub fn padZero(*_, out_block: []u8): bool {
        if (out_block.len < 128) {
            return false;
        }
        if (_.pos > 0) {
            @memcpy(out_block[0.._.pos], _.buf[0.._.pos]);
        }
        if (_.pos < 128) {
            bbFillBytes(out_block[_.pos..128], 0 as u8);
        }
        _.reset();
        return true;
    }
}

export {
    BlockBuffer64,
    BlockBuffer128,
};
