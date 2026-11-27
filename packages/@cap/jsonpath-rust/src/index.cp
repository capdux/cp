// @cap/jsonpath-rust — Zero-heap RFC 9535 JSONPath parser & serializer

pub enum JpSegKind {
    Root,
    Property,
    Index,
    Wildcard,
    RecursiveProp,
    RecursiveWildcard,
    Slice,
}

pub struct JpSegment {
    kind: JpSegKind,
    name: string,
    index: i64,
    slice_start: i64,
    slice_end: i64,
    has_start: bool,
    has_end: bool,
}

fn jprIsWs(c: u8): bool {
    return c == 32 || c == 9 || c == 10 || c == 13;
}

fn jprIsDigit(c: u8): bool {
    return c >= 48 && c <= 57;
}

fn jprIsIdentStart(c: u8): bool {
    return (c >= 65 && c <= 90) || (c >= 97 && c <= 122) || c == 95 || c >= 128;
}

fn jprIsIdentCont(c: u8): bool {
    return jprIsIdentStart(c) || jprIsDigit(c) || c == 45;
}

fn jprIsSimpleIdent(s: string): bool {
    if (s.len == 0 || !jprIsIdentStart(s[0])) {
        return false;
    }
    let i: usize = 1;
    while (i < s.len) : (i += 1) {
        if (!jprIsIdentCont(s[i])) {
            return false;
        }
    }
    return true;
}

fn jprMakeSeg(kind: JpSegKind, name: string): JpSegment {
    return JpSegment{
        kind: kind,
        name: name,
        index: 0,
        slice_start: 0,
        slice_end: 0,
        has_start: false,
        has_end: false,
    };
}

struct JprIntRes {
    val: i64,
    next_pos: usize,
    ok: bool,
}

fn jprParseInt(s: string, start_pos: usize): JprIntRes {
    const n: usize = s.len;
    let i: usize = start_pos;
    let neg: bool = false;
    if (i < n && s[i] == 45) {
        neg = true;
        i += 1;
    }
    if (i >= n || !jprIsDigit(s[i])) {
        return JprIntRes{ val: 0, next_pos: start_pos, ok: false };
    }
    let acc: i64 = 0;
    while (i < n && jprIsDigit(s[i])) : (i += 1) {
        acc = acc * 10 + ((s[i] - 48) as i64);
    }
    const final_val: i64 = if (neg) -acc else acc;
    return JprIntRes{ val: final_val, next_pos: i, ok: true };
}

pub fn parseJsonPath(expr: string, out: []JpSegment): i64 {
    const n: usize = expr.len;
    if (n == 0 || expr[0] != 36) {
        return -1;
    }
    if (out.len == 0) {
        return -1;
    }
    out[0] = jprMakeSeg(JpSegKind.Root, "$");
    let count: usize = 1;
    let i: usize = 1;

    while (i < n) {
        const c: u8 = expr[i];

        // Recursive descent: '..'
        if (c == 46 && i + 1 < n && expr[i + 1] == 46) {
            i += 2;
            if (i >= n || count >= out.len) {
                return -1;
            }
            if (expr[i] == 42) {
                out[count] = jprMakeSeg(JpSegKind.RecursiveWildcard, "*");
                count += 1;
                i += 1;
                continue;
            }
            if (expr[i] == 91) {
                i += 1;
                while (i < n && jprIsWs(expr[i])) : (i += 1) {}
                if (i >= n) {
                    return -1;
                }
                if (expr[i] == 42) {
                    i += 1;
                    while (i < n && jprIsWs(expr[i])) : (i += 1) {}
                    if (i >= n || expr[i] != 93) {
                        return -1;
                    }
                    i += 1;
                    out[count] = jprMakeSeg(JpSegKind.RecursiveWildcard, "*");
                    count += 1;
                    continue;
                }
                if (expr[i] == 39 || expr[i] == 34) {
                    const q: u8 = expr[i];
                    i += 1;
                    const ks: usize = i;
                    while (i < n && expr[i] != q) {
                        if (expr[i] == 92 && i + 1 < n) {
                            i += 2;
                            continue;
                        }
                        i += 1;
                    }
                    if (i >= n) {
                        return -1;
                    }
                    const ke: usize = i;
                    i += 1;
                    while (i < n && jprIsWs(expr[i])) : (i += 1) {}
                    if (i >= n || expr[i] != 93) {
                        return -1;
                    }
                    i += 1;
                    out[count] = jprMakeSeg(JpSegKind.RecursiveProp, expr[ks..ke]);
                    count += 1;
                    continue;
                }
                return -1;
            }
            if (!jprIsIdentStart(expr[i])) {
                return -1;
            }
            const ps: usize = i;
            i += 1;
            while (i < n && jprIsIdentCont(expr[i])) : (i += 1) {}
            out[count] = jprMakeSeg(JpSegKind.RecursiveProp, expr[ps..i]);
            count += 1;
            continue;
        }

        // Dot child: '.'
        if (c == 46) {
            i += 1;
            if (i >= n || count >= out.len) {
                return -1;
            }
            if (expr[i] == 42) {
                out[count] = jprMakeSeg(JpSegKind.Wildcard, "*");
                count += 1;
                i += 1;
                continue;
            }
            if (!jprIsIdentStart(expr[i])) {
                return -1;
            }
            const ps: usize = i;
            i += 1;
            while (i < n && jprIsIdentCont(expr[i])) : (i += 1) {}
            out[count] = jprMakeSeg(JpSegKind.Property, expr[ps..i]);
            count += 1;
            continue;
        }

        // Bracket selector: '[' ... ']'
        if (c == 91) {
            i += 1;
            if (count >= out.len) {
                return -1;
            }
            while (i < n && jprIsWs(expr[i])) : (i += 1) {}
            if (i >= n) {
                return -1;
            }

            // Wildcard [*]
            if (expr[i] == 42) {
                i += 1;
                while (i < n && jprIsWs(expr[i])) : (i += 1) {}
                if (i >= n || expr[i] != 93) {
                    return -1;
                }
                i += 1;
                out[count] = jprMakeSeg(JpSegKind.Wildcard, "*");
                count += 1;
                continue;
            }

            // Quoted property ['...'] or ["..."]
            if (expr[i] == 39 || expr[i] == 34) {
                const q: u8 = expr[i];
                i += 1;
                const ks: usize = i;
                while (i < n && expr[i] != q) {
                    if (expr[i] == 92 && i + 1 < n) {
                        i += 2;
                        continue;
                    }
                    i += 1;
                }
                if (i >= n) {
                    return -1;
                }
                const ke: usize = i;
                i += 1;
                while (i < n && jprIsWs(expr[i])) : (i += 1) {}
                if (i >= n || expr[i] != 93) {
                    return -1;
                }
                i += 1;
                out[count] = jprMakeSeg(JpSegKind.Property, expr[ks..ke]);
                count += 1;
                continue;
            }

            // Index or Slice
            const r1 = jprParseInt(expr, i);
            if (r1.ok) {
                i = r1.next_pos;
            }
            while (i < n && jprIsWs(expr[i])) : (i += 1) {}
            if (i >= n) {
                return -1;
            }
            if (expr[i] == 93) {
                if (!r1.ok) {
                    return -1;
                }
                i += 1;
                out[count] = JpSegment{
                    kind: JpSegKind.Index,
                    name: "",
                    index: r1.val,
                    slice_start: 0,
                    slice_end: 0,
                    has_start: false,
                    has_end: false,
                };
                count += 1;
                continue;
            }
            if (expr[i] == 58) {
                i += 1;
                while (i < n && jprIsWs(expr[i])) : (i += 1) {}
                const r2 = jprParseInt(expr, i);
                if (r2.ok) {
                    i = r2.next_pos;
                }
                while (i < n && jprIsWs(expr[i])) : (i += 1) {}
                if (i >= n || expr[i] != 93) {
                    return -1;
                }
                i += 1;
                out[count] = JpSegment{
                    kind: JpSegKind.Slice,
                    name: "",
                    index: 0,
                    slice_start: r1.val,
                    slice_end: r2.val,
                    has_start: r1.ok,
                    has_end: r2.ok,
                };
                count += 1;
                continue;
            }
            return -1;
        }

        return -1;
    }

    return count as i64;
}

