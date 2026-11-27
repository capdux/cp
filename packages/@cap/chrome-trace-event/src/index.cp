// @cap/chrome-trace-event — Zero-heap Chrome DevTools / Perfetto Trace Event JSON serializer

pub enum TracePhase {
    Begin,
    End,
    Complete,
    Instant,
    Counter,
    Metadata,
}

pub struct TraceEvent {
    name: string,
    cat: string,
    ph: TracePhase,
    ts_us: u64,
    dur_us: u64,
    pid: u32,
    tid: u32,
}

pub fn phaseCode(ph: TracePhase): u8 {
    if (ph == .Begin) {
        return 'B';
    }
    if (ph == .End) {
        return 'E';
    }
    if (ph == .Complete) {
        return 'X';
    }
    if (ph == .Instant) {
        return 'i';
    }
    if (ph == .Counter) {
        return 'C';
    }
    return 'M';
}

fn ctevHexDigit(n: u8): u8 {
    if (n < 10) {
        return '0' + n;
    }
    return 'a' + (n - 10);
}

fn ctevWriteByte(out: []u8, pos: i64, b: u8): i64 {
    if (pos < 0) {
        return -1;
    }
    const idx: usize = usize(pos);
    if (idx >= out.len) {
        return -1;
    }
    out[idx] = b;
    return pos + 1;
}

fn ctevWriteStr(out: []u8, pos: i64, s: string): i64 {
    if (pos < 0) {
        return -1;
    }
    const idx: usize = usize(pos);
    if (idx + s.len > out.len) {
        return -1;
    }
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        out[idx + i] = s[i];
    }
    return pos + i64(s.len);
}

fn ctevWriteJsonStr(out: []u8, pos: i64, s: string): i64 {
    let p: i64 = ctevWriteByte(out, pos, '"');
    let i: usize = 0;
    while (i < s.len && p >= 0) : (i += 1) {
        const b: u8 = s[i];
        if (b == '"') {
            p = ctevWriteStr(out, p, "\\\"");
        } else if (b == '\\') {
            p = ctevWriteStr(out, p, "\\\\");
        } else if (b == '\n') {
            p = ctevWriteStr(out, p, "\\n");
        } else if (b == '\r') {
            p = ctevWriteStr(out, p, "\\r");
        } else if (b == '\t') {
            p = ctevWriteStr(out, p, "\\t");
        } else if (b == 0x08) {
            p = ctevWriteStr(out, p, "\\b");
        } else if (b == 0x0C) {
            p = ctevWriteStr(out, p, "\\f");
        } else if (b < 0x20) {
            p = ctevWriteStr(out, p, "\\u00");
            p = ctevWriteByte(out, p, ctevHexDigit((b >> 4) & 0x0F));
            p = ctevWriteByte(out, p, ctevHexDigit(b & 0x0F));
        } else {
            p = ctevWriteByte(out, p, b);
        }
    }
    return ctevWriteByte(out, p, '"');
}

fn ctevWriteU64(out: []u8, pos: i64, val: u64): i64 {
    if (pos < 0) {
        return -1;
    }
    if (val == 0) {
        return ctevWriteByte(out, pos, '0');
    }
    let digits: [20]u8 = undefined;
    let count: usize = 0;
    let v: u64 = val;
    while (v > 0) {
        digits[count] = '0' + u8(v % 10);
        v = v / 10;
        count += 1;
    }
    let p: i64 = pos;
    let i: usize = count;
    while (i > 0 && p >= 0) {
        i -= 1;
        p = ctevWriteByte(out, p, digits[i]);
    }
    return p;
}

pub fn formatTraceEvent(ev: TraceEvent, out: []u8): usize {
    let p: i64 = 0;
    p = ctevWriteStr(out, p, "{\"name\":");
    p = ctevWriteJsonStr(out, p, ev.name);
    p = ctevWriteStr(out, p, ",\"cat\":");
    p = ctevWriteJsonStr(out, p, ev.cat);
    p = ctevWriteStr(out, p, ",\"ph\":\"");
    p = ctevWriteByte(out, p, phaseCode(ev.ph));
    p = ctevWriteStr(out, p, "\",\"ts\":");
    p = ctevWriteU64(out, p, ev.ts_us);
    if (ev.ph == .Complete) {
        p = ctevWriteStr(out, p, ",\"dur\":");
        p = ctevWriteU64(out, p, ev.dur_us);
    }
    p = ctevWriteStr(out, p, ",\"pid\":");
    p = ctevWriteU64(out, p, u64(ev.pid));
    p = ctevWriteStr(out, p, ",\"tid\":");
    p = ctevWriteU64(out, p, u64(ev.tid));
    p = ctevWriteByte(out, p, '}');
    if (p < 0) {
        return 0;
    }
    return usize(p);
}

pub fn formatTraceArray(events: []const TraceEvent, out: []u8): usize {
    let p: i64 = ctevWriteByte(out, 0, '[');
    let i: usize = 0;
    while (i < events.len && p >= 0) : (i += 1) {
        if (i > 0) {
            p = ctevWriteByte(out, p, ',');
            if (p < 0) {
                return 0;
            }
        }
        const off: usize = usize(p);
        const written: usize = formatTraceEvent(events[i], out[off..]);
        if (written == 0) {
            return 0;
        }
        p += i64(written);
    }
    p = ctevWriteByte(out, p, ']');
    if (p < 0) {
        return 0;
    }
    return usize(p);
}
