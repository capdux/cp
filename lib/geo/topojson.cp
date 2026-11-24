// Origin: Pure-CP Stdlib (Zero-Alloc TopoJSON v1.0 Quantized Arc & Topology Parser) | Created: 2026-11-22
// Pure CP zero-heap-allocation TopoJSON v1.0 Topology header, transform dequantizer,
// delta-encoded arc point decoder, signed arc reference resolver, and objects iterator.

const std = @import("std");

const MAX_JSON_DEPTH: usize = 128;

/// Affine quantization transform (`scale` & `translate`) from TopoJSON v1.0.
pub struct TopoTransform {
    scale_x: f64 = 1.0,
    scale_y: f64 = 1.0,
    translate_x: f64 = 0.0,
    translate_y: f64 = 0.0,
    present: bool = false,

    /// Applies the affine transform if `present` is true; otherwise returns raw coordinates.
    fn apply(raw_x: f64, raw_y: f64): TopoPoint {
        if (_.present) {
            return TopoPoint {
                x: raw_x * _.scale_x + _.translate_x,
                y: raw_y * _.scale_y + _.translate_y,
            };
        }
        return TopoPoint { x: raw_x, y: raw_y };
    }
}

/// Decoded 2D coordinate point `(x, y)` (typically longitude, latitude).
pub struct TopoPoint {
    x: f64 = 0.0,
    y: f64 = 0.0,
}

/// Optional 2D bounding box `[min_x, min_y, max_x, max_y]`.
pub struct TopoBBox {
    min_x: f64 = 0.0,
    min_y: f64 = 0.0,
    max_x: f64 = 0.0,
    max_y: f64 = 0.0,
    present: bool = false,
}

/// Resolved TopoJSON arc reference: `index` is the 0-based arc index; `reversed` indicates traversal in reverse.
pub struct TopoArcRef {
    index: usize = 0,
    reversed: bool = false,
}

/// Zero-copy parsed TopoJSON v1.0 Topology header.
pub struct TopoHeader {
    transform: TopoTransform = TopoTransform {},
    bbox: TopoBBox = TopoBBox {},
    arcs_slice: []const u8 = "",
    objects_slice: []const u8 = "",
    arc_count: usize = 0,

    fn hasTransform(): bool {
        return _.transform.present;
    }

    fn hasBBox(): bool {
        return _.bbox.present;
    }

    fn arcs(): TopoArcIter {
        return TopoArcIter.init(_.arcs_slice);
    }

    fn objects(): TopoObjectIter {
        return TopoObjectIter.init(_.objects_slice);
    }
}

/// Zero-copy entry yielded by `TopoObjectIter` for each named geometry inside `"objects"`.
pub struct TopoObjectEntry {
    name: []const u8 = "",
    geom_type: []const u8 = "",
    raw_json: []const u8 = "",
}

struct ParsedF64 {
    value: f64,
    next: usize,
}

// ---------------------------------------------------------------------------
// Internal Zero-Alloc JSON Scanner Primitives
// ---------------------------------------------------------------------------

fn isWs(b: u8): bool {
    return b == ' ' || b == '\t' || b == '\r' || b == '\n';
}

fn isDigit(b: u8): bool {
    return b >= '0' && b <= '9';
}

fn skipWs(data: []const u8, start: usize): usize {
    let i = start;
    while (i < data.len && isWs(data[i])) {
        i += 1;
    }
    return i;
}

/// Scans a JSON string starting at `data[start] == '"'`. Returns the index immediately after the closing `'"'`.
fn scanString(data: []const u8, start: usize): usize? {
    if (start >= data.len || data[start] != '"') {
        return null;
    }
    let i = start + 1;
    while (i < data.len) {
        const b = data[i];
        if (b == '"') {
            return i + 1;
        }
        if (b == '\\') {
            i += 2;
            if (i > data.len) {
                return null;
            }
            continue;
        }
        if (b < 0x20) {
            return null;
        }
        i += 1;
    }
    return null;
}

