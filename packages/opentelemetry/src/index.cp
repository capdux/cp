// @cap/traceparent — Zero-heap W3C Trace Context (traceparent & tracestate)
// Clean-room MIT implementation in pure CP (strict W3C Trace Context Level 1 compliance).

pub const TRACEPARENT_LEN: usize = 55;
pub const FLAG_SAMPLED: u8 = 0x01;
pub const FLAG_RANDOM: u8 = 0x02;

const HEX_LOWER: []const u8 = "0123456789abcdef";

pub struct TraceParent {
    version: u8,
    trace_id: [16]u8,
    parent_id: [8]u8,
    trace_flags: u8,
    valid: bool,

    pub fn isSampled(): bool {
        return (_.trace_flags & FLAG_SAMPLED) != 0;
    }

    pub fn format(dst: []u8): i64 {
        if (!_.valid || dst.len < TRACEPARENT_LEN) {
            return -1;
        }
        dst[0] = HEX_LOWER[(_.version >> 4) as usize];
        dst[1] = HEX_LOWER[(_.version & 0x0F) as usize];
        dst[2] = '-';
        let i: usize = 0;
        while (i < 16) {
            let b: u8 = _.trace_id[i];
            dst[3 + i * 2] = HEX_LOWER[(b >> 4) as usize];
            dst[3 + i * 2 + 1] = HEX_LOWER[(b & 0x0F) as usize];
            i += 1;
        }
        dst[35] = '-';
        let j: usize = 0;
        while (j < 8) {
            let b: u8 = _.parent_id[j];
            dst[36 + j * 2] = HEX_LOWER[(b >> 4) as usize];
            dst[36 + j * 2 + 1] = HEX_LOWER[(b & 0x0F) as usize];
            j += 1;
        }
        dst[52] = '-';
        dst[53] = HEX_LOWER[(_.trace_flags >> 4) as usize];
        dst[54] = HEX_LOWER[(_.trace_flags & 0x0F) as usize];
        return TRACEPARENT_LEN as i64;
    }
}

fn hexValLower(c: u8): i32 {
    if (c >= '0' && c <= '9') {
        return (c - '0') as i32;
    }
    if (c >= 'a' && c <= 'f') {
        return (c - 'a' + 10) as i32;
    }
    return -1;
}

fn parseHexByteLower(hi: u8, lo: u8): i32 {
    let h: i32 = hexValLower(hi);
    let l: i32 = hexValLower(lo);
    if (h < 0 || l < 0) {
        return -1;
    }
    return (h << 4) | l;
}

fn invalidTraceParent(): TraceParent {
    let tid: [16]u8 = undefined;
    let pid: [8]u8 = undefined;
    return TraceParent {
        version: 0,
        trace_id: tid,
        parent_id: pid,
        trace_flags: 0,
        valid: false,
    };
}

// Parse W3C Trace Context `traceparent`: "00-<32 lower hex>-<16 lower hex>-<2 lower hex>"
pub fn parseTraceParent(header: []const u8): TraceParent {
    if (header.len < TRACEPARENT_LEN) {
        return invalidTraceParent();
    }
    if (header[2] != '-' || header[35] != '-' || header[52] != '-') {
        return invalidTraceParent();
    }
    let ver_i: i32 = parseHexByteLower(header[0], header[1]);
    if (ver_i < 0 || ver_i == 0xFF) {
        return invalidTraceParent();
    }
    if (ver_i == 0 && header.len != TRACEPARENT_LEN) {
        return invalidTraceParent();
    }
    if (ver_i > 0 && header.len > TRACEPARENT_LEN && header[TRACEPARENT_LEN] != '-') {
        return invalidTraceParent();
    }

    let tid: [16]u8 = undefined;
    let tid_nonzero: bool = false;
    let i: usize = 0;
    while (i < 16) {
        let bv: i32 = parseHexByteLower(header[3 + i * 2], header[3 + i * 2 + 1]);
        if (bv < 0) {
            return invalidTraceParent();
        }
        tid[i] = bv as u8;
        if (bv != 0) {
            tid_nonzero = true;
        }
        i += 1;
    }
    if (!tid_nonzero) {
        return invalidTraceParent();
    }

    let pid: [8]u8 = undefined;
    let pid_nonzero: bool = false;
    let j: usize = 0;
    while (j < 8) {
        let bv: i32 = parseHexByteLower(header[36 + j * 2], header[36 + j * 2 + 1]);
        if (bv < 0) {
            return invalidTraceParent();
        }
        pid[j] = bv as u8;
        if (bv != 0) {
            pid_nonzero = true;
        }
        j += 1;
    }
    if (!pid_nonzero) {
        return invalidTraceParent();
    }

    let flags_i: i32 = parseHexByteLower(header[53], header[54]);
    if (flags_i < 0) {
        return invalidTraceParent();
    }

    return TraceParent {
        version: ver_i as u8,
        trace_id: tid,
        parent_id: pid,
        trace_flags: flags_i as u8,
        valid: true,
    };
}

// Create a child TraceParent preserving version, trace_id, and trace_flags with a new 8-byte span ID
pub fn makeChild(parent: TraceParent, new_span_id: [8]u8): TraceParent {
    if (!parent.valid) {
        return invalidTraceParent();
    }
    let nonzero: bool = false;
    let i: usize = 0;
    while (i < 8) {
        if (new_span_id[i] != 0) {
            nonzero = true;
        }
        i += 1;
    }
    if (!nonzero) {
        return invalidTraceParent();
    }
    return TraceParent {
        version: parent.version,
        trace_id: parent.trace_id,
        parent_id: new_span_id,
        trace_flags: parent.trace_flags,
        valid: true,
    };
}

// Zero-copy lookup of vendor key in W3C `tracestate` header (e.g. "rojo=00f067aa0ba902b7,congo=t61rcWkgMzE")
pub fn getTraceState(header: []const u8, vendor_key: []const u8): []const u8 {
    if (header.len == 0 || vendor_key.len == 0) {
        return "";
    }
    let pos: usize = 0;
    while (pos < header.len) {
        let end: usize = pos;
        while (end < header.len && header[end] != ',') {
            end += 1;
        }
        let s: usize = pos;
        let e: usize = end;
        while (s < e && (header[s] == ' ' || header[s] == '\t')) { s += 1; }
        while (e > s && (header[e - 1] == ' ' || header[e - 1] == '\t')) { e -= 1; }
        let eq: usize = s;
        while (eq < e && header[eq] != '=') { eq += 1; }
        if (eq < e && (eq - s) == vendor_key.len) {
            let is_match: bool = true;
            let k: usize = 0;
            while (k < vendor_key.len) {
                if (header[s + k] != vendor_key[k]) {
                    is_match = false;
                    break;
                }
                k += 1;
            }
            if (is_match) {
                return header[eq + 1 .. e];
            }
        }
        pos = end + 1;
    }
    return "";
}
