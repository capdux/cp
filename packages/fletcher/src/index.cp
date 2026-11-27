// @cap/fletcher — Zero-heap Fletcher-16, Fletcher-32, and Fletcher-64 streaming & one-shot checksums
// Clean-room MIT implementation in pure CP (modeled on crates.io `fletcher`).
// Accelerated with 16-lane @Vector(16, u8) / @Vector(16, u32) Fletcher-16 SIMD,
// 4-lane @Vector(4, u32) Fletcher-32 SIMD, and deferred modulo reduction.

type FlVec16U8 = @Vector(16, u8);
type FlVec16U32 = @Vector(16, u32);
type FlVec4U32 = @Vector(4, u32);

const FL16_WEIGHTS: [16]u32 = [16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1];
const FL32_WEIGHTS: [4]u32 = [4, 3, 2, 1];

fn fl16UpdateSlice(sum1_ptr: *u32, sum2_ptr: *u32, data: []const u8) {
    let s1 = sum1_ptr.*;
    let s2 = sum2_ptr.*;
    let i: usize = 0;
    const n = data.len;
    const vw: FlVec16U32 = FL16_WEIGHTS;

    // 16-byte SIMD block accumulation with deferred mod-255 reduction
    while (i + 16 <= n) {
        const vb: FlVec16U8 = data[i .. i + 16][0..16].*;
        const ba: [16]u8 = vb;
        let ua: [16]u32 = undefined;
        ua[0] = ba[0] as u32;
        ua[1] = ba[1] as u32;
        ua[2] = ba[2] as u32;
        ua[3] = ba[3] as u32;
        ua[4] = ba[4] as u32;
        ua[5] = ba[5] as u32;
        ua[6] = ba[6] as u32;
        ua[7] = ba[7] as u32;
        ua[8] = ba[8] as u32;
        ua[9] = ba[9] as u32;
        ua[10] = ba[10] as u32;
        ua[11] = ba[11] as u32;
        ua[12] = ba[12] as u32;
        ua[13] = ba[13] as u32;
        ua[14] = ba[14] as u32;
        ua[15] = ba[15] as u32;
        const vu: FlVec16U32 = ua;
        const sum_b: u32 = @reduce(.Add, vu);
        const weighted: u32 = @reduce(.Add, vu * vw);
        s2 = (s2 + s1 * (16 as u32) + weighted) % (255 as u32);
        s1 = (s1 + sum_b) % (255 as u32);
        i += 16;
    }

    while (i < n) {
        s1 += data[i] as u32;
        s2 += s1;
        i += 1;
    }
    sum1_ptr.* = s1 % (255 as u32);
    sum2_ptr.* = s2 % (255 as u32);
}

fn fl32UpdateBytesCore(sum1_ptr: *u32, sum2_ptr: *u32, pending_ptr: *u8, has_pending_ptr: *bool, data: []const u8) {
    let i: usize = 0;
    const n = data.len;
    if (n == 0) { return; }
    let s1 = sum1_ptr.*;
    let s2 = sum2_ptr.*;
    if (has_pending_ptr.*) {
        const word: u32 = (pending_ptr.* as u32) | ((data[0] as u32) << 8);
        s1 = (s1 + word) % (65535 as u32);
        s2 = (s2 + s1) % (65535 as u32);
        has_pending_ptr.* = false;
        i = 1;
    }

    // 4-word (8-byte) SIMD block accumulation with deferred mod-65535 reduction
    const vw: FlVec4U32 = FL32_WEIGHTS;
    while (i + 8 <= n) {
        let wa: [4]u32 = undefined;
        wa[0] = (data[i] as u32) | ((data[i + 1] as u32) << 8);
        wa[1] = (data[i + 2] as u32) | ((data[i + 3] as u32) << 8);
        wa[2] = (data[i + 4] as u32) | ((data[i + 5] as u32) << 8);
        wa[3] = (data[i + 6] as u32) | ((data[i + 7] as u32) << 8);
        const vw_cur: FlVec4U32 = wa;
        const sum_w: u32 = @reduce(.Add, vw_cur);
        const weighted: u32 = @reduce(.Add, vw_cur * vw);
        s2 = (s2 + s1 * (4 as u32) + weighted) % (65535 as u32);
        s1 = (s1 + sum_w) % (65535 as u32);
        i += 8;
    }

    while (i + 1 < n) {
        const word: u32 = (data[i] as u32) | ((data[i + 1] as u32) << 8);
        s1 = (s1 + word) % (65535 as u32);
        s2 = (s2 + s1) % (65535 as u32);
        i += 2;
    }
    if (i < n) {
        pending_ptr.* = data[i];
        has_pending_ptr.* = true;
    }
    sum1_ptr.* = s1;
    sum2_ptr.* = s2;
}

pub struct Fletcher16 {
    pub sum1: u32,
    pub sum2: u32,

