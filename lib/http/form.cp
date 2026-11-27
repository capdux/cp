// Source: Absorbed from /data/dev/bun/src/url.zig | Synced: 2026-11-21T16:45:50Z (WHATWG application/x-www-form-urlencoded Parser & Builder)
// Pure-CP zero-allocation application/x-www-form-urlencoded parser, lookup, percent-codec, and builder.

type FormVec16 = @Vector(16, u8);

fn makeFormHexValLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0xFF; 256];
    let i: usize = 0;
    while (i < 10) : (i += 1) {
        lut['0' + i] = @as(u8, @intCast(i));
    }
    let j: usize = 0;
    while (j < 6) : (j += 1) {
        lut['a' + j] = @as(u8, @intCast(10 + j));
        lut['A' + j] = @as(u8, @intCast(10 + j));
    }
    return lut;
}

fn makeFormSafeLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        const b = @as(u8, @intCast(i));
        const ok = (b >= 'A' && b <= 'Z') || (b >= 'a' && b <= 'z') || (b >= '0' && b <= '9') ||
            b == '*' || b == '-' || b == '.' || b == '_';
        lut[i] = if (ok) 1 else 0;
    }
    return lut;
}

const FORM_HEX_VAL_LUT: [u8; 256] = makeFormHexValLut();
const FORM_SAFE_LUT: [u8; 256] = makeFormSafeLut();
const FORM_HEX_UPPER: []const u8 = "0123456789ABCDEF";

fn formFindByte(s: []const u8, start: usize, ch: u8): usize {
    let i: usize = start;
    const v_ch: FormVec16 = @splat(ch);
    while (i + 16 <= s.len) : (i += 16) {
        const v: FormVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_ch)) break;
    }
    while (i < s.len) : (i += 1) {
        if (s[i] == ch) return i;
    }
    return s.len;
}

fn formFindDecodeSpecial(s: []const u8, start: usize): usize {
    let i: usize = start;
    const v_plus: FormVec16 = @splat('+' as u8);
    const v_pct: FormVec16 = @splat('%' as u8);
    while (i + 16 <= s.len) : (i += 16) {
        const v: FormVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, (v == v_plus) | (v == v_pct))) break;
    }
    while (i < s.len) : (i += 1) {
        if (s[i] == '+' || s[i] == '%') return i;
    }
    return s.len;
}

fn formScanSafeRun(s: []const u8, start: usize): usize {
    let i: usize = start;
    const v_a: FormVec16 = @splat('a' as u8);
    const v_26: FormVec16 = @splat(26 as u8);
    const v_0: FormVec16 = @splat('0' as u8);
    const v_10: FormVec16 = @splat(10 as u8);
    const v_case: FormVec16 = @splat(0x20 as u8);
    const v_star: FormVec16 = @splat('*' as u8);
    const v_dash: FormVec16 = @splat('-' as u8);
    const v_dot: FormVec16 = @splat('.' as u8);
    const v_under: FormVec16 = @splat('_' as u8);
    while (i + 16 <= s.len) : (i += 16) {
        const v: FormVec16 = s[i .. i + 16][0..16].*;
        const vl = v | v_case;
        const m_alpha: u16 = @bitCast((vl -% v_a) < v_26);
        const m_dig: u16 = @bitCast((v -% v_0) < v_10);
        const m_sym: u16 = @bitCast((v == v_star) | (v == v_dash) | (v == v_dot) | (v == v_under));
        if ((m_alpha | m_dig | m_sym) != 0xFFFF) break;
    }
    while (i < s.len && FORM_SAFE_LUT[@as(usize, s[i])] != 0) : (i += 1) {}
    return i;
}

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i + 16 <= a.len) : (i += 16) {
        const va: FormVec16 = a[i .. i + 16][0..16].*;
        const vb: FormVec16 = b[i .. i + 16][0..16].*;
        if (@reduce(.Or, va ^ vb) != 0) return false;
    }
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

/// WHATWG application/x-www-form-urlencoded encoder (space -> `+`, unsafe bytes -> `%XX`).
export fn encodeFormComponent(raw: []const u8, out: []u8): ([]const u8)? {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < raw.len) {
        const safe_end = formScanSafeRun(raw, i);
        if (safe_end > i) {
            const n = safe_end - i;
            if (pos + n > out.len) return null;
            @memcpy(out[pos .. pos + n], raw[i..safe_end]);
            pos += n;
            i = safe_end;
            if (i >= raw.len) break;
        }
        const c = raw[i];
        if (c == ' ') {
            if (pos >= out.len) return null;
            out[pos] = '+';
            pos += 1;
        } else {
            if (pos + 3 > out.len) return null;
            out[pos] = '%';
            out[pos + 1] = FORM_HEX_UPPER[@as(usize, (c >> 4) & 0x0F)];
            out[pos + 2] = FORM_HEX_UPPER[@as(usize, c & 0x0F)];
            pos += 3;
        }
        i += 1;
    }
    return out[0..pos];
}

