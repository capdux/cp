pub const TZIF_MAGIC_LEN: usize = 4;
pub const TZIF_HEADER_LEN: usize = 44;

pub struct TzifHeader {
    pub version: u8, // 0 for v1, 2 for '2', 3 for '3'
    pub isutcnt: u32,
    pub isstdcnt: u32,
    pub leapcnt: u32,
    pub timecnt: u32,
    pub typecnt: u32,
    pub charcnt: u32,
}

pub struct PosixTz {
    pub std_abbr: string,
    pub std_offset_secs: i32, // Positive east of UTC (inverted from POSIX sign!)
    pub has_dst: bool,
    pub dst_abbr: string,
    pub dst_offset_secs: i32,
}

pub struct TzTransition {
    pub at_unix: i64,
    pub offset_secs: i32,
    pub is_dst: bool,
}

fn tzReadU32Be(buf: []u8, off: usize): u32 {
    let b0 = (buf[off] as u32) << 24;
    let b1 = (buf[off + 1] as u32) << 16;
    let b2 = (buf[off + 2] as u32) << 8;
    let b3 = buf[off + 3] as u32;
    return b0 | b1 | b2 | b3;
}

pub fn isTzifMagic(buf: []u8): bool {
    if (buf.len < 4) { return false; }
    // "TZif"
    return buf[0] == 84 && buf[1] == 90 && buf[2] == 105 && buf[3] == 102;
}

pub fn parseTzifHeader(buf: []u8): TzifHeader? {
    if (buf.len < TZIF_HEADER_LEN || !isTzifMagic(buf)) {
        return null;
    }
    let ver_byte = buf[4];
    let ver: u8 = 0;
    if (ver_byte == 0) { ver = 1; }
    else if (ver_byte == 50) { ver = 2; } // '2'
    else if (ver_byte == 51) { ver = 3; } // '3'
    else if (ver_byte == 52) { ver = 4; } // '4'
    else { return null; }

    let typecnt = tzReadU32Be(buf, 36);
    if (typecnt == 0) { return null; }

    return TzifHeader {
        version: ver,
        isutcnt: tzReadU32Be(buf, 20),
        isstdcnt: tzReadU32Be(buf, 24),
        leapcnt: tzReadU32Be(buf, 28),
        timecnt: tzReadU32Be(buf, 32),
        typecnt: typecnt,
        charcnt: tzReadU32Be(buf, 40),
    };
}

fn tzIsAlpha(c: u8): bool {
    return (c >= 65 && c <= 90) || (c >= 97 && c <= 122);
}

fn tzIsDigit(c: u8): bool {
    return c >= 48 && c <= 57;
}

struct TzAbbrRes {
    abbr: string,
    next_pos: usize,
    ok: bool,
}

struct TzOffRes {
    secs: i32,
    next_pos: usize,
    ok: bool,
}

fn tzParseAbbr(s: string, pos_in: usize): TzAbbrRes {
    if (pos_in >= s.len) { return TzAbbrRes { abbr: "", next_pos: pos_in, ok: false }; }
    if (s[pos_in] == 60) { // '<' quoted abbr like <+11>
        let p = pos_in + 1;
        let start = p;
        while (p < s.len && s[p] != 62) { // '>'
            p = p + 1;
        }
        if (p >= s.len || p - start < 1) { return TzAbbrRes { abbr: "", next_pos: pos_in, ok: false }; }
        return TzAbbrRes { abbr: s[start..p], next_pos: p + 1, ok: true };
    }
    let p = pos_in;
    while (p < s.len && tzIsAlpha(s[p])) {
        p = p + 1;
    }
    if (p - pos_in < 3) { return TzAbbrRes { abbr: "", next_pos: pos_in, ok: false }; }
    return TzAbbrRes { abbr: s[pos_in..p], next_pos: p, ok: true };
}

