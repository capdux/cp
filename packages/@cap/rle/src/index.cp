// @cap/rle — Zero-allocation PackBits RLE & Delta/Delta-of-Delta time-series encoding for CP
// Clean-room MIT implementation of Apple/TIFF PackBits and Prometheus/Gorilla Delta-of-Delta codecs.
//
// Features:
// - PackBits RLE (`encodePackBits`, `decodePackBits`): control byte 0..127 = literal run (n+1 bytes),
//   129..255 = repeat run (257-c copies of next byte), 128 = NOP; strict bounds & truncation guards.
// - Delta & Delta-of-Delta integer time-series codecs (`encodeDeltaI64`, `decodeDeltaI64`,
//   `encodeDeltaOfDeltaI64`, `decodeDeltaOfDeltaI64`).
// - Zero heap allocation.

// ============================================================================
// 1. Apple / TIFF PackBits RLE
// ============================================================================

/// Compresses `input` bytes using Apple/TIFF PackBits RLE into caller-provided `out`.
/// Returns `null` if `out` is too small.
pub fn encodePackBits(input: string, out: []u8) -> string? {
    let i: usize = 0;
    let out_pos: usize = 0;

    while (i < input.len) {
        // Check for a repeat run (>= 2 identical consecutive bytes, max 128)
        if (i + 1 < input.len && input[i] == input[i + 1]) {
            const val = input[i];
            let run_len: usize = 2;
            while (i + run_len < input.len && run_len < 128 && input[i + run_len] == val) {
                run_len += 1;
            }
            if (out_pos + 2 > out.len) {
                return null;
            }
            out[out_pos] = (257 - run_len) as u8;
            out[out_pos + 1] = val;
            out_pos += 2;
            i += run_len;
        } else {
            // Collect literal run until a repeat run starts or length reaches 128
            const lit_start = i;
            let lit_len: usize = 1;
            while (lit_start + lit_len < input.len && lit_len < 128) {
                if (lit_start + lit_len + 1 < input.len &&
                    input[lit_start + lit_len] == input[lit_start + lit_len + 1]) {
                    break;
                }
                lit_len += 1;
            }
            if (out_pos + 1 + lit_len > out.len) {
                return null;
            }
            out[out_pos] = (lit_len - 1) as u8;
            out_pos += 1;
            let k: usize = 0;
            while (k < lit_len) {
                out[out_pos + k] = input[lit_start + k];
                k += 1;
            }
            out_pos += lit_len;
            i += lit_len;
        }
    }

    return out[0..out_pos];
}

/// Decompresses Apple/TIFF PackBits RLE `encoded` bytes into caller-provided `out`.
/// Returns `null` on truncated control/payload sequences or insufficient `out` capacity.
pub fn decodePackBits(encoded: string, out: []u8) -> string? {
    let i: usize = 0;
    let out_pos: usize = 0;

    while (i < encoded.len) {
        const ctrl = encoded[i];
        i += 1;

        if (ctrl == 128) {
            // NOP per PackBits specification
            continue;
        }

        if (ctrl <= 127) {
            const count: usize = (ctrl as usize) + 1;
            if (count > encoded.len - i || count > out.len - out_pos) {
                return null;
            }
            let k: usize = 0;
            while (k < count) {
                out[out_pos + k] = encoded[i + k];
                k += 1;
            }
            i += count;
            out_pos += count;
        } else {
            // ctrl in 129..=255 -> repeat next byte (257 - ctrl) times
            if (i >= encoded.len) {
                return null;
            }
            const count: usize = (257 as usize) - (ctrl as usize);
            if (count > out.len - out_pos) {
                return null;
            }
            const val = encoded[i];
            i += 1;
            let k: usize = 0;
            while (k < count) {
                out[out_pos + k] = val;
                k += 1;
            }
            out_pos += count;
        }
    }

    return out[0..out_pos];
}

// ============================================================================
// 2. Delta & Delta-of-Delta Time-Series Integer Encoding
// ============================================================================

/// First-order delta encoding: `out[0] = values[0]`, `out[i] = values[i] - values[i-1]`.
/// Returns the number of elements written (`min(values.len, out.len)`).
pub fn encodeDeltaI64(values: []const i64, out: []i64) -> usize {
    const n = if (values.len < out.len) { values.len } else { out.len };
    if (n == 0) {
        return 0;
    }
    out[0] = values[0];
    let i: usize = 1;
    while (i < n) {
        out[i] = values[i] - values[i - 1];
        i += 1;
    }
    return n;
}

/// Decodes first-order deltas back to the original integer sequence via prefix sum.
pub fn decodeDeltaI64(deltas: []const i64, out: []i64) -> usize {
    const n = if (deltas.len < out.len) { deltas.len } else { out.len };
    if (n == 0) {
        return 0;
    }
    out[0] = deltas[0];
    let i: usize = 1;
    while (i < n) {
        out[i] = out[i - 1] + deltas[i];
        i += 1;
    }
    return n;
}

/// Second-order Delta-of-Delta encoding (Prometheus/Gorilla timestamp compression):
/// constant-interval sequences produce all zeros from index 2 onward.
pub fn encodeDeltaOfDeltaI64(values: []const i64, out: []i64) -> usize {
    const n = if (values.len < out.len) { values.len } else { out.len };
    if (n == 0) {
        return 0;
    }
    out[0] = values[0];
    if (n == 1) {
        return 1;
    }
    let prev_delta = values[1] - values[0];
    out[1] = prev_delta;
    let i: usize = 2;
    while (i < n) {
        const cur_delta = values[i] - values[i - 1];
        out[i] = cur_delta - prev_delta;
        prev_delta = cur_delta;
        i += 1;
    }
    return n;
}

/// Decodes a Delta-of-Delta encoded sequence back to the original `i64` series losslessly.
pub fn decodeDeltaOfDeltaI64(dod: []const i64, out: []i64) -> usize {
    const n = if (dod.len < out.len) { dod.len } else { out.len };
    if (n == 0) {
        return 0;
    }
    out[0] = dod[0];
    if (n == 1) {
        return 1;
    }
    let prev_delta = dod[1];
    out[1] = out[0] + prev_delta;
    let i: usize = 2;
    while (i < n) {
        const cur_delta = prev_delta + dod[i];
        out[i] = out[i - 1] + cur_delta;
        prev_delta = cur_delta;
        i += 1;
    }
    return n;
}
