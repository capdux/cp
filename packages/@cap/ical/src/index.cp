// @cap/ical — Zero-allocation RFC 5545 iCalendar (.ics) Generator & Parser
// Clean-room MIT implementation in pure CP.

pub struct IcalEvent {
    pub uid: []const u8,
    pub dtstart: []const u8,
    pub dtend: []const u8,
    pub summary: []const u8,
    pub location: []const u8,
    pub description: []const u8,
}

pub struct ParsedEvent {
    pub uid_start: usize,
    pub uid_len: usize,
    pub dtstart_start: usize,
    pub dtstart_len: usize,
    pub dtend_start: usize,
    pub dtend_len: usize,
    pub summary_start: usize,
    pub summary_len: usize,
    pub location_start: usize,
    pub location_len: usize,
    pub valid: bool,
}

fn pushSlice(dst: []u8, pos: usize, src: []const u8) -> i64 {
    if (pos + src.len > dst.len) {
        return -1;
    }
    let i: usize = 0;
    while (i < src.len) {
        dst[pos + i] = src[i];
        i += 1;
    }
    return (pos + src.len) as i64;
}

fn pushEscapedText(dst: []u8, pos: usize, src: []const u8) -> i64 {
    let p = pos;
    let i: usize = 0;
    while (i < src.len) {
        const ch = src[i];
        if (ch == 13) {
            // Skip CR in CRLF; LF will become \n
            i += 1;
            continue;
        }
        if (ch == 10) {
            if (p + 2 > dst.len) { return -1; }
            dst[p] = 92;      // '\'
            dst[p + 1] = 110; // 'n'
            p += 2;
        } else if (ch == 92 || ch == 44 || ch == 59) {
            // '\', ',', ';'
            if (p + 2 > dst.len) { return -1; }
            dst[p] = 92;
            dst[p + 1] = ch;
            p += 2;
        } else {
            if (p >= dst.len) { return -1; }
            dst[p] = ch;
            p += 1;
        }
        i += 1;
    }
    return p as i64;
}

fn pushPropRaw(dst: []u8, pos: usize, prop: []const u8, val: []const u8) -> i64 {
    if (val.len == 0) {
        return pos as i64;
    }
    let p = pushSlice(dst, pos, prop);
    if (p < 0) { return -1; }
    p = pushSlice(dst, p as usize, ":");
    if (p < 0) { return -1; }
    p = pushSlice(dst, p as usize, val);
    if (p < 0) { return -1; }
    return pushSlice(dst, p as usize, "\r\n");
}

fn pushPropText(dst: []u8, pos: usize, prop: []const u8, val: []const u8) -> i64 {
    if (val.len == 0) {
        return pos as i64;
    }
    let p = pushSlice(dst, pos, prop);
    if (p < 0) { return -1; }
    p = pushSlice(dst, p as usize, ":");
    if (p < 0) { return -1; }
    p = pushEscapedText(dst, p as usize, val);
    if (p < 0) { return -1; }
    return pushSlice(dst, p as usize, "\r\n");
}

pub struct IcalWriter {
    buf: []u8,
    pos: usize,
    event_count: usize,
    failed: bool,
    finished: bool,

    /// Initializes an RFC 5545 `VCALENDAR` writer into `buf` with the given `prod_id`.
    pub static fn init(buf: []u8, prod_id: []const u8) -> IcalWriter {
        let p = pushSlice(buf, 0, "BEGIN:VCALENDAR\r\nVERSION:2.0\r\nPRODID:");
        if (p >= 0) {
            p = pushSlice(buf, p as usize, prod_id);
        }
        if (p >= 0) {
            p = pushSlice(buf, p as usize, "\r\n");
        }
        if (p < 0) {
            return IcalWriter {
                buf: buf,
                pos: 0,
                event_count: 0,
                failed: true,
                finished: false,
            };
        }
        return IcalWriter {
            buf: buf,
            pos: p as usize,
            event_count: 0,
            failed: false,
            finished: false,
        };
    }

    /// Appends a `VEVENT` block with RFC 5545 text escaping. Returns false on overflow.
    pub fn addEvent(*_, ev: IcalEvent) -> bool {
        if (_.failed || _.finished) {
            return false;
        }
        const saved = _.pos;
        let p = pushSlice(_.buf, _.pos, "BEGIN:VEVENT\r\n");
        if (p >= 0) { p = pushPropRaw(_.buf, p as usize, "UID", ev.uid); }
        if (p >= 0) { p = pushPropRaw(_.buf, p as usize, "DTSTART", ev.dtstart); }
        if (p >= 0) { p = pushPropRaw(_.buf, p as usize, "DTEND", ev.dtend); }
        if (p >= 0) { p = pushPropText(_.buf, p as usize, "SUMMARY", ev.summary); }
        if (p >= 0) { p = pushPropText(_.buf, p as usize, "LOCATION", ev.location); }
        if (p >= 0) { p = pushPropText(_.buf, p as usize, "DESCRIPTION", ev.description); }
        if (p >= 0) { p = pushSlice(_.buf, p as usize, "END:VEVENT\r\n"); }

        if (p < 0) {
            _.pos = saved;
            _.failed = true;
            return false;
        }
        _.pos = p as usize;
        _.event_count += 1;
        return true;
    }

