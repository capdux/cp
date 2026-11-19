// Source: /data/dev/bun/src/zlib/ & IEEE 802.3 / RFC 1950 specs
// ============================================================================
// lib/pack/checksum.cp — IEEE 802.3 CRC32 与 RFC 1950 Adler32 校验和引擎
// 用于 Gzip / Zlib / ZIP 帧完整性校验与流式增量哈希
// ============================================================================

/// IEEE 802.3 标准多项式 (0xEDB88320) CRC-32 增量与单次校验器
pub struct Crc32 {
    state: u32 = 0xFFFFFFFF,

    pub static fn init(): Crc32 {
        return Crc32{ state: 0xFFFFFFFF };
    }

    pub fn update(*_, data: []const u8) {
        let crc = _.state;
        let i: usize = 0;
        while (i < data.len) {
            crc = crc ^ (data[i] as u32);
            let bit: usize = 0;
            while (bit < 8) {
                let mask: u32 = if ((crc & 1) != 0) (0xEDB88320 as u32) else (0 as u32);
                crc = (crc >> 1) ^ mask;
                bit += 1;
            }
            i += 1;
        }
        _.state = crc;
    }

    pub fn final(): u32 {
        return _.state ^ 0xFFFFFFFF;
    }

    pub fn reset(*_) {
        _.state = 0xFFFFFFFF;
    }

    /// 单次计算字节切片的 IEEE CRC-32 值
    pub static fn checksum(data: []const u8): u32 {
        let hasher = Crc32.init();
        hasher.update(data);
        return hasher.final();
    }
}

/// RFC 1950 Zlib 标准 Adler-32 (MOD_ADLER = 65521) 增量与单次校验器
pub struct Adler32 {
    s1: u32 = 1,
    s2: u32 = 0,

    pub static fn init(): Adler32 {
        return Adler32{ s1: 1, s2: 0 };
    }

    pub fn update(*_, data: []const u8) {
        let a = _.s1;
        let b = _.s2;
        let i: usize = 0;
        while (i < data.len) {
            a = (a + (data[i] as u32)) % 65521;
            b = (b + a) % 65521;
            i += 1;
        }
        _.s1 = a;
        _.s2 = b;
    }

    pub fn final(): u32 {
        return (_.s2 << 16) | _.s1;
    }

    pub fn reset(*_) {
        _.s1 = 1;
        _.s2 = 0;
    }

    /// 单次计算字节切片的 RFC 1950 Adler-32 值
    pub static fn checksum(data: []const u8): u32 {
        let hasher = Adler32.init();
        hasher.update(data);
        return hasher.final();
    }
}
