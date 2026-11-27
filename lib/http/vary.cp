// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-23T01:59:15Z (RFC 9110 Vary Header Parser & Cache-Key Normalizer)
// Pure-CP zero-allocation RFC 9110 §12.5.5 Vary header iterator, membership check, and canonical merger.

type VaryVec16 = @Vector(16, u8);

fn makeVaryLowerLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        if (i >= 'A' && i <= 'Z') {
            lut[i] = @as(u8, @intCast(i + 32));
        } else {
            lut[i] = @as(u8, @intCast(i));
        }
    }
    return lut;
}

fn makeVaryTokenLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        const b = @as(u8, @intCast(i));
        const ok = (b >= 'a' && b <= 'z') || (b >= 'A' && b <= 'Z') || (b >= '0' && b <= '9') ||
            b == '!' || b == '#' || b == '$' || b == '%' || b == '&' || b == '\'' ||
            b == '*' || b == '+' || b == '-' || b == '.' || b == '^' || b == '_' ||
            b == '`' || b == '|' || b == '~';
        lut[i] = if (ok) 1 else 0;
    }
    return lut;
}

const VARY_LOWER_LUT: [u8; 256] = makeVaryLowerLut();
const VARY_TOKEN_LUT: [u8; 256] = makeVaryTokenLut();

fn varyFindComma(s: []const u8, start: usize): usize {
    let i: usize = start;
    const v_comma: VaryVec16 = @splat(',' as u8);
    while (i + 16 <= s.len) : (i += 16) {
        const v: VaryVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_comma)) break;
    }
    while (i < s.len) : (i += 1) {
        if (s[i] == ',') return i;
    }
    return s.len;
}

fn isWs(b: u8): bool {
    return b == ' ' || b == '\t' || b == '\r' || b == '\n';
}

fn trimWs(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isWs(s[start])) start += 1;
    let end: usize = s.len;
    while (end > start && isWs(s[end - 1])) end -= 1;
    return s[start..end];
}

fn eqIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    const lo_A: VaryVec16 = @splat(65 as u8);
    const hi_Z: VaryVec16 = @splat(90 as u8);
    const case_bit: VaryVec16 = @splat(0x20 as u8);
    while (i + 16 <= a.len) : (i += 16) {
        const va: VaryVec16 = a[i .. i + 16][0..16].*;
        const vb: VaryVec16 = b[i .. i + 16][0..16].*;
        const la = @select(u8, (va >= lo_A) & (va <= hi_Z), va | case_bit, va);
        const lb = @select(u8, (vb >= lo_A) & (vb <= hi_Z), vb | case_bit, vb);
        if (@reduce(.Or, la ^ lb) != 0) return false;
    }
    while (i < a.len) {
        if (VARY_LOWER_LUT[@as(usize, a[i])] != VARY_LOWER_LUT[@as(usize, b[i])]) return false;
        i += 1;
    }
    return true;
}

fn isValidFieldName(name: []const u8): bool {
    if (name.len == 0) return false;
    let i: usize = 0;
    const v_a: VaryVec16 = @splat('a' as u8);
    const v_26: VaryVec16 = @splat(26 as u8);
    const v_0: VaryVec16 = @splat('0' as u8);
    const v_10: VaryVec16 = @splat(10 as u8);
    const v_dash: VaryVec16 = @splat('-' as u8);
    const v_under: VaryVec16 = @splat('_' as u8);
    const v_case: VaryVec16 = @splat(0x20 as u8);
    while (i + 16 <= name.len) : (i += 16) {
        const v: VaryVec16 = name[i .. i + 16][0..16].*;
        const vl = v | v_case;
        const m_alpha: u16 = @bitCast((vl -% v_a) < v_26);
        const m_dig: u16 = @bitCast((v -% v_0) < v_10);
        const m_sym: u16 = @bitCast((v == v_dash) | (v == v_under));
        if ((m_alpha | m_dig | m_sym) != 0xFFFF) {
            let k: usize = 0;
            while (k < 16) : (k += 1) {
                if (VARY_TOKEN_LUT[@as(usize, name[i + k])] == 0) return false;
            }
        }
    }
    while (i < name.len) {
        if (VARY_TOKEN_LUT[@as(usize, name[i])] == 0) return false;
        i += 1;
    }
    return true;
}

export struct VaryIter {
    src: []const u8,
    pos: usize,

    pub static fn init(header: []const u8): VaryIter {
        return VaryIter { src: header, pos: 0 };
    }