    pub static fn new(): Fletcher16 {
        return Fletcher16 { sum1: 0, sum2: 0 };
    }

    pub static fn init(): Fletcher16 {
        return Fletcher16.new();
    }

    pub static fn withInitialValues(a: u8, b: u8): Fletcher16 {
        return Fletcher16 { sum1: (a as u32) % 255, sum2: (b as u32) % 255 };
    }

    pub fn update(*_, data: string) {
        fl16UpdateSlice(&_.sum1, &_.sum2, data);
    }

    pub fn updateBytes(*_, data: []const u8) {
        fl16UpdateSlice(&_.sum1, &_.sum2, data);
    }

    pub fn digest(): u16 {
        return (((_.sum2 & 0xff) << 8) | (_.sum1 & 0xff)) as u16;
    }

    pub fn value(): u16 {
        return _.digest();
    }

    pub fn reset(*_) {
        _.sum1 = 0;
        _.sum2 = 0;
    }
}

/// Streaming Fletcher-32 checksum over 16-bit little-endian words or arbitrary byte/string chunks.
pub struct Fletcher32 {
    pub sum1: u32,
    pub sum2: u32,
    pub pending: u8,
    pub has_pending: bool,

    pub static fn new(): Fletcher32 {
        return Fletcher32 { sum1: 0, sum2: 0, pending: 0, has_pending: false };
    }

    pub static fn init(): Fletcher32 {
        return Fletcher32.new();
    }

    pub static fn withInitialValues(a: u16, b: u16): Fletcher32 {
        return Fletcher32 { sum1: (a as u32) % 65535, sum2: (b as u32) % 65535, pending: 0, has_pending: false };
    }

    pub fn updateWords(*_, words: []const u16) {
        let s1 = _.sum1;
        let s2 = _.sum2;
        let i: usize = 0;
        const vw: FlVec4U32 = FL32_WEIGHTS;
        while (i + 4 <= words.len) {
            let wa: [4]u32 = undefined;
            wa[0] = words[i] as u32;
            wa[1] = words[i + 1] as u32;
            wa[2] = words[i + 2] as u32;
            wa[3] = words[i + 3] as u32;
            const vw_cur: FlVec4U32 = wa;
            s2 = (s2 + s1 * (4 as u32) + @reduce(.Add, vw_cur * vw)) % (65535 as u32);
            s1 = (s1 + @reduce(.Add, vw_cur)) % (65535 as u32);
            i += 4;
        }
        while (i < words.len) {
            s1 = (s1 + (words[i] as u32)) % 65535;
            s2 = (s2 + s1) % 65535;
            i = i + 1;
        }
        _.sum1 = s1;
        _.sum2 = s2;
    }

    pub fn updateBytes(*_, data: []const u8) {
        fl32UpdateBytesCore(&_.sum1, &_.sum2, &_.pending, &_.has_pending, data);
    }

    pub fn update(*_, data: string) {
        fl32UpdateBytesCore(&_.sum1, &_.sum2, &_.pending, &_.has_pending, data);
    }

    pub fn digest(): u32 {
        let s1 = _.sum1;
        let s2 = _.sum2;
        if (_.has_pending) {
            s1 = (s1 + (_.pending as u32)) % 65535;
            s2 = (s2 + s1) % 65535;
        }
        return ((s2 & 0xffff) << 16) | (s1 & 0xffff);
    }

    pub fn value(): u32 {
        return _.digest();
    }

    pub fn reset(*_) {
        _.sum1 = 0;
        _.sum2 = 0;
        _.pending = 0;
        _.has_pending = false;
    }
}

/// Streaming Fletcher-64 checksum over 32-bit little-endian words or arbitrary byte/string chunks.
pub struct Fletcher64 {
    pub sum1: u64,
    pub sum2: u64,
    pub pending: [4]u8,
    pub pending_len: usize,

    pub static fn new(): Fletcher64 {
        return Fletcher64 { sum1: 0, sum2: 0, pending: [0, 0, 0, 0], pending_len: 0 };
    }

    pub static fn init(): Fletcher64 {
        return Fletcher64.new();
    }

    pub static fn withInitialValues(a: u32, b: u32): Fletcher64 {
        let mod64: u64 = 0xffffffff;
        return Fletcher64 { sum1: (a as u64) % mod64, sum2: (b as u64) % mod64, pending: [0, 0, 0, 0], pending_len: 0 };
    }

    pub fn updateWords(*_, words: []const u32) {
        let mod64: u64 = 0xffffffff;
        let i: usize = 0;
        while (i < words.len) {
            _.sum1 = (_.sum1 + (words[i] as u64)) % mod64;
            _.sum2 = (_.sum2 + _.sum1) % mod64;
            i = i + 1;
        }
    }