    /// Closes the `VCALENDAR` with `END:VCALENDAR\r\n` and returns total byte length, or `-1` on overflow.
    pub fn finish(*_) -> i64 {
        if (_.failed) {
            return -1;
        }
        if (!_.finished) {
            const p = pushSlice(_.buf, _.pos, "END:VCALENDAR\r\n");
            if (p < 0) {
                _.failed = true;
                return -1;
            }
            _.pos = p as usize;
            _.finished = true;
        }
        return _.pos as i64;
    }

    /// Returns the number of events written so far.
    pub fn eventCount() -> usize {
        return _.event_count;
    }

    /// Returns the generated `.ics` byte slice written so far.
    pub fn written() -> []const u8 {
        return _.buf[0.._.pos];
    }
}

fn toUpperAscii(b: u8) -> u8 {
    if (b >= 97 && b <= 122) {
        return b - 32;
    }
    return b;
}

fn eqIgnoreCaseUpper(src: []const u8, start: usize, end: usize, upper_target: []const u8) -> bool {
    if (end - start != upper_target.len) {
        return false;
    }
    let i: usize = 0;
    while (i < upper_target.len) {
        if (toUpperAscii(src[start + i]) != upper_target[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn emptyParsedEvent() -> ParsedEvent {
    return ParsedEvent {
        uid_start: 0,
        uid_len: 0,
        dtstart_start: 0,
        dtstart_len: 0,
        dtend_start: 0,
        dtend_len: 0,
        summary_start: 0,
        summary_len: 0,
        location_start: 0,
        location_len: 0,
        valid: false,
    };
}

/// Zero-copy parser for the first `BEGIN:VEVENT`..`END:VEVENT` block in `ics`.
/// Handles parameterized property names such as `DTSTART;TZID=UTC:20261126T120000Z`.
pub fn parseFirstEvent(ics: []const u8) -> ParsedEvent {
    let ev = emptyParsedEvent();
    let in_event = false;

    let pos: usize = 0;
    const n = ics.len;
    while (pos < n) {
        const line_start = pos;
        while (pos < n && ics[pos] != 10 && ics[pos] != 13) {
            pos += 1;
        }
        const line_end = pos;
        if (pos < n && ics[pos] == 13) {
            pos += 1;
        }
        if (pos < n && ics[pos] == 10) {
            pos += 1;
        }

        if (line_end <= line_start) {
            continue;
        }

        // Find ':'
        let colon = line_start;
        while (colon < line_end && ics[colon] != 58) {
            colon += 1;
        }
        if (colon >= line_end) {
            continue;
        }

        // Property name ends at first ';' or ':'
        let name_end = line_start;
        while (name_end < colon && ics[name_end] != 59) {
            name_end += 1;
        }

        const val_start = colon + 1;
        const val_len = line_end - val_start;

        if (eqIgnoreCaseUpper(ics, line_start, name_end, "BEGIN") && eqIgnoreCaseUpper(ics, val_start, line_end, "VEVENT")) {
            in_event = true;
            continue;
        }

        if (in_event) {
            if (eqIgnoreCaseUpper(ics, line_start, name_end, "END") && eqIgnoreCaseUpper(ics, val_start, line_end, "VEVENT")) {
                ev.valid = true;
                return ev;
            }

            if (eqIgnoreCaseUpper(ics, line_start, name_end, "UID")) {
                ev.uid_start = val_start;
                ev.uid_len = val_len;
            } else if (eqIgnoreCaseUpper(ics, line_start, name_end, "DTSTART")) {
                ev.dtstart_start = val_start;
                ev.dtstart_len = val_len;
            } else if (eqIgnoreCaseUpper(ics, line_start, name_end, "DTEND")) {
                ev.dtend_start = val_start;
                ev.dtend_len = val_len;
            } else if (eqIgnoreCaseUpper(ics, line_start, name_end, "SUMMARY")) {
                ev.summary_start = val_start;
                ev.summary_len = val_len;
            } else if (eqIgnoreCaseUpper(ics, line_start, name_end, "LOCATION")) {
                ev.location_start = val_start;
                ev.location_len = val_len;
            }
        }
    }

    return emptyParsedEvent();
}

/// Unescapes RFC 5545 TEXT escape sequences (`\n`/`\N` -> LF, `\,` -> `,`, `\;` -> `;`, `\\` -> `\`).
/// Returns bytes written into `dst`, or `-1` if `dst` is too small.
pub fn unescapeText(src: []const u8, dst: []u8) -> i64 {
    let i: usize = 0;
    let out: usize = 0;
    const n = src.len;

    while (i < n) {
        if (out >= dst.len) {
            return -1;
        }
        if (src[i] == 92 && i + 1 < n) {
            const nxt = src[i + 1];
            if (nxt == 110 || nxt == 78) {
                dst[out] = 10; // '\n'
            } else {
                dst[out] = nxt;
            }
            out += 1;
            i += 2;
        } else {
            dst[out] = src[i];
            out += 1;
            i += 1;
        }
    }

    return out as i64;
}