    pub fn next(*_): ([]const u8)? {
        while (_.pos < _.src.len) {
            const start = _.pos;
            _.pos = varyFindComma(_.src, _.pos);
            const end = _.pos;
            if (_.pos < _.src.len && _.src[_.pos] == ',') {
                _.pos += 1;
            }
            const tok = trimWs(_.src[start..end]);
            if (tok.len > 0 && isValidFieldName(tok)) {
                return tok;
            }
        }
        return null;
    }
}

/// Returns true if the `Vary` header contains `*` (uncacheable / matches nothing per RFC 9110 §12.5.5).
export fn isVaryWildcard(header: []const u8): bool {
    let it = VaryIter.init(header);
    while (name := it.next()) {
        if (name.len == 1 && name[0] == '*') return true;
    }
    return false;
}

/// Check if `field_name` is listed in `Vary` header (case-insensitive).
/// Note: returns false for `*` unless `field_name` is literally `"*"`; use `isVaryWildcard` separately.
export fn varyContains(header: []const u8, field_name: []const u8): bool {
    if (field_name.len == 0) return false;
    let it = VaryIter.init(header);
    while (name := it.next()) {
        if (eqIgnoreCase(name, field_name)) return true;
    }
    return false;
}

/// Append a field name to an existing `Vary` header value with case-insensitive deduplication.
/// If either existing or `field_name` is `*`, outputs `"*"`.
export fn appendVary(existing: []const u8, field_name: []const u8, out: []u8): ([]const u8)? {
    const clean_add = trimWs(field_name);
    if (!isValidFieldName(clean_add)) return null;

    if (isVaryWildcard(existing) || (clean_add.len == 1 && clean_add[0] == '*')) {
        if (out.len < 1) return null;
        out[0] = '*';
        return out[0..1];
    }

    let pos: usize = 0;
    let first = true;
    let already = false;

    let it = VaryIter.init(existing);
    while (name := it.next()) {
        if (eqIgnoreCase(name, clean_add)) {
            already = true;
        }
        if (!first) {
            if (pos + 2 > out.len) return null;
            out[pos] = ',';
            out[pos + 1] = ' ';
            pos += 2;
        }
        first = false;
        if (pos + name.len > out.len) return null;
        if (name.len > 0) {
            @memcpy(out[pos .. pos + name.len], name);
        }
        pos += name.len;
    }

    if (!already) {
        if (!first) {
            if (pos + 2 > out.len) return null;
            out[pos] = ',';
            out[pos + 1] = ' ';
            pos += 2;
        }
        if (pos + clean_add.len > out.len) return null;
        if (clean_add.len > 0) {
            @memcpy(out[pos .. pos + clean_add.len], clean_add);
        }
        pos += clean_add.len;
    }

    return out[0..pos];
}

/// Merge two `Vary` headers into a single deduplicated lowercase canonical header value.
/// If either contains `*`, returns `"*"`.
export fn mergeVary(a: []const u8, b: []const u8, out: []u8): ([]const u8)? {
    if (isVaryWildcard(a) || isVaryWildcard(b)) {
        if (out.len < 1) return null;
        out[0] = '*';
        return out[0..1];
    }

    let pos: usize = 0;
    let first = true;

    let it_a = VaryIter.init(a);
    while (name := it_a.next()) {
        if (varyContains(out[0..pos], name)) continue;
        if (!appendLower(out, &pos, &first, name)) return null;
    }

    let it_b = VaryIter.init(b);
    while (name := it_b.next()) {
        if (varyContains(out[0..pos], name)) continue;
        if (!appendLower(out, &pos, &first, name)) return null;
    }

    return out[0..pos];
}

fn appendLower(out: []u8, pos: *usize, first: *bool, name: []const u8): bool {
    if (!first.*) {
        if (pos.* + 2 > out.len) return false;
        out[pos.*] = ',';
        out[pos.* + 1] = ' ';
        pos.* += 2;
    }
    first.* = false;
    if (pos.* + name.len > out.len) return false;
    const base = pos.*;
    let i: usize = 0;
    const lo_A: VaryVec16 = @splat(65 as u8);
    const hi_Z: VaryVec16 = @splat(90 as u8);
    const case_bit: VaryVec16 = @splat(0x20 as u8);
    while (i + 16 <= name.len) : (i += 16) {
        const v: VaryVec16 = name[i .. i + 16][0..16].*;
        const lo = @select(u8, (v >= lo_A) & (v <= hi_Z), v | case_bit, v);
        out[base + i .. base + i + 16][0..16].* = lo;
    }
    while (i < name.len) {
        out[base + i] = VARY_LOWER_LUT[@as(usize, name[i])];
        i += 1;
    }
    pos.* = base + name.len;
    return true;
}