/// WHATWG application/x-www-form-urlencoded decoder (`+` -> space, `%XX` -> byte).
/// Invalid `%` sequences are preserved verbatim per WHATWG URL Standard.
export fn decodeFormComponent(encoded: []const u8, out: []u8): ([]const u8)? {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < encoded.len) {
        const next_sp = formFindDecodeSpecial(encoded, i);
        if (next_sp > i) {
            const n = next_sp - i;
            if (pos + n > out.len) return null;
            @memcpy(out[pos .. pos + n], encoded[i..next_sp]);
            pos += n;
            i = next_sp;
            if (i >= encoded.len) break;
        }
        const c = encoded[i];
        if (c == '+') {
            if (pos >= out.len) return null;
            out[pos] = ' ';
            pos += 1;
            i += 1;
        } else if (c == '%' && i + 2 < encoded.len) {
            const hi = FORM_HEX_VAL_LUT[@as(usize, encoded[i + 1])];
            const lo = FORM_HEX_VAL_LUT[@as(usize, encoded[i + 2])];
            if (hi != 0xFF && lo != 0xFF) {
                if (pos >= out.len) return null;
                out[pos] = (hi << 4) | lo;
                pos += 1;
                i += 3;
            } else {
                if (pos >= out.len) return null;
                out[pos] = c;
                pos += 1;
                i += 1;
            }
        } else {
            if (pos >= out.len) return null;
            out[pos] = c;
            pos += 1;
            i += 1;
        }
    }
    return out[0..pos];
}

export struct FormPairRaw {
    raw_key: []const u8,
    raw_value: []const u8,

    pub fn decodeKey(out: []u8): ([]const u8)? {
        return decodeFormComponent(_.raw_key, out);
    }

    pub fn decodeValue(out: []u8): ([]const u8)? {
        return decodeFormComponent(_.raw_value, out);
    }
}

export struct FormIter {
    src: []const u8,
    pos: usize,

    pub static fn init(body: []const u8): FormIter {
        let start: usize = 0;
        if (body.len > 0 && body[0] == '?') start = 1;
        return FormIter { src: body, pos: start };
    }

    pub fn next(*_): FormPairRaw? {
        while (_.pos < _.src.len) {
            const start = _.pos;
            _.pos = formFindByte(_.src, _.pos, '&');
            const end = _.pos;
            if (_.pos < _.src.len && _.src[_.pos] == '&') {
                _.pos += 1;
            }
            const seg = _.src[start..end];
            if (seg.len == 0) continue;

            const eq_pos = formFindByte(seg, 0, '=');
            if (eq_pos < seg.len) {
                return FormPairRaw {
                    raw_key: seg[0..eq_pos],
                    raw_value: seg[eq_pos + 1..],
                };
            }
            return FormPairRaw {
                raw_key: seg,
                raw_value: "",
            };
        }
        return null;
    }
}

/// Lookup the first value whose decoded key matches `target_key` and write its decoded value into `out`.
export fn getFormParam(body: []const u8, target_key: []const u8, out: []u8): ([]const u8)? {
    let key_buf: [256]u8 = undefined;
    let it = FormIter.init(body);
    while (pair := it.next()) {
        if (dk := pair.decodeKey(key_buf[0..])) {
            if (strEq(dk, target_key)) {
                return pair.decodeValue(out);
            }
        }
    }
    return null;
}

/// Append an encoded `key=value` entry to `out` at `pos` (inserting `&` if `pos > 0`).
export fn appendFormParam(out: []u8, pos: *usize, key: []const u8, val: []const u8): bool {
    if (pos.* > 0) {
        if (pos.* >= out.len) return false;
        out[pos.*] = '&';
        pos.* += 1;
    }
    const ek = encodeFormComponent(key, out[pos.*..]) ?? return false;
    pos.* += ek.len;
    if (pos.* >= out.len) return false;
    out[pos.*] = '=';
    pos.* += 1;
    const ev = encodeFormComponent(val, out[pos.*..]) ?? return false;
    pos.* += ev.len;
    return true;
}
