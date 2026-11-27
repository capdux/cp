// @cap/subtitle — Zero-heap SRT & WebVTT subtitle parser, time-shifter & converter
// Clean-room MIT implementation in pure CP (replaces npm `subtitle` / `srt-parser-2`).

pub enum SubtitleFormat {
    Srt,
    WebVtt,
}

pub struct Cue {
    pub index: u32,
    pub start_ms: i64,
    pub end_ms: i64,
    pub text: string,
}

pub struct CueList {
    pub cues: [64]Cue,
    pub count: usize,
    pub format: SubtitleFormat,
}

fn isDigit(c: u8) -> bool {
    return c >= '0' && c <= '9';
}

fn parseUint(s: string) -> i64? {
    if (s.len == 0) { return null; }
    let val: i64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        if (!isDigit(s[i])) { return null; }
        val = val * 10 + ((s[i] - ('0' as u8)) as i64);
        i += 1;
    }
    return val;
}

fn trimSpaces(s: string) -> string {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && (s[start] == ' ' || s[start] == '\t' || s[start] == '\r')) {
        start += 1;
    }
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r')) {
        end -= 1;
    }
    return s[start..end];
}

/// Parse an SRT (`HH:MM:SS,mmm`) or WebVTT (`HH:MM:SS.mmm` or `MM:SS.mmm`) timestamp into milliseconds.
pub fn parseTimestampMs(raw: string) -> i64? {
    const s: string = trimSpaces(raw);
    if (s.len < 6) { return null; }

    // Find decimal separator ',' or '.'
    let sepPos: usize = s.len;
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == ',' || s[i] == '.') {
            sepPos = i;
            break;
        }
        i += 1;
    }
    if (sepPos == s.len || sepPos + 1 >= s.len) { return null; }

    const hmsPart: string = s[0..sepPos];
    const msPart: string = s[(sepPos + 1)..s.len];
    if (msPart.len > 3) { return null; }
    const msOpt = parseUint(msPart);
    if (msOpt == null) { return null; }
    let ms: i64 = msOpt ?? 0;
    if (msPart.len == 1) { ms = ms * 100; }
    else if (msPart.len == 2) { ms = ms * 10; }

    // Split hmsPart by ':' (either MM:SS or HH:MM:SS)
    let c1: usize = hmsPart.len;
    let c2: usize = hmsPart.len;
    let k: usize = 0;
    while (k < hmsPart.len) {
        if (hmsPart[k] == ':') {
            if (c1 == hmsPart.len) {
                c1 = k;
            } else if (c2 == hmsPart.len) {
                c2 = k;
            } else {
                return null;
            }
        }
        k += 1;
    }

    if (c1 == hmsPart.len) { return null; }

    let hours: i64 = 0;
    let mins: i64 = 0;
    let secs: i64 = 0;
    if (c2 == hmsPart.len) {
        // MM:SS
        const mOpt = parseUint(hmsPart[0..c1]);
        const sOpt = parseUint(hmsPart[(c1 + 1)..hmsPart.len]);
        if (mOpt == null || sOpt == null) { return null; }
        mins = mOpt ?? 0;
        secs = sOpt ?? 0;
    } else {
        // HH:MM:SS
        const hOpt = parseUint(hmsPart[0..c1]);
        const mOpt = parseUint(hmsPart[(c1 + 1)..c2]);
        const sOpt = parseUint(hmsPart[(c2 + 1)..hmsPart.len]);
        if (hOpt == null || mOpt == null || sOpt == null) { return null; }
        hours = hOpt ?? 0;
        mins = mOpt ?? 0;
        secs = sOpt ?? 0;
    }

    if (mins > 59 || secs > 59) { return null; }
    return ((hours * 3600) + (mins * 60) + secs) * 1000 + ms;
}

fn put2(out: []u8, pos: *usize, v: i64) {
    if (pos.* + 2 <= out.len) {
        out[pos.*] = ('0' as u8) + (@rem(@divTrunc(v, 10), 10) as u8);
        out[pos.* + 1] = ('0' as u8) + (@rem(v, 10) as u8);
        pos.* += 2;
    }
}

fn put3(out: []u8, pos: *usize, v: i64) {
    if (pos.* + 3 <= out.len) {
        out[pos.*] = ('0' as u8) + (@rem(@divTrunc(v, 100), 10) as u8);
        out[pos.* + 1] = ('0' as u8) + (@rem(@divTrunc(v, 10), 10) as u8);
        out[pos.* + 2] = ('0' as u8) + (@rem(v, 10) as u8);
        pos.* += 3;
    }
}

/// Format milliseconds into `HH:MM:SS,mmm` (SRT) or `HH:MM:SS.mmm` (WebVTT).
pub fn formatTimestampMs(ms_in: i64, fmt: SubtitleFormat, out: []u8) -> string {
    let total: i64 = ms_in;
    if (total < 0) { total = 0; }
    const ms: i64 = @rem(total, 1000);
    const totalSec: i64 = @divTrunc(total, 1000);
    const sec: i64 = @rem(totalSec, 60);
    const totalMin: i64 = @divTrunc(totalSec, 60);
    const min: i64 = @rem(totalMin, 60);
    const hr: i64 = @divTrunc(totalMin, 60);

    let pos: usize = 0;
    put2(out, &pos, hr);
    if (pos < out.len) { out[pos] = ':'; pos += 1; }
    put2(out, &pos, min);
    if (pos < out.len) { out[pos] = ':'; pos += 1; }
    put2(out, &pos, sec);
    let sep: u8 = ',' as u8;
    switch (fmt) {
        SubtitleFormat.Srt => { sep = ',' as u8; },
        SubtitleFormat.WebVtt => { sep = '.' as u8; },
    }
    if (pos < out.len) { out[pos] = sep; pos += 1; }
    put3(out, &pos, ms);
    return out[0..pos];
}

