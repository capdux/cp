pub fn zeroizeBytes(buf: []u8): usize {
    let i: usize = 0;
    while (i < buf.len) {
        buf[i] = 0;
        i = i + 1;
    }
    return buf.len;
}

pub fn zeroizeU32(buf: []u32): usize {
    let i: usize = 0;
    while (i < buf.len) {
        buf[i] = 0;
        i = i + 1;
    }
    return buf.len;
}

pub fn zeroizeU64(buf: []u64): usize {
    let i: usize = 0;
    while (i < buf.len) {
        buf[i] = 0;
        i = i + 1;
    }
    return buf.len;
}

pub fn isAllZeroBytes(buf: []u8): bool {
    let acc: u8 = 0;
    let i: usize = 0;
    while (i < buf.len) {
        acc = acc | buf[i];
        i = i + 1;
    }
    return acc == 0;
}

pub struct ZeroizingKey32 {
    pub bytes: [32]u8,
    pub active: bool,

    pub static fn fromSlice(src: []u8): ZeroizingKey32? {
        if (src.len != 32) { return null; }
        let buf: [32]u8 = [0; 32];
        let i: usize = 0;
        while (i < 32) {
            buf[i] = src[i];
            i = i + 1;
        }
        return ZeroizingKey32 { bytes: buf, active: true };
    }

    pub fn zeroize(*_) {
        let i: usize = 0;
        while (i < 32) {
            _.bytes[i] = 0;
            i = i + 1;
        }
        _.active = false;
    }

    pub fn isZeroed(): bool {
        let acc: u8 = 0;
        let i: usize = 0;
        while (i < 32) {
            acc = acc | _.bytes[i];
            i = i + 1;
        }
        return acc == 0 && !_.active;
    }
}

pub struct ZeroizingBuf64 {
    pub bytes: [64]u8,
    pub len: usize,
    pub active: bool,

    pub static fn fromSlice(src: []u8): ZeroizingBuf64? {
        if (src.len > 64) { return null; }
        let buf: [64]u8 = [0; 64];
        let i: usize = 0;
        while (i < src.len) {
            buf[i] = src[i];
            i = i + 1;
        }
        return ZeroizingBuf64 { bytes: buf, len: src.len, active: true };
    }

    pub fn zeroize(*_) {
        let i: usize = 0;
        while (i < 64) {
            _.bytes[i] = 0;
            i = i + 1;
        }
        _.len = 0;
        _.active = false;
    }

    pub fn isZeroed(): bool {
        let acc: u8 = 0;
        let i: usize = 0;
        while (i < 64) {
            acc = acc | _.bytes[i];
            i = i + 1;
        }
        return acc == 0 && !_.active && _.len == 0;
    }
}
