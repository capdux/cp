// @cap/fletcher — Zero-heap Fletcher-16, Fletcher-32, and Fletcher-64 streaming & one-shot checksums
// Clean-room MIT implementation in pure CP (modeled on crates.io `fletcher`).

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
        let i: usize = 0;
        let n = data.len;
        while (i < n) {
            _.sum1 = (_.sum1 + (data[i] as u32)) % 255;
            _.sum2 = (_.sum2 + _.sum1) % 255;
            i = i + 1;
        }
    }

    pub fn updateBytes(*_, data: []const u8) {
        let i: usize = 0;
        let n = data.len;
        while (i < n) {
            _.sum1 = (_.sum1 + (data[i] as u32)) % 255;
            _.sum2 = (_.sum2 + _.sum1) % 255;
            i = i + 1;
        }
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
        let i: usize = 0;
        while (i < words.len) {
            _.sum1 = (_.sum1 + (words[i] as u32)) % 65535;
            _.sum2 = (_.sum2 + _.sum1) % 65535;
            i = i + 1;
        }
    }

    pub fn updateBytes(*_, data: []const u8) {
        let i: usize = 0;
        let n = data.len;
        if (n == 0) { return; }
        if (_.has_pending) {
            let word: u32 = (_.pending as u32) | ((data[0] as u32) << 8);
            _.sum1 = (_.sum1 + word) % 65535;
            _.sum2 = (_.sum2 + _.sum1) % 65535;
            _.has_pending = false;
            i = 1;
        }
        while (i + 1 < n) {
            let word: u32 = (data[i] as u32) | ((data[i + 1] as u32) << 8);
            _.sum1 = (_.sum1 + word) % 65535;
            _.sum2 = (_.sum2 + _.sum1) % 65535;
            i = i + 2;
        }
        if (i < n) {
            _.pending = data[i];
            _.has_pending = true;
        }
    }

    pub fn update(*_, data: string) {
        let i: usize = 0;
        let n = data.len;
        if (n == 0) { return; }
        if (_.has_pending) {
            let word: u32 = (_.pending as u32) | ((data[0] as u32) << 8);
            _.sum1 = (_.sum1 + word) % 65535;
            _.sum2 = (_.sum2 + _.sum1) % 65535;
            _.has_pending = false;
            i = 1;
        }
        while (i + 1 < n) {
            let word: u32 = (data[i] as u32) | ((data[i + 1] as u32) << 8);
            _.sum1 = (_.sum1 + word) % 65535;
            _.sum2 = (_.sum2 + _.sum1) % 65535;
            i = i + 2;
        }
        if (i < n) {
            _.pending = data[i];
            _.has_pending = true;
        }
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