/// Returns the inner byte slice of a scanned JSON string token `[start..end]`.
fn stringContent(data: []const u8, start: usize, end: usize): []const u8 {
    if (end < start + 2) {
        return "";
    }
    return data[start + 1 .. end - 1];
}

/// Scans an RFC 8259 JSON number starting at `start`. Returns the index after the last number byte.
fn scanNumber(data: []const u8, start: usize): usize? {
    let i = start;
    if (i >= data.len) {
        return null;
    }
    if (data[i] == '-') {
        i += 1;
        if (i >= data.len) {
            return null;
        }
    }
    if (data[i] == '0') {
        i += 1;
    } else if (data[i] >= '1' && data[i] <= '9') {
        i += 1;
        while (i < data.len && isDigit(data[i])) {
            i += 1;
        }
    } else {
        return null;
    }
    if (i < data.len && data[i] == '.') {
        i += 1;
        if (i >= data.len || !isDigit(data[i])) {
            return null;
        }
        while (i < data.len && isDigit(data[i])) {
            i += 1;
        }
    }
    if (i < data.len && (data[i] == 'e' || data[i] == 'E')) {
        i += 1;
        if (i < data.len && (data[i] == '+' || data[i] == '-')) {
            i += 1;
        }
        if (i >= data.len || !isDigit(data[i])) {
            return null;
        }
        while (i < data.len && isDigit(data[i])) {
            i += 1;
        }
    }
    return i;
}

/// Parses an RFC 8259 JSON number at `start` into `f64` with zero heap allocation.
fn parseF64At(data: []const u8, start: usize): ParsedF64? {
    const p = skipWs(data, start);
    const end = scanNumber(data, p) ?? return null;
    const slice = data[p..end];
    const val = std.fmt.parseFloat(f64, slice) catch return null;
    if (!std.math.isFinite(val)) {
        return null;
    }
    return ParsedF64 { value: val, next: end };
}

fn matchLiteral(data: []const u8, start: usize, lit: []const u8): usize? {
    if (start + lit.len > data.len) {
        return null;
    }
    if (!std.mem.eql(u8, data[start .. start + lit.len], lit)) {
        return null;
    }
    return start + lit.len;
}

/// Iterative zero-allocation JSON value scanner with explicit container stack (up to MAX_JSON_DEPTH).
/// Returns the byte index immediately following the complete JSON value, or `null` if malformed/truncated.
fn scanValue(data: []const u8, start: usize): usize? {
    let i = skipWs(data, start);
    if (i >= data.len) {
        return null;
    }

    // Fast path for primitive values
    const first = data[i];
    if (first == '"') {
        return scanString(data, i);
    }
    if (first == '-' || isDigit(first)) {
        return scanNumber(data, i);
    }
    if (first == 't') {
        return matchLiteral(data, i, "true");
    }
    if (first == 'f') {
        return matchLiteral(data, i, "false");
    }
    if (first == 'n') {
        return matchLiteral(data, i, "null");
    }
    if (first != '{' && first != '[') {
        return null;
    }

    // Explicit stack: 1 = object '{', 2 = array '['
    // State per frame: 0 = expect first item or close, 1 = expect next item (after comma), 2 = expect comma or close
    let kind_stack: [MAX_JSON_DEPTH]u8 = [0; MAX_JSON_DEPTH];
    let state_stack: [MAX_JSON_DEPTH]u8 = [0; MAX_JSON_DEPTH];
    let depth: usize = 0;

    kind_stack[0] = if (first == '{') 1 else 2;
    state_stack[0] = 0;
    depth = 1;
    i += 1;

    while (depth > 0) {
        i = skipWs(data, i);
        if (i >= data.len) {
            return null;
        }
        const top = depth - 1;
        const kind = kind_stack[top];
        const st = state_stack[top];
        const b = data[i];

        if (st == 2) {
            if (b == ',') {
                state_stack[top] = 1;
                i += 1;
                continue;
            }
            if ((kind == 1 && b == '}') || (kind == 2 && b == ']')) {
                depth -= 1;
                i += 1;
                continue;
            }
            return null;
        }

        if (st == 0) {
            if ((kind == 1 && b == '}') || (kind == 2 && b == ']')) {
                depth -= 1;
                i += 1;
                continue;
            }
        }

        // Expecting an entry (object key:value or array element)
        if (kind == 1) {
            if (b != '"') {
                return null;
            }
            const key_end = scanString(data, i) ?? return null;
            i = skipWs(data, key_end);
            if (i >= data.len || data[i] != ':') {
                return null;
            }
            i = skipWs(data, i + 1);
            if (i >= data.len) {
                return null;
            }
        }

        // Now at the start of a value
        const vb = data[i];
        state_stack[top] = 2;

        if (vb == '{' || vb == '[') {
            if (depth >= MAX_JSON_DEPTH) {
                return null;
            }
            kind_stack[depth] = if (vb == '{') 1 else 2;
            state_stack[depth] = 0;
            depth += 1;
            i += 1;
        } else if (vb == '"') {
            i = scanString(data, i) ?? return null;
        } else if (vb == '-' || isDigit(vb)) {
            i = scanNumber(data, i) ?? return null;
        } else if (vb == 't') {
            i = matchLiteral(data, i, "true") ?? return null;
        } else if (vb == 'f') {
            i = matchLiteral(data, i, "false") ?? return null;
        } else if (vb == 'n') {
            i = matchLiteral(data, i, "null") ?? return null;
        } else {
            return null;
        }
    }

    return i;
}