// Parses POSIX offset [+-]hh[:mm[:ss]] and returns east-of-UTC seconds (POSIX sign is inverted)
fn tzParsePosixOffset(s: string, pos_in: usize): TzOffRes {
    if (pos_in >= s.len) { return TzOffRes { secs: 0, next_pos: pos_in, ok: false }; }
    let p = pos_in;
    let posix_neg = false;
    if (s[p] == 45) { // '-'
        posix_neg = true;
        p = p + 1;
    } else if (s[p] == 43) { // '+'
        p = p + 1;
    }
    if (p >= s.len || !tzIsDigit(s[p])) { return TzOffRes { secs: 0, next_pos: pos_in, ok: false }; }
    let hh: i32 = 0;
    while (p < s.len && tzIsDigit(s[p])) {
        hh = hh * 10 + ((s[p] - 48) as i32);
        p = p + 1;
    }
    let mm: i32 = 0;
    let ss: i32 = 0;
    if (p < s.len && s[p] == 58) { // ':'
        p = p + 1;
        if (p >= s.len || !tzIsDigit(s[p])) { return TzOffRes { secs: 0, next_pos: pos_in, ok: false }; }
        while (p < s.len && tzIsDigit(s[p])) {
            mm = mm * 10 + ((s[p] - 48) as i32);
            p = p + 1;
        }
        if (p < s.len && s[p] == 58) {
            p = p + 1;
            if (p >= s.len || !tzIsDigit(s[p])) { return TzOffRes { secs: 0, next_pos: pos_in, ok: false }; }
            while (p < s.len && tzIsDigit(s[p])) {
                ss = ss * 10 + ((s[p] - 48) as i32);
                p = p + 1;
            }
        }
    }
    if (hh > 24 || mm > 59 || ss > 59) { return TzOffRes { secs: 0, next_pos: pos_in, ok: false }; }
    let total = hh * 3600 + mm * 60 + ss;
    // In POSIX TZ, "EST5" means UTC-5 (-18000s), "AEST-10" means UTC+10 (+36000s)
    let east_secs: i32 = if (posix_neg) { total } else { -total };
    return TzOffRes { secs: east_secs, next_pos: p, ok: true };
}

pub fn parsePosixTz(spec: string): PosixTz? {
    let std_r = tzParseAbbr(spec, 0);
    if (!std_r.ok) { return null; }
    let std_abbr = std_r.abbr;
    let pos = std_r.next_pos;

    let std_o = tzParsePosixOffset(spec, pos);
    if (!std_o.ok) { return null; }
    let std_off = std_o.secs;
    pos = std_o.next_pos;

    let has_dst = false;
    let dst_abbr = "";
    let dst_off = std_off + 3600;
    if (pos < spec.len && spec[pos] != 44) { // not ','
        let dst_r = tzParseAbbr(spec, pos);
        if (!dst_r.ok) { return null; }
        has_dst = true;
        dst_abbr = dst_r.abbr;
        pos = dst_r.next_pos;
        if (pos < spec.len && spec[pos] != 44) {
            let doff_r = tzParsePosixOffset(spec, pos);
            if (!doff_r.ok) { return null; }
            dst_off = doff_r.secs;
            pos = doff_r.next_pos;
        }
    }
    return PosixTz {
        std_abbr: std_abbr,
        std_offset_secs: std_off,
        has_dst: has_dst,
        dst_abbr: dst_abbr,
        dst_offset_secs: dst_off,
    };
}

pub fn lookupTransitionOffset(transitions: []TzTransition, unix_secs: i64, default_offset: i32): i32 {
    if (transitions.len == 0) { return default_offset; }
    let best = default_offset;
    let i: usize = 0;
    while (i < transitions.len) {
        if (transitions[i].at_unix <= unix_secs) {
            best = transitions[i].offset_secs;
        } else {
            break;
        }
        i = i + 1;
    }
    return best;
}

pub fn utcToLocalSeconds(unix_secs: i64, offset_secs: i32): i64 {
    return unix_secs + (offset_secs as i64);
}
