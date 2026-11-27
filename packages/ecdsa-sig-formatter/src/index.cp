// @cap/ecdsa-sig-formatter — ASN.1 DER <-> JOSE IEEE P1363 ECDSA signature formatter
// Inspired by npm:ecdsa-sig-formatter (https://github.com/Brightspace/node-ecdsa-sig-formatter)

pub enum EcdsaAlg {
    ES256,
    ES384,
    ES512,
}

/// Returns the coordinate byte length for the given JWS ECDSA algorithm:
/// ES256 -> 32, ES384 -> 48, ES512 -> 66.
pub fn paramBytes(alg: EcdsaAlg): usize {
    if (alg == EcdsaAlg.ES256) {
        return 32;
    }
    if (alg == EcdsaAlg.ES384) {
        return 48;
    }
    return 66;
}

fn esfParseDerInt(
    der: []const u8,
    pos: usize,
    limit: usize,
    param_len: usize,
    dst: []u8
): i64 {
    if (pos + 2 > limit) {
        return -1;
    }
    if (der[pos] != 0x02) {
        return -1;
    }
    const len_byte: u8 = der[pos + 1];
    if ((len_byte & 0x80) != 0) {
        return -1;
    }
    const int_len: usize = usize(len_byte);
    if (int_len == 0 || pos + 2 + int_len > limit) {
        return -1;
    }

    let raw_start: usize = pos + 2;
    let raw_len: usize = int_len;
    // ASN.1 INTEGER is signed; ECDSA r/s must be non-negative
    if ((der[raw_start] & 0x80) != 0) {
        return -1;
    }

    // Strip leading 0x00 padding bytes
    while (raw_len > 0 && der[raw_start] == 0x00) {
        raw_start += 1;
        raw_len -= 1;
    }
    if (raw_len > param_len) {
        return -1;
    }

    const pad: usize = param_len - raw_len;
    let i: usize = 0;
    while (i < pad) : (i += 1) {
        dst[i] = 0;
    }
    let j: usize = 0;
    while (j < raw_len) : (j += 1) {
        dst[pad + j] = der[raw_start + j];
    }

    return i64(pos + 2 + int_len);
}

fn esfTrimLeadingZeros(src: []const u8): usize {
    let off: usize = 0;
    while (off + 1 < src.len && src[off] == 0x00) {
        off += 1;
    }
    return off;
}

/// Converts an ASN.1 DER ECDSA signature into JOSE IEEE P1363 `(r || s)` format.
/// Returns `2 * paramBytes(alg)` on success, or `-1` on malformed DER or insufficient output capacity.
pub fn derToJose(der: []const u8, alg: EcdsaAlg, out: []u8): i64 {
    const n: usize = paramBytes(alg);
    const jose_len: usize = n * 2;
    if (out.len < jose_len || der.len < 8) {
        return -1;
    }
    if (der[0] != 0x30) {
        return -1;
    }

    let seq_len: usize = 0;
    let pos: usize = 0;
    const len_b: u8 = der[1];
    if ((len_b & 0x80) == 0) {
        seq_len = usize(len_b);
        pos = 2;
    } else if (len_b == 0x81) {
        if (der.len < 3) {
            return -1;
        }
        seq_len = usize(der[2]);
        if (seq_len < 0x80) {
            return -1;
        }
        pos = 3;
    } else {
        return -1;
    }

    if (pos + seq_len != der.len) {
        return -1;
    }

    const after_r: i64 = esfParseDerInt(der, pos, der.len, n, out[0..n]);
    if (after_r < 0) {
        return -1;
    }
    const after_s: i64 = esfParseDerInt(der, usize(after_r), der.len, n, out[n..jose_len]);
    if (after_s < 0) {
        return -1;
    }
    if (usize(after_s) != der.len) {
        return -1;
    }

    return i64(jose_len);
}

/// Converts a JOSE IEEE P1363 `(r || s)` ECDSA signature into canonical ASN.1 DER SEQUENCE format.
/// Returns the DER byte length on success, or `-1` on invalid input length or insufficient output capacity.
pub fn joseToDer(jose: []const u8, alg: EcdsaAlg, out: []u8): i64 {
    const n: usize = paramBytes(alg);
    if (jose.len != n * 2) {
        return -1;
    }

    const r_slice: []const u8 = jose[0..n];
    const s_slice: []const u8 = jose[n..n * 2];

    const r_off: usize = esfTrimLeadingZeros(r_slice);
    const s_off: usize = esfTrimLeadingZeros(s_slice);

    const r_raw_len: usize = n - r_off;
    const s_raw_len: usize = n - s_off;

    const r_pad: usize = if ((r_slice[r_off] & 0x80) != 0) 1 else 0;
    const s_pad: usize = if ((s_slice[s_off] & 0x80) != 0) 1 else 0;

    const r_der_len: usize = r_raw_len + r_pad;
    const s_der_len: usize = s_raw_len + s_pad;
    const seq_len: usize = 2 + r_der_len + 2 + s_der_len;
    const hdr_len: usize = if (seq_len >= 0x80) 3 else 2;
    const total_len: usize = hdr_len + seq_len;

    if (out.len < total_len) {
        return -1;
    }

    let p: usize = 0;
    out[p] = 0x30;
    p += 1;
    if (seq_len >= 0x80) {
        out[p] = 0x81;
        out[p + 1] = u8(seq_len);
        p += 2;
    } else {
        out[p] = u8(seq_len);
        p += 1;
    }

    // Encode r
    out[p] = 0x02;
    out[p + 1] = u8(r_der_len);
    p += 2;
    if (r_pad == 1) {
        out[p] = 0x00;
        p += 1;
    }
    let ri: usize = 0;
    while (ri < r_raw_len) : (ri += 1) {
        out[p + ri] = r_slice[r_off + ri];
    }
    p += r_raw_len;

    // Encode s
    out[p] = 0x02;
    out[p + 1] = u8(s_der_len);
    p += 2;
    if (s_pad == 1) {
        out[p] = 0x00;
        p += 1;
    }
    let si: usize = 0;
    while (si < s_raw_len) : (si += 1) {
        out[p + si] = s_slice[s_off + si];
    }
    p += s_raw_len;

    return i64(p);
}