/// Parses a JSON 2-element numeric array `[a, b]` into a `TopoPoint`.
fn parseNumPair(data: []const u8, start: usize): TopoPoint? {
    let i = skipWs(data, start);
    if (i >= data.len || data[i] != '[') {
        return null;
    }
    const p1 = parseF64At(data, i + 1) ?? return null;
    i = skipWs(data, p1.next);
    if (i >= data.len || data[i] != ',') {
        return null;
    }
    const p2 = parseF64At(data, i + 1) ?? return null;
    i = skipWs(data, p2.next);
    if (i >= data.len || data[i] != ']') {
        return null;
    }
    return TopoPoint { x: p1.value, y: p2.value };
}

/// Parses a TopoJSON `"transform": { "scale": [sx, sy], "translate": [tx, ty] }` object.
fn parseTransformObj(slice: []const u8): TopoTransform? {
    let i = skipWs(slice, 0);
    if (i >= slice.len || slice[i] != '{') {
        return null;
    }
    i = skipWs(slice, i + 1);

    let has_scale = false;
    let has_translate = false;
    let scale_x: f64 = 1.0;
    let scale_y: f64 = 1.0;
    let translate_x: f64 = 0.0;
    let translate_y: f64 = 0.0;

    if (i < slice.len && slice[i] == '}') {
        return null;
    }

    while (i < slice.len) {
        const key_end = scanString(slice, i) ?? return null;
        const key = stringContent(slice, i, key_end);
        i = skipWs(slice, key_end);
        if (i >= slice.len || slice[i] != ':') {
            return null;
        }
        const val_start = skipWs(slice, i + 1);
        const val_end = scanValue(slice, val_start) ?? return null;
        const val_slice = slice[val_start..val_end];

        if (std.mem.eql(u8, key, "scale")) {
            const pair = parseNumPair(val_slice, 0) ?? return null;
            scale_x = pair.x;
            scale_y = pair.y;
            has_scale = true;
        } else if (std.mem.eql(u8, key, "translate")) {
            const pair = parseNumPair(val_slice, 0) ?? return null;
            translate_x = pair.x;
            translate_y = pair.y;
            has_translate = true;
        }

        i = skipWs(slice, val_end);
        if (i >= slice.len) {
            return null;
        }
        if (slice[i] == ',') {
            i = skipWs(slice, i + 1);
            continue;
        }
        if (slice[i] == '}') {
            break;
        }
        return null;
    }

    if (!has_scale || !has_translate) {
        return null;
    }
    return TopoTransform {
        scale_x: scale_x,
        scale_y: scale_y,
        translate_x: translate_x,
        translate_y: translate_y,
        present: true,
    };
}

