// @cap/curve25519-dalek — Zero-allocation constant-time GF(2^255-19) & RFC 7748 X25519
// Parity with dalek-cryptography/curve25519-dalek (crates.io:curve25519-dalek, BSD-3-Clause)

fn c25Carry(o: []i64) {
    let i: usize = 0;
    while (i < 16) {
        o[i] += 65536 as i64;
        const c: i64 = o[i] >> 16;
        if (i < 15) {
            o[i + 1] += c - (1 as i64);
        } else {
            o[0] += (c - (1 as i64)) * (38 as i64);
        }
        o[i] -= c << 16;
        i += 1;
    }
}

fn c25Sel(p: []i64, q: []i64, b: i64) {
    let mask: i64 = 0;
    if (b != (0 as i64)) {
        mask = -1 as i64;
    }
    let i: usize = 0;
    while (i < 16) {
        const t: i64 = mask & (p[i] ^ q[i]);
        p[i] = p[i] ^ t;
        q[i] = q[i] ^ t;
        i += 1;
    }
}

/// Element of the prime field GF(2^255 - 19) stored as 16 signed 16-bit-radix limbs.
pub struct FieldElement {
    limbs: [16]i64;

    static const ZERO: FieldElement = FieldElement{
        limbs: @as([16]i64, [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]),
    };

    static const ONE: FieldElement = FieldElement{
        limbs: @as([16]i64, [1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]),
    };

    static fn fromBytes(b: [32]u8): FieldElement {
        let out: [16]i64 = @as([16]i64, [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]);
        let i: usize = 0;
        while (i < 16) {
            out[i] = (b[2 * i] as i64) | ((b[2 * i + 1] as i64) << 8);
            i += 1;
        }
        out[15] = out[15] & (0x7fff as i64);
        return FieldElement{ limbs: out };
    }

    fn toBytes(): [32]u8 {
        let t: [16]i64 = @as([16]i64, [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]);
        let ci: usize = 0;
        while (ci < 16) {
            t[ci] = _.limbs[ci];
            ci += 1;
        }
        let m: [16]i64 = @as([16]i64, [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]);
        c25Carry(t[0..16]);
        c25Carry(t[0..16]);
        c25Carry(t[0..16]);

        let j: usize = 0;
        while (j < 2) {
            m[0] = t[0] - (0xffed as i64);
            let i: usize = 1;
            while (i < 15) {
                m[i] = t[i] - (0xffff as i64) - ((m[i - 1] >> 16) & (1 as i64));
                m[i - 1] = m[i - 1] & (0xffff as i64);
                i += 1;
            }
            m[15] = t[15] - (0x7fff as i64) - ((m[14] >> 16) & (1 as i64));
            const b: i64 = (m[15] >> 16) & (1 as i64);
            m[14] = m[14] & (0xffff as i64);
            c25Sel(t[0..16], m[0..16], (1 as i64) - b);
            j += 1;
        }

        let out: [32]u8 = undefined;
        let k: usize = 0;
        while (k < 16) {
            out[2 * k] = u8(t[k] & (0xff as i64));
            out[2 * k + 1] = u8((t[k] >> 8) & (0xff as i64));
            k += 1;
        }
        return out;
    }

    fn add(other: FieldElement): FieldElement {
        let out: [16]i64 = @as([16]i64, [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]);
        let i: usize = 0;
        while (i < 16) {
            out[i] = _.limbs[i] + other.limbs[i];
            i += 1;
        }
        return FieldElement{ limbs: out };
    }

    fn sub(other: FieldElement): FieldElement {
        let out: [16]i64 = @as([16]i64, [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]);
        let i: usize = 0;
        while (i < 16) {
            out[i] = _.limbs[i] - other.limbs[i];
            i += 1;
        }
        return FieldElement{ limbs: out };
    }

