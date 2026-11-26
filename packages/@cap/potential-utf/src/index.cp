// @cap/potential-utf — Zero-allocation potentially-ill-formed UTF-8/UTF-16/CodePoint wrappers
// Parity with ICU4X crates.io:potential_utf (Unicode-3.0)

fn putfIsCont(b: u8): bool {
    return (b & (0xC0 as u8)) == (0x80 as u8);
}

// Validate a single UTF-8 sequence starting at bytes[pos].
// Returns sequence length (1..=4) if valid, or 0 if ill-formed.
fn putfValidSeqLen(bytes: []const u8, pos: usize): usize {
    if (pos >= bytes.len) {
        return 0;
    }
    const b0 = bytes[pos];
    if (b0 <= (0x7F as u8)) {
        return 1;
    }
    if (b0 >= (0xC2 as u8) && b0 <= (0xDF as u8)) {
        if (pos + 1 < bytes.len && putfIsCont(bytes[pos + 1])) {
            return 2;
        }
        return 0;
    }
    if (b0 == (0xE0 as u8)) {
        if (pos + 2 < bytes.len) {
            const b1 = bytes[pos + 1];
            const b2 = bytes[pos + 2];
            if (b1 >= (0xA0 as u8) && b1 <= (0xBF as u8) && putfIsCont(b2)) {
                return 3;
            }
        }
        return 0;
    }
    if ((b0 >= (0xE1 as u8) && b0 <= (0xEC as u8)) || (b0 >= (0xEE as u8) && b0 <= (0xEF as u8))) {
        if (pos + 2 < bytes.len && putfIsCont(bytes[pos + 1]) && putfIsCont(bytes[pos + 2])) {
            return 3;
        }
        return 0;
    }
    if (b0 == (0xED as u8)) {
        if (pos + 2 < bytes.len) {
            const b1 = bytes[pos + 1];
            const b2 = bytes[pos + 2];
            if (b1 >= (0x80 as u8) && b1 <= (0x9F as u8) && putfIsCont(b2)) {
                return 3;
            }
        }
        return 0;
    }
    if (b0 == (0xF0 as u8)) {
        if (pos + 3 < bytes.len) {
            const b1 = bytes[pos + 1];
            const b2 = bytes[pos + 2];
            const b3 = bytes[pos + 3];
            if (b1 >= (0x90 as u8) && b1 <= (0xBF as u8) && putfIsCont(b2) && putfIsCont(b3)) {
                return 4;
            }
        }
        return 0;
    }
    if (b0 >= (0xF1 as u8) && b0 <= (0xF3 as u8)) {
        if (pos + 3 < bytes.len && putfIsCont(bytes[pos + 1]) && putfIsCont(bytes[pos + 2]) && putfIsCont(bytes[pos + 3])) {
            return 4;
        }
        return 0;
    }
    if (b0 == (0xF4 as u8)) {
        if (pos + 3 < bytes.len) {
            const b1 = bytes[pos + 1];
            const b2 = bytes[pos + 2];
            const b3 = bytes[pos + 3];
            if (b1 >= (0x80 as u8) && b1 <= (0x8F as u8) && putfIsCont(b2) && putfIsCont(b3)) {
                return 4;
            }
        }
        return 0;
    }
    return 0;
}

// Compute maximal subpart length (1..=3) of an ill-formed UTF-8 subsequence per Unicode Standard.
fn putfMaximalSubpart(bytes: []const u8, pos: usize): usize {
    const b0 = bytes[pos];
    if (b0 >= (0xC2 as u8) && b0 <= (0xDF as u8)) {
        return 1;
    }
    if (b0 == (0xE0 as u8)) {
        if (pos + 1 < bytes.len) {
            const b1 = bytes[pos + 1];
            if (b1 >= (0xA0 as u8) && b1 <= (0xBF as u8)) {
                return 2;
            }
        }
        return 1;
    }
    if ((b0 >= (0xE1 as u8) && b0 <= (0xEC as u8)) || (b0 >= (0xEE as u8) && b0 <= (0xEF as u8))) {
        if (pos + 1 < bytes.len && putfIsCont(bytes[pos + 1])) {
            return 2;
        }
        return 1;
    }
    if (b0 == (0xED as u8)) {
        if (pos + 1 < bytes.len) {
            const b1 = bytes[pos + 1];
            if (b1 >= (0x80 as u8) && b1 <= (0x9F as u8)) {
                return 2;
            }
        }
        return 1;
    }
    if (b0 == (0xF0 as u8)) {
        if (pos + 1 < bytes.len) {
            const b1 = bytes[pos + 1];
            if (b1 >= (0x90 as u8) && b1 <= (0xBF as u8)) {
                if (pos + 2 < bytes.len && putfIsCont(bytes[pos + 2])) {
                    return 3;
                }
                return 2;
            }
        }
        return 1;
    }
    if (b0 >= (0xF1 as u8) && b0 <= (0xF3 as u8)) {
        if (pos + 1 < bytes.len && putfIsCont(bytes[pos + 1])) {
            if (pos + 2 < bytes.len && putfIsCont(bytes[pos + 2])) {
                return 3;
            }
            return 2;
        }
        return 1;
    }
    if (b0 == (0xF4 as u8)) {
        if (pos + 1 < bytes.len) {
            const b1 = bytes[pos + 1];
            if (b1 >= (0x80 as u8) && b1 <= (0x8F as u8)) {
                if (pos + 2 < bytes.len && putfIsCont(bytes[pos + 2])) {
                    return 3;
                }
                return 2;
            }
        }
        return 1;
    }
    return 1;
}