    pub fn updateBytes(*_, data: []const u8) {
        let mod64: u64 = 0xffffffff;
        let i: usize = 0;
        let n = data.len;
        if (_.pending_len > 0) {
            while (_.pending_len < 4 && i < n) {
                _.pending[_.pending_len] = data[i];
                _.pending_len = _.pending_len + 1;
                i = i + 1;
            }
            if (_.pending_len == 4) {
                let word: u64 = (_.pending[0] as u64)
                    | ((_.pending[1] as u64) << 8)
                    | ((_.pending[2] as u64) << 16)
                    | ((_.pending[3] as u64) << 24);
                _.sum1 = (_.sum1 + word) % mod64;
                _.sum2 = (_.sum2 + _.sum1) % mod64;
                _.pending_len = 0;
            }
        }
        while (i + 3 < n) {
            let word: u64 = (data[i] as u64)
                | ((data[i + 1] as u64) << 8)
                | ((data[i + 2] as u64) << 16)
                | ((data[i + 3] as u64) << 24);
            _.sum1 = (_.sum1 + word) % mod64;
            _.sum2 = (_.sum2 + _.sum1) % mod64;
            i = i + 4;
        }
        while (i < n) {
            _.pending[_.pending_len] = data[i];
            _.pending_len = _.pending_len + 1;
            i = i + 1;
        }
    }

    pub fn update(*_, data: string) {
        let mod64: u64 = 0xffffffff;
        let i: usize = 0;
        let n = data.len;
        if (_.pending_len > 0) {
            while (_.pending_len < 4 && i < n) {
                _.pending[_.pending_len] = data[i];
                _.pending_len = _.pending_len + 1;
                i = i + 1;
            }
            if (_.pending_len == 4) {
                let word: u64 = (_.pending[0] as u64)
                    | ((_.pending[1] as u64) << 8)
                    | ((_.pending[2] as u64) << 16)
                    | ((_.pending[3] as u64) << 24);
                _.sum1 = (_.sum1 + word) % mod64;
                _.sum2 = (_.sum2 + _.sum1) % mod64;
                _.pending_len = 0;
            }
        }
        while (i + 3 < n) {
            let word: u64 = (data[i] as u64)
                | ((data[i + 1] as u64) << 8)
                | ((data[i + 2] as u64) << 16)
                | ((data[i + 3] as u64) << 24);
            _.sum1 = (_.sum1 + word) % mod64;
            _.sum2 = (_.sum2 + _.sum1) % mod64;
            i = i + 4;
        }
        while (i < n) {
            _.pending[_.pending_len] = data[i];
            _.pending_len = _.pending_len + 1;
            i = i + 1;
        }
    }

    pub fn digest(): u64 {
        let mod64: u64 = 0xffffffff;
        let s1 = _.sum1;
        let s2 = _.sum2;
        if (_.pending_len > 0) {
            let word: u64 = 0;
            let shift: u64 = 0;
            let k: usize = 0;
            while (k < _.pending_len) {
                word = word | ((_.pending[k] as u64) << (shift as u6));
                shift = shift + 8;
                k = k + 1;
            }
            s1 = (s1 + word) % mod64;
            s2 = (s2 + s1) % mod64;
        }
        return ((s2 & mod64) << 32) | (s1 & mod64);
    }

    pub fn value(): u64 {
        return _.digest();
    }

    pub fn reset(*_) {
        _.sum1 = 0;
        _.sum2 = 0;
        _.pending_len = 0;
    }
}

pub fn fletcher16(data: string): u16 {
    let h = Fletcher16.new();
    h.update(data);
    return h.digest();
}

pub fn fletcher16Bytes(data: []const u8): u16 {
    let h = Fletcher16.new();
    h.updateBytes(data);
    return h.digest();
}

/// Upstream `fletcher::calc_fletcher16` one-shot byte checksum.
pub fn calcFletcher16(data: []const u8): u16 {
    return fletcher16Bytes(data);
}

pub fn fletcher32(data: string): u32 {
    let h = Fletcher32.new();
    h.update(data);
    return h.digest();
}

pub fn fletcher32Bytes(data: []const u8): u32 {
    let h = Fletcher32.new();
    h.updateBytes(data);
    return h.digest();
}

/// Upstream `fletcher::calc_fletcher32` one-shot 16-bit word checksum.
pub fn calcFletcher32(words: []const u16): u32 {
    let h = Fletcher32.new();
    h.updateWords(words);
    return h.digest();
}

pub fn fletcher64(data: string): u64 {
    let h = Fletcher64.new();
    h.update(data);
    return h.digest();
}

pub fn fletcher64Bytes(data: []const u8): u64 {
    let h = Fletcher64.new();
    h.updateBytes(data);
    return h.digest();
}

/// Upstream `fletcher::calc_fletcher64` one-shot 32-bit word checksum.
pub fn calcFletcher64(words: []const u32): u64 {
    let h = Fletcher64.new();
    h.updateWords(words);
    return h.digest();
}
