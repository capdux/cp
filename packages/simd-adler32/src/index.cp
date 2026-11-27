// @cap/crc32 — Zero-allocation IEEE CRC-32, Castagnoli CRC-32C, Adler-32 & CRC-32 combine engine for CP
// Clean-room MIT implementation inspired by Rust `crc32fast` (918M downloads) and zlib `crc32_combine`.

const IEEE_POLY: u32 = 0xEDB88320;
const CASTAGNOLI_POLY: u32 = 0x82F63B78;
const ADLER_MOD: u32 = 65521;

fn buildCrcTable(poly: u32) -> [256]u32 {
    let table: [256]u32 = undefined;
    let i: u32 = 0;
    while (i < 256) {
        let crc = i;
        let bit: u8 = 0;
        while (bit < 8) {
            if ((crc & (1 as u32)) != (0 as u32)) {
                crc = (crc / (2 as u32)) ^ poly;
            } else {
                crc = crc / (2 as u32);
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
            crc = (crc / (256 as u32)) ^ _.table[idx];
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
            crc = (crc / (256 as u32)) ^ _.table[idx];
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
    let a: u32 = 1;
    let b: u32 = 0;
    let i: usize = 0;
    while (i < data.len) {
        a = (a + (data[i] as u32)) % ADLER_MOD;
        b = (b + a) % ADLER_MOD;
        i += 1;
    }
    return (b * (65536 as u32)) | a;
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