// Encode a valid Unicode scalar value into out[w..]. Returns bytes written (0 if insufficient space).
fn putfEncodeScalar(scalar: u32, out: []u8, w: usize): usize {
    if (scalar <= (0x7F as u32)) {
        if (w + 1 > out.len) {
            return 0;
        }
        out[w] = u8(scalar);
        return 1;
    }
    if (scalar <= (0x7FF as u32)) {
        if (w + 2 > out.len) {
            return 0;
        }
        out[w] = u8((0xC0 as u32) | (scalar >> 6));
        out[w + 1] = u8((0x80 as u32) | (scalar & (0x3F as u32)));
        return 2;
    }
    if (scalar <= (0xFFFF as u32)) {
        if (w + 3 > out.len) {
            return 0;
        }
        out[w] = u8((0xE0 as u32) | (scalar >> 12));
        out[w + 1] = u8((0x80 as u32) | ((scalar >> 6) & (0x3F as u32)));
        out[w + 2] = u8((0x80 as u32) | (scalar & (0x3F as u32)));
        return 3;
    }
    if (w + 4 > out.len) {
        return 0;
    }
    out[w] = u8((0xF0 as u32) | (scalar >> 18));
    out[w + 1] = u8((0x80 as u32) | ((scalar >> 12) & (0x3F as u32)));
    out[w + 2] = u8((0x80 as u32) | ((scalar >> 6) & (0x3F as u32)));
    out[w + 3] = u8((0x80 as u32) | (scalar & (0x3F as u32)));
    return 4;
}

fn putfWriteReplacement(out: []u8, w: usize): usize {
    if (w + 3 > out.len) {
        return 0;
    }
    out[w] = 0xEF as u8;
    out[w + 1] = 0xBF as u8;
    out[w + 2] = 0xBD as u8;
    return 3;
}

/// Wrapper around a byte slice that is expected to be UTF-8, but may contain ill-formed sequences.
pub struct PotentialUtf8 {
    bytes: []const u8;

    static fn fromBytes(b: []const u8): PotentialUtf8 {
        return PotentialUtf8{ bytes: b };
    }

    static fn fromStr(s: string): PotentialUtf8 {
        return PotentialUtf8{ bytes: s };
    }

    fn asBytes(): []const u8 {
        return _.bytes;
    }

    fn len(): usize {
        return _.bytes.len;
    }

    fn isEmpty(): bool {
        return _.bytes.len == 0;
    }

    fn isValidUtf8(): bool {
        let pos: usize = 0;
        while (pos < _.bytes.len) {
            const seqLen = putfValidSeqLen(_.bytes, pos);
            if (seqLen == 0) {
                return false;
            }
            pos += seqLen;
        }
        return true;
    }

    /// Returns the longest valid UTF-8 prefix slice starting at index 0.
    fn toValidPrefix(): []const u8 {
        let pos: usize = 0;
        while (pos < _.bytes.len) {
            const seqLen = putfValidSeqLen(_.bytes, pos);
            if (seqLen == 0) {
                break;
            }
            pos += seqLen;
        }
        return _.bytes[0..pos];
    }