/// Parses a TopoJSON `"bbox": [min_x, min_y, max_x, max_y]` (or 6-element 3D bbox) array.
fn parseBBoxArray(slice: []const u8): TopoBBox? {
    let i = skipWs(slice, 0);
    if (i >= slice.len || slice[i] != '[') {
        return null;
    }
    let vals: [6]f64 = [0.0; 6];
    let count: usize = 0;
    i = skipWs(slice, i + 1);
    if (i < slice.len && slice[i] == ']') {
        return null;
    }
    while (i < slice.len) {
        const num = parseF64At(slice, i) ?? return null;
        if (count < 6) {
            vals[count] = num.value;
        }
        count += 1;
        i = skipWs(slice, num.next);
        if (i >= slice.len) {
            return null;
        }
        if (slice[i] == ',') {
            i = skipWs(slice, i + 1);
            continue;
        }
        if (slice[i] == ']') {
            break;
        }
        return null;
    }
    if (count == 4) {
        return TopoBBox {
            min_x: vals[0],
            min_y: vals[1],
            max_x: vals[2],
            max_y: vals[3],
            present: true,
        };
    }
    if (count == 6) {
        return TopoBBox {
            min_x: vals[0],
            min_y: vals[1],
            max_x: vals[3],
            max_y: vals[4],
            present: true,
        };
    }
    return null;
}

/// Counts top-level elements inside a JSON `"arcs": [...]` array without allocating.
fn countTopLevelArrayElements(slice: []const u8): usize? {
    let i = skipWs(slice, 0);
    if (i >= slice.len || slice[i] != '[') {
        return null;
    }
    i = skipWs(slice, i + 1);
    if (i >= slice.len) {
        return null;
    }
    if (slice[i] == ']') {
        return 0;
    }
    let count: usize = 0;
    while (i < slice.len) {
        const elem_end = scanValue(slice, i) ?? return null;
        count += 1;
        i = skipWs(slice, elem_end);
        if (i >= slice.len) {
            return null;
        }
        if (slice[i] == ',') {
            i = skipWs(slice, i + 1);
            continue;
        }
        if (slice[i] == ']') {
            return count;
        }
        return null;
    }
    return null;
}

/// Extracts the `"type"` string field from a JSON object slice (returns `""` if absent or non-string).
fn extractObjectType(obj_slice: []const u8): []const u8 {
    let i = skipWs(obj_slice, 0);
    if (i >= obj_slice.len || obj_slice[i] != '{') {
        return "";
    }
    i = skipWs(obj_slice, i + 1);
    if (i >= obj_slice.len || obj_slice[i] == '}') {
        return "";
    }
    while (i < obj_slice.len) {
        const key_end = scanString(obj_slice, i) ?? return "";
        const key = stringContent(obj_slice, i, key_end);
        i = skipWs(obj_slice, key_end);
        if (i >= obj_slice.len || obj_slice[i] != ':') {
            return "";
        }
        const val_start = skipWs(obj_slice, i + 1);
        const val_end = scanValue(obj_slice, val_start) ?? return "";
        if (std.mem.eql(u8, key, "type")) {
            if (val_start < val_end && obj_slice[val_start] == '"') {
                return stringContent(obj_slice, val_start, val_end);
            }
            return "";
        }
        i = skipWs(obj_slice, val_end);
        if (i >= obj_slice.len || obj_slice[i] != ',') {
            return "";
        }
        i = skipWs(obj_slice, i + 1);
    }
    return "";
}

// ---------------------------------------------------------------------------
// Public API: Detection & Header Parsing
// ---------------------------------------------------------------------------

