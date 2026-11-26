pub const CRC32_POLY_REVERSED: u32 = 3988292384; // 0xEDB88320

fn crcByteStep(crc_in: u32, b: u8): u32 {
    let c: u32 = crc_in ^ (b as u32);
    let bit: usize = 0;
    while (bit < 8) {
        if ((c & 1) != 0) {
            c = (c >> 1) ^ CRC32_POLY_REVERSED;
        } else {
            c = c >> 1;
        }
        bit = bit + 1;
    }
    return c;
}

fn crcGf2MatrixTimes(mat: [32]u32, vec: u32): u32 {
    let sum: u32 = 0;
    let v = vec;
    let idx: usize = 0;
    while (v != 0 && idx < 32) {
        if ((v & 1) != 0) {
            sum = sum ^ mat[idx];
        }
        v = v >> 1;
        idx = idx + 1;
    }
    return sum;
}

fn crcGf2MatrixSquare(src: [32]u32): [32]u32 {
    let out: [32]u32 = [0; 32];
    let n: usize = 0;
    while (n < 32) {
        out[n] = crcGf2MatrixTimes(src, src[n]);
        n = n + 1;
    }
    return out;
}

pub fn combineCrc32(crc1: u32, crc2: u32, len2: u64): u32 {
    if (len2 == 0) { return crc1; }
    let odd: [32]u32 = [0; 32];
    odd[0] = CRC32_POLY_REVERSED;
    let row: u32 = 1;
    let n: usize = 1;
    while (n < 32) {
        odd[n] = row;
        row = row << 1;
        n = n + 1;
    }
    let even = crcGf2MatrixSquare(odd);
    odd = crcGf2MatrixSquare(even);

    let c1 = crc1;
    let rem = len2;
    while (rem != 0) {
        even = crcGf2MatrixSquare(odd);
        if ((rem & 1) != 0) {
            c1 = crcGf2MatrixTimes(even, c1);
        }
        rem = rem >> 1;
        if (rem == 0) { break; }
        odd = crcGf2MatrixSquare(even);
        if ((rem & 1) != 0) {
            c1 = crcGf2MatrixTimes(odd, c1);
        }
        rem = rem >> 1;
    }
    return c1 ^ crc2;
}

pub struct Crc32Hasher {
    pub state: u32,
    pub amount: u64,

    pub static fn new(): Crc32Hasher {
        return Crc32Hasher { state: 0, amount: 0 };
    }

    pub static fn newWithInitial(init_crc: u32): Crc32Hasher {
        return Crc32Hasher { state: init_crc, amount: 0 };
    }

    pub fn update(*_, buf: []u8) {
        let c: u32 = _.state ^ 4294967295; // 0xFFFFFFFF
        let i: usize = 0;
        while (i < buf.len) {
            c = crcByteStep(c, buf[i]);
            i = i + 1;
        }
        _.state = c ^ 4294967295;
        _.amount = _.amount + (buf.len as u64);
    }

    pub fn updateStr(*_, s: string) {
        let c: u32 = _.state ^ 4294967295;
        let i: usize = 0;
        while (i < s.len) {
            c = crcByteStep(c, s[i]);
            i = i + 1;
        }
        _.state = c ^ 4294967295;
        _.amount = _.amount + (s.len as u64);
    }

    pub fn finalize(): u32 {
        return _.state;
    }

    pub fn reset(*_) {
        _.state = 0;
        _.amount = 0;
    }

    pub fn combine(*_, other: Crc32Hasher) {
        _.state = combineCrc32(_.state, other.state, other.amount);
        _.amount = _.amount + other.amount;
    }
}

pub fn hashBytes(buf: []u8): u32 {
    let h = Crc32Hasher.new();
    h.update(buf);
    return h.finalize();
}

pub fn hashStr(s: string): u32 {
    let h = Crc32Hasher.new();
    h.updateStr(s);
    return h.finalize();
}