    /// Decodes potentially-ill-formed UTF-8 into `out`, replacing each maximal ill-formed
    /// subsequence with U+FFFD (EF BF BD). Returns the number of bytes written to `out`.
    fn decodeLossy(out: []u8): usize {
        let pos: usize = 0;
        let w: usize = 0;
        while (pos < _.bytes.len) {
            const seqLen = putfValidSeqLen(_.bytes, pos);
            if (seqLen > 0) {
                if (w + seqLen > out.len) {
                    break;
                }
                let k: usize = 0;
                while (k < seqLen) {
                    out[w + k] = _.bytes[pos + k];
                    k += 1;
                }
                w += seqLen;
                pos += seqLen;
            } else {
                const wrote = putfWriteReplacement(out, w);
                if (wrote == 0) {
                    break;
                }
                w += wrote;
                pos += putfMaximalSubpart(_.bytes, pos);
            }
        }
        return w;
    }
}

/// Wrapper around a UTF-16 code unit slice that may contain unpaired surrogates.
pub struct PotentialUtf16 {
    units: []const u16;

    static fn fromSlice(u: []const u16): PotentialUtf16 {
        return PotentialUtf16{ units: u };
    }

    fn asSlice(): []const u16 {
        return _.units;
    }

    fn len(): usize {
        return _.units.len;
    }

    fn isEmpty(): bool {
        return _.units.len == 0;
    }

    fn isValidUtf16(): bool {
        let i: usize = 0;
        while (i < _.units.len) {
            const u = _.units[i];
            if (u >= (0xD800 as u16) && u <= (0xDBFF as u16)) {
                if (i + 1 >= _.units.len) {
                    return false;
                }
                const lo = _.units[i + 1];
                if (lo < (0xDC00 as u16) || lo > (0xDFFF as u16)) {
                    return false;
                }
                i += 2;
            } else if (u >= (0xDC00 as u16) && u <= (0xDFFF as u16)) {
                return false;
            } else {
                i += 1;
            }
        }
        return true;
    }

    /// Converts potentially-ill-formed UTF-16 into UTF-8 in `out`, replacing unpaired
    /// surrogates with U+FFFD (EF BF BD). Returns the number of UTF-8 bytes written.
    fn toUtf8Lossy(out: []u8): usize {
        let i: usize = 0;
        let w: usize = 0;
        while (i < _.units.len) {
            const u = _.units[i];
            if (u >= (0xD800 as u16) && u <= (0xDBFF as u16)) {
                if (i + 1 < _.units.len) {
                    const lo = _.units[i + 1];
                    if (lo >= (0xDC00 as u16) && lo <= (0xDFFF as u16)) {
                        const hiPart: u32 = ((u as u32) - (0xD800 as u32)) << 10;
                        const loPart: u32 = (lo as u32) - (0xDC00 as u32);
                        const scalar: u32 = (0x10000 as u32) + hiPart + loPart;
                        const wrote = putfEncodeScalar(scalar, out, w);
                        if (wrote == 0) {
                            break;
                        }
                        w += wrote;
                        i += 2;
                        continue;
                    }
                }
                const wrote = putfWriteReplacement(out, w);
                if (wrote == 0) {
                    break;
                }
                w += wrote;
                i += 1;
            } else if (u >= (0xDC00 as u16) && u <= (0xDFFF as u16)) {
                const wrote = putfWriteReplacement(out, w);
                if (wrote == 0) {
                    break;
                }
                w += wrote;
                i += 1;
            } else {
                const wrote = putfEncodeScalar(u as u32, out, w);
                if (wrote == 0) {
                    break;
                }
                w += wrote;
                i += 1;
            }
        }
        return w;
    }
}

/// Wrapper around a 32-bit integer that may or may not be a valid Unicode scalar value.
pub struct PotentialCodePoint {
    raw: u32;

    static fn fromU32(v: u32): PotentialCodePoint {
        return PotentialCodePoint{ raw: v };
    }

    fn toU32(): u32 {
        return _.raw;
    }

    /// Returns true iff `raw <= 0x10FFFF` and not in surrogate range `0xD800..=0xDFFF`.
    fn isValidScalar(): bool {
        if (_.raw > (0x10FFFF as u32)) {
            return false;
        }
        if (_.raw >= (0xD800 as u32) && _.raw <= (0xDFFF as u32)) {
            return false;
        }
        return true;
    }

    /// Returns the scalar value if valid, otherwise U+FFFD (0xFFFD).
    fn toCharOrReplacement(): u32 {
        if (_.isValidScalar()) {
            return _.raw;
        }
        return 0xFFFD as u32;
    }

    /// Encodes `toCharOrReplacement()` as UTF-8 into `out`. Returns bytes written.
    fn encodeUtf8Lossy(out: []u8): usize {
        return putfEncodeScalar(_.toCharOrReplacement(), out, 0);
    }
}