/// Returns `true` if `data` is a well-formed top-level JSON object with `"type": "Topology"`.
pub fn isTopoJson(data: []const u8): bool {
    let i = skipWs(data, 0);
    if (i >= data.len || data[i] != '{') {
        return false;
    }
    i = skipWs(data, i + 1);
    if (i >= data.len || data[i] == '}') {
        return false;
    }

    let found_topology = false;
    while (i < data.len) {
        const key_end = scanString(data, i) ?? return false;
        const key = stringContent(data, i, key_end);
        i = skipWs(data, key_end);
        if (i >= data.len || data[i] != ':') {
            return false;
        }
        const val_start = skipWs(data, i + 1);
        const val_end = scanValue(data, val_start) ?? return false;

        if (std.mem.eql(u8, key, "type")) {
            if (val_start < val_end && data[val_start] == '"') {
                const val = stringContent(data, val_start, val_end);
                if (std.mem.eql(u8, val, "Topology")) {
                    found_topology = true;
                } else {
                    return false;
                }
            } else {
                return false;
            }
        }

        i = skipWs(data, val_end);
        if (i >= data.len) {
            return false;
        }
        if (data[i] == ',') {
            i = skipWs(data, i + 1);
            continue;
        }
        if (data[i] == '}') {
            const tail = skipWs(data, i + 1);
            return found_topology && tail == data.len;
        }
        return false;
    }
    return false;
}

/// Zero-allocation parser for a TopoJSON v1.0 Topology document header.
/// Extracts optional `transform` and `bbox`, raw `"arcs"` and `"objects"` slices, and `arc_count`.
pub fn parseTopoHeader(data: []const u8): TopoHeader? {
    let i = skipWs(data, 0);
    if (i >= data.len || data[i] != '{') {
        return null;
    }
    i = skipWs(data, i + 1);
    if (i >= data.len || data[i] == '}') {
        return null;
    }

    let header = TopoHeader {};
    let has_type = false;

    while (i < data.len) {
        const key_end = scanString(data, i) ?? return null;
        const key = stringContent(data, i, key_end);
        i = skipWs(data, key_end);
        if (i >= data.len || data[i] != ':') {
            return null;
        }
        const val_start = skipWs(data, i + 1);
        const val_end = scanValue(data, val_start) ?? return null;
        const val_slice = data[val_start..val_end];

        if (std.mem.eql(u8, key, "type")) {
            if (val_start >= val_end || data[val_start] != '"') {
                return null;
            }
            const t_str = stringContent(data, val_start, val_end);
            if (!std.mem.eql(u8, t_str, "Topology")) {
                return null;
            }
            has_type = true;
        } else if (std.mem.eql(u8, key, "transform")) {
            header.transform = parseTransformObj(val_slice) ?? return null;
        } else if (std.mem.eql(u8, key, "bbox")) {
            header.bbox = parseBBoxArray(val_slice) ?? return null;
        } else if (std.mem.eql(u8, key, "arcs")) {
            if (val_slice.len == 0 || val_slice[0] != '[') {
                return null;
            }
            header.arcs_slice = val_slice;
            header.arc_count = countTopLevelArrayElements(val_slice) ?? return null;
        } else if (std.mem.eql(u8, key, "objects")) {
            if (val_slice.len == 0 || val_slice[0] != '{') {
                return null;
            }
            header.objects_slice = val_slice;
        }

        i = skipWs(data, val_end);
        if (i >= data.len) {
            return null;
        }
        if (data[i] == ',') {
            i = skipWs(data, i + 1);
            continue;
        }
        if (data[i] == '}') {
            const tail = skipWs(data, i + 1);
            if (!has_type || tail != data.len) {
                return null;
            }
            return header;
        }
        return null;
    }

    return null;
}

// ---------------------------------------------------------------------------
// Public API: Signed Arc Reference Resolution
// ---------------------------------------------------------------------------

/// Decodes a signed TopoJSON arc index:
/// - `index >= 0`: forward arc at `index` (`reversed = false`)
/// - `index < 0`: reversed arc at bitwise complement `~index` = `-(index + 1)` (`reversed = true`)
pub fn resolveArcRef(index: i64): TopoArcRef {
    if (index >= 0) {
        return TopoArcRef {
            index: index as usize,
            reversed: false,
        };
    }
    const rev_idx: i64 = -(index + 1);
    return TopoArcRef {
        index: rev_idx as usize,
        reversed: true,
    };
}

