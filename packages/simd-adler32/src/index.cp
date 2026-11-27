// @cap/crc32 — Zero-allocation IEEE CRC-32, Castagnoli CRC-32C, Adler-32 & CRC-32 combine engine for CP
// Clean-room MIT implementation inspired by Rust `crc32fast` (918M downloads) and zlib `crc32_combine`.

const IEEE_POLY: u32 = 0xEDB88320;
const CASTAGNOLI_POLY: u32 = 0x82F63B78;
const ADLER_MOD: u32 = 65521;
const ADLER_NMAX: usize = 5552;
type SimdAdlVec16U8 = @Vector(16, u8);
type SimdAdlVec16U32 = @Vector(16, u32);

fn buildCrcTable(poly: u32) -> [256]u32 {
    let table: [256]u32 = undefined;
    let i: u32 = 0;
    while (i < 256) {
        let crc = i;
        let bit: u8 = 0;
        while (bit < 8) {
            if ((crc & (1 as u32)) != (0 as u32)) {
                crc = (crc >> 1) ^ poly;
            } else {
                crc = crc >> 1;
            }
            bit += 1;
        }
        table[i as usize] = crc;
        i += 1;
    }
    return table;
}

pub struct Crc32Hasher {
    pub state: u32,
    pub table: [256]u32,

    pub static fn init() -> Crc32Hasher {
        return Crc32Hasher{
            state: 0xFFFFFFFF as u32,
            table: buildCrcTable(IEEE_POLY),
        };
    }

    pub fn update(*_, data: string) {
        let crc = _.state;
        let i: usize = 0;
        while (i < data.len) {
            const idx = ((crc ^ (data[i] as u32)) & (0xFF as u32)) as usize;
            crc = (crc >> 8) ^ _.table[idx];
            i += 1;
        }
        _.state = crc;
    }

    pub fn finalize() -> u32 {
        return _.state ^ (0xFFFFFFFF as u32);
    }

    pub fn reset(*_) {
        _.state = 0xFFFFFFFF as u32;
    }
}

pub struct Crc32cHasher {
    pub state: u32,
    pub table: [256]u32,

    pub static fn init() -> Crc32cHasher {
        return Crc32cHasher{
            state: 0xFFFFFFFF as u32,
            table: buildCrcTable(CASTAGNOLI_POLY),
        };
    }

    pub fn update(*_, data: string) {
        let crc = _.state;
        let i: usize = 0;
        while (i < data.len) {
            const idx = ((crc ^ (data[i] as u32)) & (0xFF as u32)) as usize;
            crc = (crc >> 8) ^ _.table[idx];
            i += 1;
        }
        _.state = crc;
    }

    pub fn finalize() -> u32 {
        return _.state ^ (0xFFFFFFFF as u32);
    }

    pub fn reset(*_) {
        _.state = 0xFFFFFFFF as u32;
    }
}

pub fn crc32Ieee(data: string) -> u32 {
    let h = Crc32Hasher.init();
    h.update(data);
    return h.finalize();
}

pub fn crc32c(data: string) -> u32 {
    let h = Crc32cHasher.init();
    h.update(data);
    return h.finalize();
}

pub fn adler32(data: string) -> u32 {
    const weights: SimdAdlVec16U32 = [16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1];
    let a: u32 = 1;
    let b: u32 = 0;
    let pos: usize = 0;
    const n: usize = data.len;
    while (pos < n) {
        let chunk: usize = n - pos;
        if (chunk > ADLER_NMAX) {
            chunk = ADLER_NMAX;
        }
        const end: usize = pos + chunk;
        while (pos + 16 <= end) {
            const v8: SimdAdlVec16U8 = data[pos .. pos + 16][0..16].*;
            const v32: SimdAdlVec16U32 = v8;
            b += a * 16 + @reduce(.Add, v32 * weights);
            a += @reduce(.Add, v32);
            pos += 16;
        }
        while (pos < end) : (pos += 1) {
            a += data[pos] as u32;
            b += a;
        }
        a = a % ADLER_MOD;
        b = b % ADLER_MOD;
    }
    return (b << 16) | a;
}

fn gf2MatrixTimes(mat: *[32]u32, vec_in: u32) -> u32 {
    let sum: u32 = 0;
    let vec = vec_in;
    let idx: usize = 0;
    while (vec != (0 as u32) && idx < 32) {
        if ((vec & (1 as u32)) != (0 as u32)) {
            sum = sum ^ mat[idx];
        }
        vec = vec / (2 as u32);
        idx += 1;
    }
    return sum;
}

fn gf2MatrixSquare(square: *[32]u32, mat: *[32]u32) {
    let n: usize = 0;
    while (n < 32) {
        square[n] = gf2MatrixTimes(mat, mat[n]);
        n += 1;
    }
}

pub fn combineCrc32Ieee(crc1: u32, crc2: u32, len2: usize) -> u32 {
    if (len2 == 0) {
        return crc1;
    }

    let odd: [32]u32 = undefined;
    let even: [32]u32 = undefined;

    odd[0] = IEEE_POLY;
    let row: u32 = 1;
    let n: usize = 1;
    while (n < 32) {
        odd[n] = row;
        row = row * (2 as u32);
        n += 1;
    }

    gf2MatrixSquare(&even, &odd);
    gf2MatrixSquare(&odd, &even);

    let c1 = crc1;
    let rem = len2;
    while (rem > 0) {
        gf2MatrixSquare(&even, &odd);
        if ((rem & (1 as usize)) != (0 as usize)) {
            c1 = gf2MatrixTimes(&even, c1);
        }
        rem = rem / (2 as usize);
        if (rem == 0) {
            break;
        }
        gf2MatrixSquare(&odd, &even);
        if ((rem & (1 as usize)) != (0 as usize)) {
            c1 = gf2MatrixTimes(&odd, c1);
        }
        rem = rem / (2 as usize);
    }

    return c1 ^ crc2;
}

export {
    Crc32Hasher,
    Crc32cHasher,
    crc32Ieee,
    crc32c,
    adler32,
    combineCrc32Ieee,
};
