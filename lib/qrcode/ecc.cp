// Source: cp/lib/qrcode/ecc.cp (GF(256) Reed-Solomon Error Correction for QR Code)
// Pure CP Galois Field GF(2^8) arithmetic (primitive polynomial 0x11D) and
// Reed-Solomon generator polynomial & error-correction codeword computation.

pub const QR_GF_PRIMITIVE: u16 = 0x11D;

struct GfTables {
    exp: [512]u8,
    log: [256]u8,
}

fn buildGfTables(): GfTables {
    @setEvalBranchQuota(10000);
    let exp_tbl: [512]u8 = [0; 512];
    let log_tbl: [256]u8 = [0; 256];
    let x: u16 = 1;
    let i: usize = 0;
    while (i < 255) : (i += 1) {
        exp_tbl[i] = x as u8;
        log_tbl[usize(x)] = i as u8;
        x = x << 1;
        if (x >= 256) {
            x = x ^ QR_GF_PRIMITIVE;
        }
    }
    let j: usize = 255;
    while (j < 512) : (j += 1) {
        exp_tbl[j] = exp_tbl[j - 255];
    }
    return GfTables{ exp: exp_tbl, log: log_tbl };
}

const GF_TABLES: GfTables = buildGfTables();

/// Pure CP GF(256) and Reed-Solomon error-correction encoder for ISO/IEC 18004 QR Code.
pub static struct ReedSolomon {
    /// Returns `alpha^i` in GF(256) with primitive polynomial `0x11D`.
    static fn gfExp(i: usize): u8 {
        return GF_TABLES.exp[i % 255];
    }

    /// Returns discrete log base `alpha` of non-zero `a` in GF(256).
    static fn gfLog(a: u8): u8 {
        return GF_TABLES.log[usize(a)];
    }

    /// Multiplies two elements in GF(256) modulo `0x11D`.
    static fn gfMul(a: u8, b: u8): u8 {
        if (a == 0 || b == 0) return 0;
        const sum = usize(GF_TABLES.log[usize(a)]) + usize(GF_TABLES.log[usize(b)]);
        return GF_TABLES.exp[sum];
    }

    /// Raises `base` to `power` in GF(256).
    static fn gfPow(base: u8, power: usize): u8 {
        if (power == 0) return 1;
        if (base == 0) return 0;
        const prod = (usize(GF_TABLES.log[usize(base)]) * (power % 255)) % 255;
        return GF_TABLES.exp[prod];
    }

    /// Computes the Reed-Solomon generator polynomial of degree `ecc_len` into `out_gen[0..ecc_len+1]`.
    /// Leading coefficient is `out_gen[0] == 1`. Requires `1 <= ecc_len <= 255` and `out_gen.len > ecc_len`.
    static fn buildGenerator(ecc_len: usize, out_gen: []u8) {
        if (ecc_len == 0 || ecc_len > 255 || out_gen.len <= ecc_len) return;
        let k: usize = 0;
        while (k <= ecc_len) : (k += 1) {
            out_gen[k] = 0;
        }
        out_gen[0] = 1;

        let deg: usize = 0;
        while (deg < ecc_len) : (deg += 1) {
            const root = GF_TABLES.exp[deg];
            let j: usize = deg + 1;
            while (j > 0) : (j -= 1) {
                out_gen[j] = out_gen[j] ^ ReedSolomon.gfMul(out_gen[j - 1], root);
            }
        }
    }

    /// Computes `ecc_len` Reed-Solomon error-correction codewords for `data` and writes them to `out_ecc[0..ecc_len]`.
    static fn computeEcc(data: []const u8, ecc_len: usize, out_ecc: []u8) {
        if (ecc_len == 0 || ecc_len > 255 || out_ecc.len < ecc_len) return;

        let gen: [256]u8 = [0; 256];
        ReedSolomon.buildGenerator(ecc_len, gen[0 .. ecc_len + 1]);

        let i: usize = 0;
        while (i < ecc_len) : (i += 1) {
            out_ecc[i] = 0;
        }

        for (b of data) {
            const factor = b ^ out_ecc[0];
            let j: usize = 0;
            while (j + 1 < ecc_len) : (j += 1) {
                out_ecc[j] = out_ecc[j + 1];
            }
            out_ecc[ecc_len - 1] = 0;

            if (factor != 0) {
                let k: usize = 0;
                while (k < ecc_len) : (k += 1) {
                    out_ecc[k] = out_ecc[k] ^ ReedSolomon.gfMul(gen[k + 1], factor);
                }
            }
        }
    }
}