// ---------------------------------------------------------------------------
// Public API: Zero-Alloc Arc & Point Iterators
// ---------------------------------------------------------------------------

/// Zero-copy iterator over each arc slice `[[x0, y0], ...]` in a TopoJSON `"arcs"` array.
pub struct TopoArcIter {
    src: []const u8 = "",
    pos: usize = 0,
    done: bool = true,

    static fn init(arcs_slice: []const u8): TopoArcIter {
        const start = skipWs(arcs_slice, 0);
        if (start >= arcs_slice.len || arcs_slice[start] != '[') {
            return TopoArcIter { src: arcs_slice, pos: arcs_slice.len, done: true };
        }
        return TopoArcIter { src: arcs_slice, pos: start + 1, done: false };
    }

    /// Returns the raw JSON slice for the next arc, or `null` at end / malformed input.
    fn next(*_): ([]const u8)? {
        if (_.done) {
            return null;
        }
        const p = skipWs(_.src, _.pos);
        if (p >= _.src.len || _.src[p] == ']') {
            _.done = true;
            return null;
        }
        if (_.src[p] != '[') {
            _.done = true;
            return null;
        }
        const end_opt = scanValue(_.src, p);
        if (end_opt == null) {
            _.done = true;
            return null;
        }
        const end = end_opt.?;
        const after = skipWs(_.src, end);
        if (after >= _.src.len) {
            _.done = true;
            return null;
        }
        if (_.src[after] == ',') {
            _.pos = after + 1;
        } else if (_.src[after] == ']') {
            _.done = true;
        } else {
            _.done = true;
            return null;
        }
        return _.src[p..end];
    }
}

/// Returns the raw JSON slice for the `idx`-th arc in `header`, or `null` if out of bounds.
pub fn arcSliceAt(header: TopoHeader, idx: usize): ([]const u8)? {
    if (idx >= header.arc_count) {
        return null;
    }
    let it = TopoArcIter.init(header.arcs_slice);
    let cur: usize = 0;
    while (slice := it.next()) {
        if (cur == idx) {
            return slice;
        }
        cur += 1;
    }
    return null;
}

/// Zero-allocation point decoder for a single TopoJSON arc slice `[[dx0, dy0], [dx1, dy1], ...]`.
/// When `transform.present` is true, accumulates delta coordinates and applies affine dequantization:
///   `x = acc_x * scale_x + translate_x`, `y = acc_y * scale_y + translate_y`.
/// When `transform.present` is false, yields raw unquantized coordinates directly.
pub struct TopoArcPointIter {
    src: []const u8 = "",
    pos: usize = 0,
    done: bool = true,
    transform: TopoTransform = TopoTransform {},
    acc_x: f64 = 0.0,
    acc_y: f64 = 0.0,

    static fn init(arc_slice: []const u8, transform: TopoTransform): TopoArcPointIter {
        const start = skipWs(arc_slice, 0);
        if (start >= arc_slice.len || arc_slice[start] != '[') {
            return TopoArcPointIter {
                src: arc_slice,
                pos: arc_slice.len,
                done: true,
                transform: transform,
                acc_x: 0.0,
                acc_y: 0.0,
            };
        }
        return TopoArcPointIter {
            src: arc_slice,
            pos: start + 1,
            done: false,
            transform: transform,
            acc_x: 0.0,
            acc_y: 0.0,
        };
    }

