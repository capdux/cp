// packages/@cap/json-schema-traverse/src/index.cp
// Clean-room zero-allocation JSON Schema traversal engine for CP.
// Parity with npm:json-schema-traverse (AJV official schema traverser).

pub enum SchemaNodeKind {
    Root,
    Property,
    Items,
    AdditionalProperties,
    AllOf,
    AnyOf,
    OneOf,
    Not,
    Defs,
}

pub struct SchemaVisitEvent {
    pointer_start: usize,
    pointer_end: usize,
    pointer_buf: [128]u8,
    pointer_len: usize,
    kind: SchemaNodeKind,
    schema_start: usize,
    schema_end: usize,
    depth: usize,

    fn pointer(): string {
        return _.pointer_buf[0.._.pointer_len];
    }

    fn schemaSlice(src: string): string {
        if (_.schema_start <= _.schema_end && _.schema_end <= src.len) {
            return src[_.schema_start.._.schema_end];
        }
        return "";
    }
}

struct JstvSpan {
    start: usize,
    end: usize,
    ok: bool,
}

struct JstvTraverseState {
    src: string,
    out: []SchemaVisitEvent,
    written: usize,
    total: usize,
    target_ptr: string,
    find_mode: bool,
    found: bool,
    found_start: usize,
    found_end: usize,
}

const JSTV_MAX_DEPTH: usize = 32;

fn jstvIsWs(b: u8): bool {
    return b == ' ' || b == '\t' || b == '\n' || b == '\r';
}

fn jstvSkipWs(src: string, start: usize): usize {
    let i = start;
    while (i < src.len && jstvIsWs(src[i])) {
        i += 1;
    }
    return i;
}

fn jstvScanString(src: string, start: usize): JstvSpan {
    if (start >= src.len || src[start] != '"') {
        return JstvSpan{ start: start, end: start, ok: false };
    }
    let i = start + 1;
    while (i < src.len) {
        const ch = src[i];
        if (ch == '\\') {
            if (i + 1 >= src.len) {
                return JstvSpan{ start: start, end: i, ok: false };
            }
            i += 2;
            continue;
        }
        if (ch == '"') {
            return JstvSpan{ start: start, end: i + 1, ok: true };
        }
        i += 1;
    }
    return JstvSpan{ start: start, end: src.len, ok: false };
}

fn jstvScanValue(src: string, start: usize): JstvSpan {
    const pos = jstvSkipWs(src, start);
    if (pos >= src.len) {
        return JstvSpan{ start: pos, end: pos, ok: false };
    }
    const ch = src[pos];
    if (ch == '"') {
        return jstvScanString(src, pos);
    }
    if (ch == '{' || ch == '[') {
        let i = pos;
        let depth: usize = 0;
        while (i < src.len) {
            const c = src[i];
            if (c == '"') {
                const s = jstvScanString(src, i);
                if (!s.ok) {
                    return JstvSpan{ start: pos, end: i, ok: false };
                }
                i = s.end;
                continue;
            }
            if (c == '{' || c == '[') {
                depth += 1;
            } else if (c == '}' || c == ']') {
                if (depth == 0) {
                    return JstvSpan{ start: pos, end: i, ok: false };
                }
                depth -= 1;
                if (depth == 0) {
                    return JstvSpan{ start: pos, end: i + 1, ok: true };
                }
            }
            i += 1;
        }
        return JstvSpan{ start: pos, end: src.len, ok: false };
    }
    // Primitive: number, true, false, null
    let i = pos;
    while (i < src.len) {
        const c = src[i];
        if (jstvIsWs(c) || c == ',' || c == '}' || c == ']') {
            break;
        }
        i += 1;
    }
    if (i == pos) {
        return JstvSpan{ start: pos, end: pos, ok: false };
    }
    return JstvSpan{ start: pos, end: i, ok: true };
}