fn jprPushByte(out: []u8, pos: usize, b: u8): usize {
    if (pos < out.len) {
        out[pos] = b;
        return pos + 1;
    }
    return pos;
}

fn jprPushSlice(out: []u8, pos: usize, s: string): usize {
    let p: usize = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) : (i += 1) {
        out[p] = s[i];
        p += 1;
    }
    return p;
}

fn jprPushI64(out: []u8, pos: usize, val: i64): usize {
    let p: usize = pos;
    if (val == 0) {
        return jprPushByte(out, p, 48);
    }
    let uv: u64 = 0;
    if (val < 0) {
        p = jprPushByte(out, p, 45);
        uv = (-val) as u64;
    } else {
        uv = val as u64;
    }
    let digits: [24]u8 = [0; 24];
    let dlen: usize = 0;
    while (uv > 0 && dlen < 24) {
        const rem: u64 = uv % 10;
        digits[dlen] = (48 + rem) as u8;
        dlen += 1;
        uv = uv / 10;
    }
    while (dlen > 0) {
        dlen -= 1;
        p = jprPushByte(out, p, digits[dlen]);
    }
    return p;
}

pub fn formatJsonPath(segs: []const JpSegment, out: []u8): usize {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < segs.len) : (i += 1) {
        const seg = segs[i];
        if (seg.kind == JpSegKind.Root) {
            pos = jprPushByte(out, pos, 36);
        } else if (seg.kind == JpSegKind.Property) {
            if (jprIsSimpleIdent(seg.name)) {
                pos = jprPushByte(out, pos, 46);
                pos = jprPushSlice(out, pos, seg.name);
            } else {
                pos = jprPushSlice(out, pos, "['");
                pos = jprPushSlice(out, pos, seg.name);
                pos = jprPushSlice(out, pos, "']");
            }
        } else if (seg.kind == JpSegKind.Index) {
            pos = jprPushByte(out, pos, 91);
            pos = jprPushI64(out, pos, seg.index);
            pos = jprPushByte(out, pos, 93);
        } else if (seg.kind == JpSegKind.Wildcard) {
            pos = jprPushSlice(out, pos, "[*]");
        } else if (seg.kind == JpSegKind.RecursiveProp) {
            if (jprIsSimpleIdent(seg.name)) {
                pos = jprPushSlice(out, pos, "..");
                pos = jprPushSlice(out, pos, seg.name);
            } else {
                pos = jprPushSlice(out, pos, "..['");
                pos = jprPushSlice(out, pos, seg.name);
                pos = jprPushSlice(out, pos, "']");
            }
        } else if (seg.kind == JpSegKind.RecursiveWildcard) {
            pos = jprPushSlice(out, pos, "..*");
        } else if (seg.kind == JpSegKind.Slice) {
            pos = jprPushByte(out, pos, 91);
            if (seg.has_start) {
                pos = jprPushI64(out, pos, seg.slice_start);
            }
            pos = jprPushByte(out, pos, 58);
            if (seg.has_end) {
                pos = jprPushI64(out, pos, seg.slice_end);
            }
            pos = jprPushByte(out, pos, 93);
        }
    }
    return pos;
}