    /// Decodes and returns the next `TopoPoint`, or `null` when exhausted or malformed.
    fn next(*_): TopoPoint? {
        if (_.done) {
            return null;
        }
        let i = skipWs(_.src, _.pos);
        if (i >= _.src.len || _.src[i] == ']') {
            _.done = true;
            return null;
        }
        if (_.src[i] != '[') {
            _.done = true;
            return null;
        }

        const px_opt = parseF64At(_.src, i + 1);
        if (px_opt == null) {
            _.done = true;
            return null;
        }
        const px = px_opt.?;
        i = skipWs(_.src, px.next);
        if (i >= _.src.len || _.src[i] != ',') {
            _.done = true;
            return null;
        }
        const py_opt = parseF64At(_.src, i + 1);
        if (py_opt == null) {
            _.done = true;
            return null;
        }
        const py = py_opt.?;
        i = skipWs(_.src, py.next);

        // Skip any optional extra dimensions (e.g., altitude z) in the coordinate tuple
        while (i < _.src.len && _.src[i] == ',') {
            const extra_opt = parseF64At(_.src, i + 1);
            if (extra_opt == null) {
                _.done = true;
                return null;
            }
            const extra = extra_opt.?;
            i = skipWs(_.src, extra.next);
        }

        if (i >= _.src.len || _.src[i] != ']') {
            _.done = true;
            return null;
        }

        const after = skipWs(_.src, i + 1);
        if (after >= _.src.len) {
            _.done = true;
            return null;
        }
        if (_.src[after] == ',') {
            _.pos = after + 1;
        } else if (_.src[after] == ']') {
            _.done = true;
        } else {
            _.done = true;
            return null;
        }

        if (_.transform.present) {
            _.acc_x += px.value;
            _.acc_y += py.value;
            return TopoPoint {
                x: _.acc_x * _.transform.scale_x + _.transform.translate_x,
                y: _.acc_y * _.transform.scale_y + _.transform.translate_y,
            };
        }
        return TopoPoint { x: px.value, y: py.value };
    }
}

/// Counts the number of valid coordinate points in a single arc slice without allocating.
pub fn countArcPoints(arc_slice: []const u8): usize {
    let it = TopoArcPointIter.init(arc_slice, TopoTransform {});
    let count: usize = 0;
    while (pt := it.next()) {
        * = pt;
        count += 1;
    }
    return count;
}

// ---------------------------------------------------------------------------
// Public API: Zero-Copy Objects Iterator
// ---------------------------------------------------------------------------

/// Zero-copy iterator over named geometry objects inside a TopoJSON `"objects": { ... }` map.
pub struct TopoObjectIter {
    src: []const u8 = "",
    pos: usize = 0,
    done: bool = true,

    static fn init(objects_slice: []const u8): TopoObjectIter {
        const start = skipWs(objects_slice, 0);
        if (start >= objects_slice.len || objects_slice[start] != '{') {
            return TopoObjectIter { src: objects_slice, pos: objects_slice.len, done: true };
        }
        return TopoObjectIter { src: objects_slice, pos: start + 1, done: false };
    }

    /// Returns the next `TopoObjectEntry`, or `null` when exhausted or malformed.
    fn next(*_): TopoObjectEntry? {
        if (_.done) {
            return null;
        }
        let i = skipWs(_.src, _.pos);
        if (i >= _.src.len || _.src[i] == '}') {
            _.done = true;
            return null;
        }
        const key_end_opt = scanString(_.src, i);
        if (key_end_opt == null) {
            _.done = true;
            return null;
        }
        const key_end = key_end_opt.?;
        const name = stringContent(_.src, i, key_end);
        i = skipWs(_.src, key_end);
        if (i >= _.src.len || _.src[i] != ':') {
            _.done = true;
            return null;
        }
        const val_start = skipWs(_.src, i + 1);
        const val_end_opt = scanValue(_.src, val_start);
        if (val_end_opt == null) {
            _.done = true;
            return null;
        }
        const val_end = val_end_opt.?;
        const raw_json = _.src[val_start..val_end];
        const geom_type = extractObjectType(raw_json);

        const after = skipWs(_.src, val_end);
        if (after >= _.src.len) {
            _.done = true;
            return null;
        }
        if (_.src[after] == ',') {
            _.pos = after + 1;
        } else if (_.src[after] == '}') {
            _.done = true;
        } else {
            _.done = true;
            return null;
        }

        return TopoObjectEntry {
            name: name,
            geom_type: geom_type,
            raw_json: raw_json,
        };
    }
}