    fn mul(other: FieldElement): FieldElement {
        let t: [31]i64 = @as([31]i64, [
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        ]);
        let i: usize = 0;
        while (i < 16) {
            let j: usize = 0;
            while (j < 16) {
                t[i + j] += _.limbs[i] * other.limbs[j];
                j += 1;
            }
            i += 1;
        }
        let k: usize = 0;
        while (k < 15) {
            t[k] += (38 as i64) * t[k + 16];
            k += 1;
        }
        let out: [16]i64 = @as([16]i64, [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]);
        let m: usize = 0;
        while (m < 16) {
            out[m] = t[m];
            m += 1;
        }
        c25Carry(out[0..16]);
        c25Carry(out[0..16]);
        return FieldElement{ limbs: out };
    }

    fn square(): FieldElement {
        return _.mul(_);
    }

    /// Computes multiplicative inverse `a^(p - 2) mod p` where `p = 2^255 - 19`.
    fn invert(): FieldElement {
        let c: FieldElement = _;
        let a: usize = 253;
        while (true) {
            c = c.square();
            if (a != 2 && a != 4) {
                c = c.mul(_);
            }
            if (a == 0) {
                break;
            }
            a -= 1;
        }
        return c;
    }
}

/// Applies RFC 7748 §5 scalar clamping to a 32-byte Curve25519 private key.
pub fn clampScalar(k: [32]u8): [32]u8 {
    let out: [32]u8 = undefined;
    let i: usize = 0;
    while (i < 32) {
        out[i] = k[i];
        i += 1;
    }
    out[0] = out[0] & (0xf8 as u8);
    out[31] = (out[31] & (0x7f as u8)) | (0x40 as u8);
    return out;
}

/// Computes RFC 7748 X25519 scalar multiplication `scalar * u_coord` on Curve25519.
pub fn x25519(scalar: [32]u8, u_coord: [32]u8): [32]u8 {
    const e: [32]u8 = clampScalar(scalar);
    const x1: FieldElement = FieldElement.fromBytes(u_coord);
    const a24: FieldElement = FieldElement{
        limbs: @as([16]i64, [0xdb41, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]),
    };

    let x2: FieldElement = FieldElement.ONE;
    let z2: FieldElement = FieldElement.ZERO;
    let x3: FieldElement = x1;
    let z3: FieldElement = FieldElement.ONE;

    let swap: i64 = 0;
    let pos: usize = 254;
    while (true) {
        const byteIdx = pos >> 3;
        const bitIdx = pos & 7;
        const kt: i64 = ((e[byteIdx] >> bitIdx) & (1 as u8)) as i64;
        swap = swap ^ kt;
        c25Sel(x2.limbs[0..16], x3.limbs[0..16], swap);
        c25Sel(z2.limbs[0..16], z3.limbs[0..16], swap);
        swap = kt;

        const a = x2.add(z2);
        const aa = a.square();
        const b = x2.sub(z2);
        const bb = b.square();
        const diff = aa.sub(bb);
        const c = x3.add(z3);
        const d = x3.sub(z3);
        const da = d.mul(a);
        const cb = c.mul(b);

        x3 = da.add(cb).square();
        z3 = x1.mul(da.sub(cb).square());
        x2 = aa.mul(bb);
        z2 = diff.mul(aa.add(a24.mul(diff)));

        if (pos == 0) {
            break;
        }
        pos -= 1;
    }

    c25Sel(x2.limbs[0..16], x3.limbs[0..16], swap);
    c25Sel(z2.limbs[0..16], z3.limbs[0..16], swap);

    const out: [32]u8 = x2.mul(z2.invert()).toBytes();
    return out;
}

/// Computes X25519 public key from a 32-byte private scalar using Curve25519 basepoint u = 9.
pub fn x25519Basepoint(scalar: [32]u8): [32]u8 {
    const base: [32]u8 = @as([32]u8, [
        9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    ]);
    const out: [32]u8 = x25519(scalar, base);
    return out;
}
