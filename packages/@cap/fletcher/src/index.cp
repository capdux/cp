// @cap/fletcher — Zero-heap Fletcher-16, Fletcher-32, and Fletcher-64 checksums
// Clean-room MIT implementation in pure CP.

pub struct Fletcher16 {
    pub sum1: u32,
    pub sum2: u32,

    pub static fn new(): Fletcher16 {
        return Fletcher16 { sum1: 0, sum2: 0 };
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

    pub fn updateBytes(*_, data: []u8) {
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
}

pub fn fletcher16(data: string): u16 {
    let h = Fletcher16.new();
    h.update(data);
    return h.digest();
}

pub fn fletcher16Bytes(data: []u8): u16 {
    let h = Fletcher16.new();
    h.updateBytes(data);
    return h.digest();
}

pub fn fletcher32(data: string): u32 {
    let sum1: u32 = 0;
    let sum2: u32 = 0;
    let i: usize = 0;
    let n = data.len;
    while (i + 1 < n) {
        let word: u32 = (data[i] as u32) | ((data[i + 1] as u32) << 8);
        sum1 = (sum1 + word) % 65535;
        sum2 = (sum2 + sum1) % 65535;
        i = i + 2;
    }
    if (i < n) {
        let word: u32 = data[i] as u32;
        sum1 = (sum1 + word) % 65535;
        sum2 = (sum2 + sum1) % 65535;
    }
    return ((sum2 & 0xffff) << 16) | (sum1 & 0xffff);
}

pub fn fletcher32Bytes(data: []u8): u32 {
    let sum1: u32 = 0;
    let sum2: u32 = 0;
    let i: usize = 0;
    let n = data.len;
    while (i + 1 < n) {
        let word: u32 = (data[i] as u32) | ((data[i + 1] as u32) << 8);
        sum1 = (sum1 + word) % 65535;
        sum2 = (sum2 + sum1) % 65535;
        i = i + 2;
    }
    if (i < n) {
        let word: u32 = data[i] as u32;
        sum1 = (sum1 + word) % 65535;
        sum2 = (sum2 + sum1) % 65535;
    }
    return ((sum2 & 0xffff) << 16) | (sum1 & 0xffff);
}

pub fn fletcher64(data: string): u64 {
    let mod64: u64 = 0xffffffff;
    let sum1: u64 = 0;
    let sum2: u64 = 0;
    let i: usize = 0;
    let n = data.len;
    while (i + 3 < n) {
        let word: u64 = (data[i] as u64)
            | ((data[i + 1] as u64) << 8)
            | ((data[i + 2] as u64) << 16)
            | ((data[i + 3] as u64) << 24);
        sum1 = (sum1 + word) % mod64;
        sum2 = (sum2 + sum1) % mod64;
        i = i + 4;
    }
    if (i < n) {
        let word: u64 = 0;
        let shift: u64 = 0;
        while (i < n) {
            word = word | ((data[i] as u64) << (shift as u6));
            shift = shift + 8;
            i = i + 1;
        }
        sum1 = (sum1 + word) % mod64;
        sum2 = (sum2 + sum1) % mod64;
    }
    return ((sum2 & mod64) << 32) | (sum1 & mod64);
}
