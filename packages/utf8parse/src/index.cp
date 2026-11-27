// @cap/utf8-valid — Zero-allocation RFC 3629 strict UTF-8 validator & lossy repair for CP
// Clean-room MIT implementation inspired by Rust `utf-8`, `simdutf8`, and WHATWG Encoding Standard.

pub struct Utf8Error {
    pub valid_up_to: usize,
    pub error_len: u8,
}

pub struct Utf8CheckResult {
    pub valid: bool,
    pub valid_up_to: usize,
    pub error_len: u8,
    pub codepoint_count: usize,
}

struct StepResult {
    valid: bool,
    step_len: usize,
    error_len: u8, // 0 indicates unexpected EOF (incomplete sequence at end of input)
}

fn isCont(b: u8) -> bool {
    return (b & 0xC0) == 0x80;
}

/// Validates one RFC 3629 UTF-8 sequence starting at `data[pos]` using WHATWG maximal subpart rules.
fn checkSeqAt(data: string, pos: usize) -> StepResult {
    const b0 = data[pos];

    // 1-byte ASCII: 0x00..=0x7F
    if (b0 < 0x80) {
        return StepResult{ valid: true, step_len: 1, error_len: 0 };
    }

    // 2-byte: 0xC2..=0xDF, 0x80..=0xBF (rejects overlong 0xC0, 0xC1)
    if (b0 >= 0xC2 && b0 <= 0xDF) {
        if (pos + 1 >= data.len) {
            return StepResult{ valid: false, step_len: data.len - pos, error_len: 0 };
        }
        if (!isCont(data[pos + 1])) {
            return StepResult{ valid: false, step_len: 1, error_len: 1 };
        }
        return StepResult{ valid: true, step_len: 2, error_len: 0 };
    }

    // 3-byte: 0xE0..=0xEF
    if (b0 >= 0xE0 && b0 <= 0xEF) {
        if (pos + 1 >= data.len) {
            return StepResult{ valid: false, step_len: data.len - pos, error_len: 0 };
        }
        const b1 = data[pos + 1];
        let lo1: u8 = 0x80;
        let hi1: u8 = 0xBF;
        if (b0 == 0xE0) {
            lo1 = 0xA0; // Reject overlong 3-byte (0xE0 0x80..0x9F)
        } else if (b0 == 0xED) {
            hi1 = 0x9F; // Reject UTF-16 surrogates U+D800..U+DFFF (0xED 0xA0..0xBF)
        }
        if (b1 < lo1 || b1 > hi1) {
            return StepResult{ valid: false, step_len: 1, error_len: 1 };
        }
        if (pos + 2 >= data.len) {
            return StepResult{ valid: false, step_len: data.len - pos, error_len: 0 };
        }
        if (!isCont(data[pos + 2])) {
            return StepResult{ valid: false, step_len: 2, error_len: 2 };
        }
        return StepResult{ valid: true, step_len: 3, error_len: 0 };
    }

    // 4-byte: 0xF0..=0xF4
    if (b0 >= 0xF0 && b0 <= 0xF4) {
        if (pos + 1 >= data.len) {
            return StepResult{ valid: false, step_len: data.len - pos, error_len: 0 };
        }
        const b1 = data[pos + 1];
        let lo1: u8 = 0x80;
        let hi1: u8 = 0xBF;
        if (b0 == 0xF0) {
            lo1 = 0x90; // Reject overlong 4-byte (0xF0 0x80..0x8F)
        } else if (b0 == 0xF4) {
            hi1 = 0x8F; // Reject > U+10FFFF (0xF4 0x90..0xBF)
        }
        if (b1 < lo1 || b1 > hi1) {
            return StepResult{ valid: false, step_len: 1, error_len: 1 };
        }
        if (pos + 2 >= data.len) {
            return StepResult{ valid: false, step_len: data.len - pos, error_len: 0 };
        }
        if (!isCont(data[pos + 2])) {
            return StepResult{ valid: false, step_len: 2, error_len: 2 };
        }
        if (pos + 3 >= data.len) {
            return StepResult{ valid: false, step_len: data.len - pos, error_len: 0 };
        }
        if (!isCont(data[pos + 3])) {
            return StepResult{ valid: false, step_len: 3, error_len: 3 };
        }
        return StepResult{ valid: true, step_len: 4, error_len: 0 };
    }

    // Invalid lead byte (0x80..0xC1, 0xF5..0xFF)
    return StepResult{ valid: false, step_len: 1, error_len: 1 };
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

type Utf8Vec16 = @Vector(16, u8);

/// Fast-path check returning `true` if every byte in `data` is 7-bit ASCII (`< 0x80`).
pub fn isAscii(data: string) -> bool {
    const hi_mask: Utf8Vec16 = @splat(0x80 as u8);
    let i: usize = 0;
    while (i + 16 <= data.len) {
        const chunk: Utf8Vec16 = data[i .. i + 16][0..16].*;
        if (@reduce(.Or, chunk & hi_mask) != 0) {
            return false;
        }
        i += 16;
    }
    while (i < data.len) {
        if (data[i] >= 0x80) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// Strictly validates `data` per RFC 3629, returning exact `valid_up_to`, `error_len`
/// (`0` for truncated tail), and `codepoint_count` of the valid prefix.
pub fn validateUtf8(data: string) -> Utf8CheckResult {
    const hi_mask: Utf8Vec16 = @splat(0x80 as u8);
    let i: usize = 0;
    let cps: usize = 0;
    while (i < data.len) {
        // 16-lane hardware SIMD ASCII fast-path
        while (i + 16 <= data.len) {
            const chunk: Utf8Vec16 = data[i .. i + 16][0..16].*;
            if (@reduce(.Or, chunk & hi_mask) != 0) {
                break;
            }
            i += 16;
            cps += 16;
        }
        if (i >= data.len) {
            break;
        }
        if (data[i] < 0x80) {
            i += 1;
            cps += 1;
            continue;
        }
        const st = checkSeqAt(data, i);
        if (!st.valid) {
            return Utf8CheckResult{
                valid: false,
                valid_up_to: i,
                error_len: st.error_len,
                codepoint_count: cps,
            };
        }
        i += st.step_len;
        cps += 1;
    }
    return Utf8CheckResult{
        valid: true,
        valid_up_to: data.len,
        error_len: 0,
        codepoint_count: cps,
    };
}

/// Returns `true` if `data` is strictly valid RFC 3629 UTF-8.
pub fn isValidUtf8(data: string) -> bool {
    return validateUtf8(data).valid;
}

fn appendByte(out: []u8, pos: usize, c: u8) -> usize {
    if (pos < out.len) {
        out[pos] = c;
        return pos + 1;
    }
    return pos;
}

/// Repairs invalid UTF-8 byte sequences in `data` by replacing each WHATWG maximal invalid
/// subpart with `U+FFFD` (`0xEF 0xBF 0xBD`), writing valid UTF-8 into `out`.
pub fn toLossyUtf8(data: string, out: []u8) -> string {
    const hi_mask: Utf8Vec16 = @splat(0x80 as u8);
    let pos: usize = 0;
    let i: usize = 0;
    while (i < data.len) {
        // 16-lane SIMD ASCII block copy fast-path
        while (i + 16 <= data.len && pos + 16 <= out.len) {
            const chunk: Utf8Vec16 = data[i .. i + 16][0..16].*;
            if (@reduce(.Or, chunk & hi_mask) != 0) {
                break;
            }
            const arr: [16]u8 = chunk;
            let j: usize = 0;
            while (j < 16) {
                out[pos + j] = arr[j];
                j += 1;
            }
            pos += 16;
            i += 16;
        }
        if (i >= data.len) {
            break;
        }
        if (data[i] < 0x80) {
            pos = appendByte(out, pos, data[i]);
            i += 1;
            continue;
        }
        const st = checkSeqAt(data, i);
        if (st.valid) {
            let k: usize = 0;
            while (k < st.step_len) {
                pos = appendByte(out, pos, data[i + k]);
                k += 1;
            }
            i += st.step_len;
        } else {
            // Emit U+FFFD (EF BF BD)
            pos = appendByte(out, pos, 0xEF);
            pos = appendByte(out, pos, 0xBF);
            pos = appendByte(out, pos, 0xBD);
            i += st.step_len;
        }
    }
    return out[0..pos];
}

/// Computes the equivalent UTF-16 code-unit length of `data` (for LSP / JS / Windows interop):
/// BMP codepoints (1..3 UTF-8 bytes) count as 1 unit; supplementary plane (4 UTF-8 bytes) count as 2.
pub fn utf16Len(data: string) -> usize {
    const hi_mask: Utf8Vec16 = @splat(0x80 as u8);
    let units: usize = 0;
    let i: usize = 0;
    while (i < data.len) {
        // 16-lane SIMD ASCII fast-path
        while (i + 16 <= data.len) {
            const chunk: Utf8Vec16 = data[i .. i + 16][0..16].*;
            if (@reduce(.Or, chunk & hi_mask) != 0) {
                break;
            }
            i += 16;
            units += 16;
        }
        if (i >= data.len) {
            break;
        }
        if (data[i] < 0x80) {
            units += 1;
            i += 1;
            continue;
        }
        const st = checkSeqAt(data, i);
        if (st.valid) {
            units += if (st.step_len == 4) 2 as usize else 1 as usize;
            i += st.step_len;
        } else {
            // Lossy U+FFFD is in BMP (1 UTF-16 unit)
            units += 1;
            i += st.step_len;
        }
    }
    return units;
}