fn jstvStrEq(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn jstvCopyBuf(src: [128]u8, len: usize): [128]u8 {
    let dst: [128]u8 = undefined;
    let i: usize = 0;
    const lim = if (len < 128) len else 128;
    while (i < lim) : (i += 1) {
        dst[i] = src[i];
    }
    return dst;
}

fn jstvAppendByte(buf: *[128]u8, cur_len: usize, b: u8): usize {
    if (cur_len < 128) {
        buf[cur_len] = b;
        return cur_len + 1;
    }
    return cur_len;
}

fn jstvAppendEscapedSegment(buf: *[128]u8, cur_len: usize, seg: string): usize {
    let n = jstvAppendByte(buf, cur_len, '/');
    let i: usize = 0;
    while (i < seg.len) : (i += 1) {
        const c = seg[i];
        if (c == '~') {
            n = jstvAppendByte(buf, n, '~');
            n = jstvAppendByte(buf, n, '0');
        } else if (c == '/') {
            n = jstvAppendByte(buf, n, '~');
            n = jstvAppendByte(buf, n, '1');
        } else {
            n = jstvAppendByte(buf, n, c);
        }
    }
    return n;
}

fn jstvAppendIndexSegment(buf: *[128]u8, cur_len: usize, idx: usize): usize {
    let n = jstvAppendByte(buf, cur_len, '/');
    if (idx == 0) {
        return jstvAppendByte(buf, n, '0');
    }
    let digits: [20]u8 = undefined;
    let dlen: usize = 0;
    let v = idx;
    while (v > 0 && dlen < 20) {
        digits[dlen] = u8('0' + (v % 10));
        dlen += 1;
        v = v / 10;
    }
    while (dlen > 0) {
        dlen -= 1;
        n = jstvAppendByte(buf, n, digits[dlen]);
    }
    return n;
}

fn jstvEmitEvent(
    st: *JstvTraverseState,
    ptr_buf: [128]u8,
    ptr_len: usize,
    kind: SchemaNodeKind,
    schema_start: usize,
    schema_end: usize,
    depth: usize,
) {
    st.total += 1;
    if (st.find_mode && !st.found) {
        const cur_ptr = ptr_buf[0..ptr_len];
        if (jstvStrEq(cur_ptr, st.target_ptr)) {
            st.found = true;
            st.found_start = schema_start;
            st.found_end = schema_end;
        }
    }
    if (st.written < st.out.len) {
        let ev = SchemaVisitEvent{
            pointer_start: 0,
            pointer_end: ptr_len,
            pointer_buf: ptr_buf,
            pointer_len: ptr_len,
            kind: kind,
            schema_start: schema_start,
            schema_end: schema_end,
            depth: depth,
        };
        st.out[st.written] = ev;
        st.written += 1;
    }
}

fn jstvWalkMapKeyword(
    st: *JstvTraverseState,
    obj_start: usize,
    obj_end: usize,
    base_buf: [128]u8,
    base_len: usize,
    kw: string,
    kind: SchemaNodeKind,
    depth: usize,
) {
    if (obj_start >= obj_end || st.src[obj_start] != '{') return;
    let kw_buf = jstvCopyBuf(base_buf, base_len);
    const kw_len = jstvAppendEscapedSegment(*kw_buf, base_len, kw);

    let i = jstvSkipWs(st.src, obj_start + 1);
    while (i < obj_end) {
        if (st.src[i] == '}') break;
        const kspan = jstvScanString(st.src, i);
        if (!kspan.ok) break;
        const prop_name = st.src[kspan.start + 1..kspan.end - 1];
        i = jstvSkipWs(st.src, kspan.end);
        if (i >= obj_end || st.src[i] != ':') break;
        i = jstvSkipWs(st.src, i + 1);
        const vspan = jstvScanValue(st.src, i);
        if (!vspan.ok) break;

        if (vspan.start < vspan.end && st.src[vspan.start] == '{') {
            let child_buf = jstvCopyBuf(kw_buf, kw_len);
            const child_len = jstvAppendEscapedSegment(*child_buf, kw_len, prop_name);
            jstvWalkSchema(st, vspan.start, vspan.end, child_buf, child_len, kind, depth);
        }

        i = jstvSkipWs(st.src, vspan.end);
        if (i < obj_end && st.src[i] == ',') {
            i = jstvSkipWs(st.src, i + 1);
        } else {
            break;
        }
    }
}

fn jstvWalkArrayKeyword(
    st: *JstvTraverseState,
    arr_start: usize,
    arr_end: usize,
    base_buf: [128]u8,
    base_len: usize,
    kw: string,
    kind: SchemaNodeKind,
    depth: usize,
) {
    if (arr_start >= arr_end || st.src[arr_start] != '[') return;
    let kw_buf = jstvCopyBuf(base_buf, base_len);
    const kw_len = jstvAppendEscapedSegment(*kw_buf, base_len, kw);

    let i = jstvSkipWs(st.src, arr_start + 1);
    let idx: usize = 0;
    while (i < arr_end) {
        if (st.src[i] == ']') break;
        const vspan = jstvScanValue(st.src, i);
        if (!vspan.ok) break;

        if (vspan.start < vspan.end && st.src[vspan.start] == '{') {
            let child_buf = jstvCopyBuf(kw_buf, kw_len);
            const child_len = jstvAppendIndexSegment(*child_buf, kw_len, idx);
            jstvWalkSchema(st, vspan.start, vspan.end, child_buf, child_len, kind, depth);
        }

        idx += 1;
        i = jstvSkipWs(st.src, vspan.end);
        if (i < arr_end && st.src[i] == ',') {
            i = jstvSkipWs(st.src, i + 1);
        } else {
            break;
        }
    }
}

fn jstvWalkSchema(
    st: *JstvTraverseState,
    schema_start: usize,
    schema_end: usize,
    ptr_buf: [128]u8,
    ptr_len: usize,
    kind: SchemaNodeKind,
    depth: usize,
) {
    if (depth > JSTV_MAX_DEPTH) return;
    if (schema_start >= schema_end || st.src[schema_start] != '{') return;

    jstvEmitEvent(st, ptr_buf, ptr_len, kind, schema_start, schema_end, depth);
    if (st.find_mode && st.found) return;

    let i = jstvSkipWs(st.src, schema_start + 1);
    while (i < schema_end) {
        if (st.src[i] == '}') break;
        const kspan = jstvScanString(st.src, i);
        if (!kspan.ok) break;
        const key = st.src[kspan.start + 1..kspan.end - 1];
        i = jstvSkipWs(st.src, kspan.end);
        if (i >= schema_end || st.src[i] != ':') break;
        i = jstvSkipWs(st.src, i + 1);
        const vspan = jstvScanValue(st.src, i);
        if (!vspan.ok) break;

        const first_ch = if (vspan.start < vspan.end) st.src[vspan.start] else u8(0);
        if (jstvStrEq(key, "properties")) {
            if (first_ch == '{') {
                jstvWalkMapKeyword(st, vspan.start, vspan.end, ptr_buf, ptr_len, "properties", .Property, depth + 1);
            }
        } else if (jstvStrEq(key, "patternProperties")) {
            if (first_ch == '{') {
                jstvWalkMapKeyword(st, vspan.start, vspan.end, ptr_buf, ptr_len, "patternProperties", .Property, depth + 1);
            }
        } else if (jstvStrEq(key, "$defs")) {
            if (first_ch == '{') {
                jstvWalkMapKeyword(st, vspan.start, vspan.end, ptr_buf, ptr_len, "$defs", .Defs, depth + 1);
            }
        } else if (jstvStrEq(key, "definitions")) {
            if (first_ch == '{') {
                jstvWalkMapKeyword(st, vspan.start, vspan.end, ptr_buf, ptr_len, "definitions", .Defs, depth + 1);
            }
        } else if (jstvStrEq(key, "items")) {
            if (first_ch == '{') {
                let child_buf = jstvCopyBuf(ptr_buf, ptr_len);
                const child_len = jstvAppendEscapedSegment(*child_buf, ptr_len, "items");
                jstvWalkSchema(st, vspan.start, vspan.end, child_buf, child_len, .Items, depth + 1);
            } else if (first_ch == '[') {
                jstvWalkArrayKeyword(st, vspan.start, vspan.end, ptr_buf, ptr_len, "items", .Items, depth + 1);
            }
        } else if (jstvStrEq(key, "additionalProperties")) {
            if (first_ch == '{') {
                let child_buf = jstvCopyBuf(ptr_buf, ptr_len);
                const child_len = jstvAppendEscapedSegment(*child_buf, ptr_len, "additionalProperties");
                jstvWalkSchema(st, vspan.start, vspan.end, child_buf, child_len, .AdditionalProperties, depth + 1);
            }
        } else if (jstvStrEq(key, "not")) {
            if (first_ch == '{') {
                let child_buf = jstvCopyBuf(ptr_buf, ptr_len);
                const child_len = jstvAppendEscapedSegment(*child_buf, ptr_len, "not");
                jstvWalkSchema(st, vspan.start, vspan.end, child_buf, child_len, .Not, depth + 1);
            }
        } else if (jstvStrEq(key, "allOf")) {
            if (first_ch == '[') {
                jstvWalkArrayKeyword(st, vspan.start, vspan.end, ptr_buf, ptr_len, "allOf", .AllOf, depth + 1);
            }
        } else if (jstvStrEq(key, "anyOf")) {
            if (first_ch == '[') {
                jstvWalkArrayKeyword(st, vspan.start, vspan.end, ptr_buf, ptr_len, "anyOf", .AnyOf, depth + 1);
            }
        } else if (jstvStrEq(key, "oneOf")) {
            if (first_ch == '[') {
                jstvWalkArrayKeyword(st, vspan.start, vspan.end, ptr_buf, ptr_len, "oneOf", .OneOf, depth + 1);
            }
        }

        if (st.find_mode && st.found) return;

        i = jstvSkipWs(st.src, vspan.end);
        if (i < schema_end && st.src[i] == ',') {
            i = jstvSkipWs(st.src, i + 1);
        } else {
            break;
        }
    }
}

pub fn traverseJsonSchema(schema_json: string, out: []SchemaVisitEvent): usize {
    const root = jstvScanValue(schema_json, 0);
    if (!root.ok || root.start >= root.end || schema_json[root.start] != '{') {
        return 0;
    }
    let st = JstvTraverseState{
        src: schema_json,
        out: out,
        written: 0,
        total: 0,
        target_ptr: "",
        find_mode: false,
        found: false,
        found_start: 0,
        found_end: 0,
    };
    let ptr_buf: [128]u8 = undefined;
    jstvWalkSchema(*st, root.start, root.end, ptr_buf, 0, .Root, 0);
    return st.written;
}

pub fn countSchemaNodes(schema_json: string): usize {
    const root = jstvScanValue(schema_json, 0);
    if (!root.ok || root.start >= root.end || schema_json[root.start] != '{') {
        return 0;
    }
    let empty_buf: [0]SchemaVisitEvent = undefined;
    let st = JstvTraverseState{
        src: schema_json,
        out: empty_buf[0..0],
        written: 0,
        total: 0,
        target_ptr: "",
        find_mode: false,
        found: false,
        found_start: 0,
        found_end: 0,
    };
    let ptr_buf: [128]u8 = undefined;
    jstvWalkSchema(*st, root.start, root.end, ptr_buf, 0, .Root, 0);
    return st.total;
}

pub fn findSubschemaByPointer(schema_json: string, pointer: string): string? {
    const root = jstvScanValue(schema_json, 0);
    if (!root.ok || root.start >= root.end || schema_json[root.start] != '{') {
        return null;
    }
    let empty_buf: [0]SchemaVisitEvent = undefined;
    let st = JstvTraverseState{
        src: schema_json,
        out: empty_buf[0..0],
        written: 0,
        total: 0,
        target_ptr: pointer,
        find_mode: true,
        found: false,
        found_start: 0,
        found_end: 0,
    };
    let ptr_buf: [128]u8 = undefined;
    jstvWalkSchema(*st, root.start, root.end, ptr_buf, 0, .Root, 0);
    if (st.found && st.found_start <= st.found_end && st.found_end <= schema_json.len) {
        return schema_json[st.found_start..st.found_end];
    }
    return null;
}