fn findArrow(line: string) -> usize? {
    if (line.len < 3) { return null; }
    let i: usize = 0;
    while (i + 3 <= line.len) {
        if (line[i] == '-' && line[i + 1] == '-' && line[i + 2] == '>') {
            return i;
        }
        i += 1;
    }
    return null;
}

/// Parse SRT or WebVTT text into a zero-heap CueList (zero-copy cue text slices).
pub fn parseSubtitles(input: string) -> CueList {
    let cues: [64]Cue = undefined;
    let list: CueList = CueList {
        cues: cues,
        count: 0,
        format: SubtitleFormat.Srt,
    };

    // Detect WebVTT header
    if (input.len >= 6 && input[0..6] == "WEBVTT") {
        list.format = SubtitleFormat.WebVtt;
    }

    let pos: usize = 0;
    let nextIdx: u32 = 1;
    while (pos < input.len && list.count < 64) {
        // Read one line
        let lineEnd: usize = pos;
        while (lineEnd < input.len && input[lineEnd] != '\n') {
            lineEnd += 1;
        }
        const line: string = trimSpaces(input[pos..lineEnd]);
        pos = if (lineEnd < input.len) lineEnd + 1 else input.len;

        const arrowOpt = findArrow(line);
        if (arrowOpt != null) {
            const arrow: usize = arrowOpt ?? (0 as usize);
            const left: string = trimSpaces(line[0..arrow]);
            // Right side may have WebVTT cue settings after space
            let rightRaw: string = trimSpaces(line[(arrow + 3)..line.len]);
            let sp: usize = 0;
            while (sp < rightRaw.len && rightRaw[sp] != ' ' && rightRaw[sp] != '\t') {
                sp += 1;
            }
            const right: string = rightRaw[0..sp];

            const startOpt = parseTimestampMs(left);
            const endOpt = parseTimestampMs(right);
            if (startOpt != null && endOpt != null) {
                // Capture cue text lines until blank line or EOF
                const textStart: usize = pos;
                let textEnd: usize = pos;
                while (pos < input.len) {
                    let le: usize = pos;
                    while (le < input.len && input[le] != '\n') {
                        le += 1;
                    }
                    const tline: string = trimSpaces(input[pos..le]);
                    if (tline.len == 0) {
                        pos = if (le < input.len) le + 1 else input.len;
                        break;
                    }
                    textEnd = le;
                    if (textEnd > textStart && input[textEnd - 1] == '\r') {
                        textEnd -= 1;
                    }
                    pos = if (le < input.len) le + 1 else input.len;
                }

                list.cues[list.count] = Cue {
                    index: nextIdx,
                    start_ms: startOpt ?? 0,
                    end_ms: endOpt ?? 0,
                    text: input[textStart..textEnd],
                };
                list.count += 1;
                nextIdx += 1;
            }
        }
    }

    return list;
}

/// Shift all cue timestamps by `offset_ms` (clamped to >= 0).
pub fn shiftCues(list: *CueList, offset_ms: i64) {
    let i: usize = 0;
    while (i < list.count) {
        let s: i64 = list.cues[i].start_ms + offset_ms;
        let e: i64 = list.cues[i].end_ms + offset_ms;
        if (s < 0) { s = 0; }
        if (e < 0) { e = 0; }
        list.cues[i].start_ms = s;
        list.cues[i].end_ms = e;
        i += 1;
    }
}

fn writeStr(out: []u8, pos: *usize, s: string) {
    let i: usize = 0;
    while (i < s.len && pos.* < out.len) {
        out[pos.*] = s[i];
        pos.* += 1;
        i += 1;
    }
}

fn writeU32(out: []u8, pos: *usize, val: u32) {
    if (val == 0) {
        writeStr(out, pos, "0");
        return;
    }
    let tmp: [12]u8 = undefined;
    let n: usize = 0;
    let v: u32 = val;
    while (v > 0 && n < 12) {
        tmp[n] = ('0' as u8) + ((v % 10) as u8);
        v = v / 10;
        n += 1;
    }
    while (n > 0) {
        n -= 1;
        if (pos.* < out.len) {
            out[pos.*] = tmp[n];
            pos.* += 1;
        }
    }
}

/// Render a CueList to SRT or WebVTT format into `out`.
pub fn formatSubtitles(list: CueList, target_fmt: SubtitleFormat, out: []u8) -> string {
    let pos: usize = 0;
    let isVtt: bool = false;
    switch (target_fmt) {
        SubtitleFormat.Srt => { isVtt = false; },
        SubtitleFormat.WebVtt => { isVtt = true; },
    }

    if (isVtt) {
        writeStr(out, &pos, "WEBVTT\n\n");
    }

    let tsBuf: [16]u8 = undefined;
    let i: usize = 0;
    while (i < list.count) {
        const c: Cue = list.cues[i];
        if (!isVtt) {
            writeU32(out, &pos, (i + 1) as u32);
            writeStr(out, &pos, "\n");
        }
        const sStr: string = formatTimestampMs(c.start_ms, target_fmt, tsBuf[0..16]);
        writeStr(out, &pos, sStr);
        writeStr(out, &pos, " --> ");
        const eStr: string = formatTimestampMs(c.end_ms, target_fmt, tsBuf[0..16]);
        writeStr(out, &pos, eStr);
        writeStr(out, &pos, "\n");
        writeStr(out, &pos, c.text);
        if (i + 1 < list.count) {
            writeStr(out, &pos, "\n\n");
        } else {
            writeStr(out, &pos, "\n");
        }
        i += 1;
    }

    return out[0..pos];
}
