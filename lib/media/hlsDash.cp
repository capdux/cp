// Origin: Pure-CP Stdlib (Unbounded-Slice RFC 8216 HLS/LL-HLS M3U8 Codec, MPEG-DASH SegmentTemplate & Dual-EWMA/BOLA ABR Controller) | Created: 2026-11-23
// 100% pure CP, zero dynamic heap allocation, unbounded caller-slice adaptive streaming protocol & ABR engine:
//   - RFC 8216 HLS Master Playlist parser & formatter (`parseHlsMasterPlaylist` / `formatHlsMasterPlaylist`)
//     with quoted-comma attribute handling (`CODECS`, `BANDWIDTH`, `AVERAGE-BANDWIDTH`, `RESOLUTION`, `FRAME-RATE`)
//   - RFC 8216 HLS & LL-HLS Media Playlist parser & formatter (`parseHlsMediaPlaylist` / `formatHlsMediaPlaylist`)
//     with `#EXT-X-BYTERANGE` implicit offset accumulation, `#EXT-X-DISCONTINUITY`, `#EXT-X-KEY`, `#EXT-X-PART`
//   - ISO/IEC 23009-1 MPEG-DASH `SegmentTemplate` expander (`$RepresentationID$`, `$Number[%0Nd]$`,
//     `$Bandwidth[%0Nd]$`, `$Time[%0Nd]$`, `$$` literal escape)
//   - Production Adaptive Bitrate (ABR) controller (`AbrController`): Fast/Slow Dual-EWMA throughput estimator,
//     BOLA logarithmic buffer utility, safety-factor capping, upgrade hysteresis cooldown & low-buffer panic guard

const std = @import("std");

// ============================================================================
// 1. Error Set & Enums
// ============================================================================

/// Error set returned by HLS/DASH manifest codecs and ABR routines.
pub err StreamManifestError {
    MissingExtM3u,
    InvalidTag,
    InvalidNumber,
    InvalidByteRange,
    BufferTooSmall,
    OutputSliceTooSmall,
    InvalidTemplate,
    InvalidConfig,
}

/// Classification of an HLS M3U8 playlist.
pub enum HlsPlaylistKind {
    master,
    media_vod,
    media_event,
    media_live,
}

// ============================================================================
// 2. Internal Zero-Alloc Parsing & Formatting Helpers
// ============================================================================

fn bytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn startsWith(haystack: []const u8, prefix: []const u8): bool {
    if (haystack.len < prefix.len) return false;
    let i: usize = 0;
    while (i < prefix.len) : (i += 1) {
        if (haystack[i] != prefix[i]) return false;
    }
    return true;
}

fn trimLineBounds(raw: []const u8, start_in: usize, end_in: usize, out_start: *usize, out_end: *usize) {
    let s: usize = start_in;
    let e: usize = end_in;
    while (s < e && (raw[s] == ' ' || raw[s] == '\t' || raw[s] == '\r')) {
        s += 1;
    }
    while (e > s && (raw[e - 1] == ' ' || raw[e - 1] == '\t' || raw[e - 1] == '\r')) {
        e -= 1;
    }
    out_start.* = s;
    out_end.* = e;
}

fn parseU64Slice(s: []const u8): u64 ! StreamManifestError {
    if (s.len == 0) return StreamManifestError.InvalidNumber;
    let val: u64 = 0;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const ch: u8 = s[i];
        if (ch < '0' || ch > '9') return StreamManifestError.InvalidNumber;
        val = val * 10 + @as(u64, ch - '0');
    }
    return val;
}

/// Parses a non-negative decimal seconds string (e.g. `"6.006"` or `"2"`) into integer milliseconds.
fn parseDecimalMillis(s: []const u8): u64 ! StreamManifestError {
    if (s.len == 0) return StreamManifestError.InvalidNumber;
    let int_part: u64 = 0;
    let frac_ms: u64 = 0;
    let frac_scale: u64 = 100;
    let in_frac: bool = false;
    let digits_seen: usize = 0;

    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const ch: u8 = s[i];
        if (ch == '.') {
            if (in_frac) return StreamManifestError.InvalidNumber;
            in_frac = true;
            continue;
        }
        if (ch < '0' || ch > '9') return StreamManifestError.InvalidNumber;
        digits_seen += 1;
        const d: u64 = @as(u64, ch - '0');
        if (!in_frac) {
            int_part = int_part * 10 + d;
        } else if (frac_scale > 0) {
            frac_ms += d * frac_scale;
            frac_scale /= 10;
        }
    }
    if (digits_seen == 0) return StreamManifestError.InvalidNumber;
    return int_part * 1000 + frac_ms;
}

fn appendBytes(dst: *[]u8, pos: *usize, src: []const u8): bool {
    if (pos.* + src.len > dst.len) return false;
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        dst[pos.* + i] = src[i];
    }
    pos.* += src.len;
    return true;
}

fn appendByte(dst: *[]u8, pos: *usize, b: u8): bool {
    if (pos.* >= dst.len) return false;
    dst[pos.*] = b;
    pos.* += 1;
    return true;
}

fn appendU64Padded(dst: *[]u8, pos: *usize, val_in: u64, min_width: usize, pad_char: u8): bool {
    let digits: [24]u8 = [0; 24];
    let len: usize = 0;
    let v: u64 = val_in;
    if (v == 0) {
        digits[0] = '0';
        len = 1;
    } else {
        while (v > 0) {
            digits[len] = @as(u8, @intCast('0' + (v % 10)));
            len += 1;
            v /= 10;
        }
    }

    const pad_count: usize = if (min_width > len) min_width - len else 0;
    if (pos.* + pad_count + len > dst.len) return false;

    let p: usize = 0;
    while (p < pad_count) : (p += 1) {
        dst[pos.* + p] = pad_char;
    }
    pos.* += pad_count;

    let i: usize = 0;
    while (i < len) : (i += 1) {
        dst[pos.* + i] = digits[len - 1 - i];
    }
    pos.* += len;
    return true;
}

fn appendMillisAsSec(dst: *[]u8, pos: *usize, ms: u64): bool {
    const sec: u64 = ms / 1000;
    const rem: u64 = ms % 1000;
    if (!appendU64Padded(dst, pos, sec, 0, '0')) return false;
    if (!appendByte(dst, pos, '.')) return false;
    return appendU64Padded(dst, pos, rem, 3, '0');
}

/// Scans the next `KEY=VALUE` attribute pair inside `raw[cursor..end]` respecting quoted strings.
/// Returns `true` if a pair was extracted.
fn nextAttrPair(
    raw: []const u8,
    cursor: *usize,
    end: usize,
    key_off: *usize,
    key_len: *usize,
    val_off: *usize,
    val_len: *usize,
    val_quoted: *bool,
): bool {
    let p: usize = cursor.*;
    while (p < end && (raw[p] == ',' || raw[p] == ' ' || raw[p] == '\t')) {
        p += 1;
    }
    if (p >= end) {
        cursor.* = end;
        return false;
    }

    const ks: usize = p;
    while (p < end && raw[p] != '=' && raw[p] != ',') {
        p += 1;
    }
    if (p >= end || raw[p] != '=') {
        cursor.* = end;
        return false;
    }
    key_off.* = ks;
    key_len.* = p - ks;
    p += 1; // skip '='

    if (p < end && raw[p] == '"') {
        val_quoted.* = true;
        p += 1;
        const vs: usize = p;
        while (p < end && raw[p] != '"') {
            p += 1;
        }
        val_off.* = vs;
        val_len.* = p - vs;
        if (p < end && raw[p] == '"') p += 1;
    } else {
        val_quoted.* = false;
        const vs: usize = p;
        while (p < end && raw[p] != ',') {
            p += 1;
        }
        val_off.* = vs;
        val_len.* = p - vs;
    }

    cursor.* = p;
    return true;
}

fn skipBomAndVerifyExtM3u(raw: []const u8): usize ! StreamManifestError {
    let pos: usize = 0;
    if (raw.len >= 3 && raw[0] == 0xEF && raw[1] == 0xBB && raw[2] == 0xBF) {
        pos = 3;
    }
    while (pos < raw.len) {
        let line_end: usize = pos;
        while (line_end < raw.len && raw[line_end] != '\n') {
            line_end += 1;
        }
        let s: usize = 0;
        let e: usize = 0;
        trimLineBounds(raw, pos, line_end, *s, *e);
        const next_pos: usize = if (line_end < raw.len) line_end + 1 else raw.len;
        if (s == e) {
            pos = next_pos;
            continue;
        }
        if (bytesEql(raw[s..e], "#EXTM3U")) {
            return next_pos;
        }
        return StreamManifestError.MissingExtM3u;
    }
    return StreamManifestError.MissingExtM3u;
}

// ============================================================================
// 3. RFC 8216 HLS Master Playlist Codec
// ============================================================================

/// Zero-copy parsed HLS Master Playlist variant (`#EXT-X-STREAM-INF` + URI).
pub struct HlsVariant {
    bandwidth_bps: u64 = 0;
    average_bandwidth_bps: u64 = 0;
    width: u32 = 0;
    height: u32 = 0;
    frame_rate_milli: u32 = 0;
    codecs_off: usize = 0;
    codecs_len: usize = 0;
    uri_off: usize = 0;
    uri_len: usize = 0;

    /// Returns the zero-copy `CODECS` attribute slice inside the original manifest buffer `src`.
    fn codecs(src: []const u8): []const u8 {
        if (codecs_len == 0 || codecs_off + codecs_len > src.len) return "";
        return src[codecs_off..(codecs_off + codecs_len)];
    }

    /// Returns the zero-copy variant playlist URI slice inside the original manifest buffer `src`.
    fn uri(src: []const u8): []const u8 {
        if (uri_len == 0 || uri_off + uri_len > src.len) return "";
        return src[uri_off..(uri_off + uri_len)];
    }
}

/// Input descriptor for formatting an HLS Master Playlist via `formatHlsMasterPlaylist`.
pub struct HlsVariantSpec {
    bandwidth_bps: u64 = 0;
    average_bandwidth_bps: u64 = 0;
    width: u32 = 0;
    height: u32 = 0;
    frame_rate_milli: u32 = 0;
    codecs: []const u8 = "";
    uri: []const u8 = "";
}

fn parseResolution(s: []const u8, out_w: *u32, out_h: *u32): bool {
    let x_pos: usize = s.len;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] == 'x' || s[i] == 'X') {
            x_pos = i;
            break;
        }
    }
    if (x_pos == 0 || x_pos + 1 >= s.len) return false;
    const w64 = parseU64Slice(s[0..x_pos]) catch return false;
    const h64 = parseU64Slice(s[(x_pos + 1)..s.len]) catch return false;
    out_w.* = @as(u32, @intCast(w64));
    out_h.* = @as(u32, @intCast(h64));
    return true;
}

/// Parses an RFC 8216 HLS Master Playlist into caller's `out_variants` slice (zero heap allocation).
/// Returns the number of variants parsed.
pub fn parseHlsMasterPlaylist(
    raw: []const u8,
    out_variants: *[]HlsVariant,
): usize ! StreamManifestError {
    let pos: usize = skipBomAndVerifyExtM3u(raw)?;
    let count: usize = 0;
    let has_pending: bool = false;
    let pending = HlsVariant{};

    while (pos < raw.len) {
        let line_end: usize = pos;
        while (line_end < raw.len && raw[line_end] != '\n') {
            line_end += 1;
        }
        let s: usize = 0;
        let e: usize = 0;
        trimLineBounds(raw, pos, line_end, *s, *e);
        pos = if (line_end < raw.len) line_end + 1 else raw.len;

        if (s == e) continue;
        const line: []const u8 = raw[s..e];

        if (startsWith(line, "#EXT-X-STREAM-INF:")) {
            pending = HlsVariant{};
            has_pending = true;
            let cur: usize = s + 18;
            let k_off: usize = 0;
            let k_len: usize = 0;
            let v_off: usize = 0;
            let v_len: usize = 0;
            let v_quoted: bool = false;

            while (nextAttrPair(raw, *cur, e, *k_off, *k_len, *v_off, *v_len, *v_quoted)) {
                const key: []const u8 = raw[k_off..(k_off + k_len)];
                const val: []const u8 = raw[v_off..(v_off + v_len)];
                if (bytesEql(key, "BANDWIDTH")) {
                    pending.bandwidth_bps = parseU64Slice(val)?;
                } else if (bytesEql(key, "AVERAGE-BANDWIDTH")) {
                    pending.average_bandwidth_bps = parseU64Slice(val)?;
                } else if (bytesEql(key, "RESOLUTION")) {
                    if (!parseResolution(val, *pending.width, *pending.height)) {
                        return StreamManifestError.InvalidTag;
                    }
                } else if (bytesEql(key, "FRAME-RATE")) {
                    const fr_ms: u64 = parseDecimalMillis(val)?;
                    pending.frame_rate_milli = @as(u32, @intCast(fr_ms));
                } else if (bytesEql(key, "CODECS")) {
                    pending.codecs_off = v_off;
                    pending.codecs_len = v_len;
                }
            }
        } else if (line[0] == '#') {
            continue;
        } else if (has_pending) {
            if (count >= out_variants.len) {
                return StreamManifestError.OutputSliceTooSmall;
            }
            pending.uri_off = s;
            pending.uri_len = e - s;
            out_variants[count] = pending;
            count += 1;
            has_pending = false;
        }
    }

    return count;
}

/// Formats an RFC 8216 HLS Master Playlist from `variants_spec` into `out_buf`.
pub fn formatHlsMasterPlaylist(
    variants_spec: []const HlsVariantSpec,
    out_buf: *[]u8,
): usize ! StreamManifestError {
    let pos: usize = 0;
    if (!appendBytes(out_buf, *pos, "#EXTM3U\n#EXT-X-VERSION:7\n")) {
        return StreamManifestError.BufferTooSmall;
    }

    let i: usize = 0;
    while (i < variants_spec.len) : (i += 1) {
        const v = variants_spec[i];
        if (!appendBytes(out_buf, *pos, "#EXT-X-STREAM-INF:BANDWIDTH=")) {
            return StreamManifestError.BufferTooSmall;
        }
        if (!appendU64Padded(out_buf, *pos, v.bandwidth_bps, 0, '0')) {
            return StreamManifestError.BufferTooSmall;
        }
        if (v.average_bandwidth_bps > 0) {
            if (!appendBytes(out_buf, *pos, ",AVERAGE-BANDWIDTH=") ||
                !appendU64Padded(out_buf, *pos, v.average_bandwidth_bps, 0, '0'))
            {
                return StreamManifestError.BufferTooSmall;
            }
        }
        if (v.width > 0 && v.height > 0) {
            if (!appendBytes(out_buf, *pos, ",RESOLUTION=") ||
                !appendU64Padded(out_buf, *pos, @as(u64, v.width), 0, '0') ||
                !appendByte(out_buf, *pos, 'x') ||
                !appendU64Padded(out_buf, *pos, @as(u64, v.height), 0, '0'))
            {
                return StreamManifestError.BufferTooSmall;
            }
        }
        if (v.frame_rate_milli > 0) {
            if (!appendBytes(out_buf, *pos, ",FRAME-RATE=") ||
                !appendMillisAsSec(out_buf, *pos, @as(u64, v.frame_rate_milli)))
            {
                return StreamManifestError.BufferTooSmall;
            }
        }
        if (v.codecs.len > 0) {
            if (!appendBytes(out_buf, *pos, ",CODECS=\"") ||
                !appendBytes(out_buf, *pos, v.codecs) ||
                !appendByte(out_buf, *pos, '"'))
            {
                return StreamManifestError.BufferTooSmall;
            }
        }
        if (!appendByte(out_buf, *pos, '\n') ||
            !appendBytes(out_buf, *pos, v.uri) ||
            !appendByte(out_buf, *pos, '\n'))
        {
            return StreamManifestError.BufferTooSmall;
        }
    }

    return pos;
}

// ============================================================================
// 4. RFC 8216 HLS & LL-HLS Media Playlist Codec
// ============================================================================

/// Sub-range byte specification for `#EXT-X-BYTERANGE`.
pub struct HlsByteRange {
    length: u64 = 0;
    offset: u64 = 0;
    has_offset: bool = false;
}

/// Zero-copy parsed HLS media segment (`#EXTINF` + URI + byte range / discontinuity / encryption).
pub struct HlsSegment {
    media_sequence: u64 = 0;
    duration_ms: u64 = 0;
    discontinuity: bool = false;
    has_byte_range: bool = false;
    byte_range: HlsByteRange = HlsByteRange{};
    key_method_aes128: bool = false;
    uri_off: usize = 0;
    uri_len: usize = 0;

    /// Returns the zero-copy segment URI slice inside the original manifest buffer `src`.
    fn uri(src: []const u8): []const u8 {
        if (uri_len == 0 || uri_off + uri_len > src.len) return "";
        return src[uri_off..(uri_off + uri_len)];
    }
}

/// Input descriptor for formatting a media segment via `formatHlsMediaPlaylist`.
pub struct HlsSegmentSpec {
    duration_ms: u64 = 0;
    discontinuity: bool = false;
    has_byte_range: bool = false;
    byte_range: HlsByteRange = HlsByteRange{};
    uri: []const u8 = "";
}

/// Zero-copy parsed LL-HLS partial segment (`#EXT-X-PART`).
pub struct HlsPartialSegment {
    duration_ms: u64 = 0;
    independent: bool = false;
    uri_off: usize = 0;
    uri_len: usize = 0;

    /// Returns the zero-copy partial segment URI slice inside the original manifest buffer `src`.
    fn uri(src: []const u8): []const u8 {
        if (uri_len == 0 || uri_off + uri_len > src.len) return "";
        return src[uri_off..(uri_off + uri_len)];
    }
}

/// Summary metadata of a parsed or formatted HLS Media Playlist.
pub struct HlsMediaPlaylistInfo {
    kind: HlsPlaylistKind = HlsPlaylistKind.media_live;
    version: u32 = 3;
    target_duration_sec: u32 = 0;
    media_sequence_base: u64 = 0;
    discontinuity_sequence: u64 = 0;
    part_target_ms: u64 = 0;
    end_list: bool = false;
    segment_count: usize = 0;
    part_count: usize = 0;
    total_duration_ms: u64 = 0;
}

fn parseByteRangeValue(val: []const u8): HlsByteRange ! StreamManifestError {
    let at_pos: usize = val.len;
    let i: usize = 0;
    while (i < val.len) : (i += 1) {
        if (val[i] == '@') {
            at_pos = i;
            break;
        }
    }
    if (at_pos == 0) return StreamManifestError.InvalidByteRange;
    const len_v: u64 = parseU64Slice(val[0..at_pos]) catch return StreamManifestError.InvalidByteRange;
    if (at_pos < val.len) {
        const off_v: u64 = parseU64Slice(val[(at_pos + 1)..val.len]) catch return StreamManifestError.InvalidByteRange;
        return HlsByteRange{ length: len_v, offset: off_v, has_offset: true };
    }
    return HlsByteRange{ length: len_v, offset: 0, has_offset: false };
}

/// Parses an RFC 8216 HLS / LL-HLS Media Playlist into caller's `out_segments` and `out_parts` slices.
pub fn parseHlsMediaPlaylist(
    raw: []const u8,
    out_segments: *[]HlsSegment,
    out_parts: *[]HlsPartialSegment,
): HlsMediaPlaylistInfo ! StreamManifestError {
    let pos: usize = skipBomAndVerifyExtM3u(raw)?;
    let info = HlsMediaPlaylistInfo{};

    let explicit_vod: bool = false;
    let explicit_event: bool = false;
    let cur_aes128: bool = false;
    let next_discontinuity: bool = false;
    let has_pending_extinf: bool = false;
    let pending_dur_ms: u64 = 0;
    let has_pending_br: bool = false;
    let pending_br = HlsByteRange{};
    let prev_br_end: u64 = 0;
    let has_prev_br: bool = false;

    while (pos < raw.len) {
        let line_end: usize = pos;
        while (line_end < raw.len && raw[line_end] != '\n') {
            line_end += 1;
        }
        let s: usize = 0;
        let e: usize = 0;
        trimLineBounds(raw, pos, line_end, *s, *e);
        pos = if (line_end < raw.len) line_end + 1 else raw.len;

        if (s == e) continue;
        const line: []const u8 = raw[s..e];

        if (startsWith(line, "#EXT-X-VERSION:")) {
            const v64: u64 = parseU64Slice(line[15..line.len])?;
            info.version = @as(u32, @intCast(v64));
        } else if (startsWith(line, "#EXT-X-TARGETDURATION:")) {
            const td: u64 = parseU64Slice(line[22..line.len])?;
            info.target_duration_sec = @as(u32, @intCast(td));
        } else if (startsWith(line, "#EXT-X-MEDIA-SEQUENCE:")) {
            info.media_sequence_base = parseU64Slice(line[22..line.len])?;
        } else if (startsWith(line, "#EXT-X-DISCONTINUITY-SEQUENCE:")) {
            info.discontinuity_sequence = parseU64Slice(line[30..line.len])?;
        } else if (startsWith(line, "#EXT-X-PLAYLIST-TYPE:")) {
            const pt: []const u8 = line[21..line.len];
            if (bytesEql(pt, "VOD")) {
                explicit_vod = true;
            } else if (bytesEql(pt, "EVENT")) {
                explicit_event = true;
            }
        } else if (startsWith(line, "#EXT-X-PART-INF:")) {
            let cur: usize = s + 16;
            let k_off: usize = 0;
            let k_len: usize = 0;
            let v_off: usize = 0;
            let v_len: usize = 0;
            let v_quoted: bool = false;
            while (nextAttrPair(raw, *cur, e, *k_off, *k_len, *v_off, *v_len, *v_quoted)) {
                if (bytesEql(raw[k_off..(k_off + k_len)], "PART-TARGET")) {
                    info.part_target_ms = parseDecimalMillis(raw[v_off..(v_off + v_len)])?;
                }
            }
        } else if (startsWith(line, "#EXT-X-KEY:")) {
            let cur: usize = s + 11;
            let k_off: usize = 0;
            let k_len: usize = 0;
            let v_off: usize = 0;
            let v_len: usize = 0;
            let v_quoted: bool = false;
            while (nextAttrPair(raw, *cur, e, *k_off, *k_len, *v_off, *v_len, *v_quoted)) {
                if (bytesEql(raw[k_off..(k_off + k_len)], "METHOD")) {
                    const mval: []const u8 = raw[v_off..(v_off + v_len)];
                    cur_aes128 = bytesEql(mval, "AES-128");
                }
            }
        } else if (bytesEql(line, "#EXT-X-DISCONTINUITY")) {
            next_discontinuity = true;
        } else if (startsWith(line, "#EXT-X-BYTERANGE:")) {
            let br = parseByteRangeValue(line[17..line.len])?;
            if (!br.has_offset) {
                if (!has_prev_br) {
                    return StreamManifestError.InvalidByteRange;
                }
                br.offset = prev_br_end;
            }
            pending_br = br;
            has_pending_br = true;
        } else if (startsWith(line, "#EXTINF:")) {
            const body: []const u8 = line[8..line.len];
            let comma: usize = body.len;
            let ci: usize = 0;
            while (ci < body.len) : (ci += 1) {
                if (body[ci] == ',') {
                    comma = ci;
                    break;
                }
            }
            pending_dur_ms = parseDecimalMillis(body[0..comma])?;
            has_pending_extinf = true;
        } else if (startsWith(line, "#EXT-X-PART:")) {
            if (info.part_count >= out_parts.len) {
                return StreamManifestError.OutputSliceTooSmall;
            }
            let part = HlsPartialSegment{};
            let cur: usize = s + 12;
            let k_off: usize = 0;
            let k_len: usize = 0;
            let v_off: usize = 0;
            let v_len: usize = 0;
            let v_quoted: bool = false;
            while (nextAttrPair(raw, *cur, e, *k_off, *k_len, *v_off, *v_len, *v_quoted)) {
                const key: []const u8 = raw[k_off..(k_off + k_len)];
                const val: []const u8 = raw[v_off..(v_off + v_len)];
                if (bytesEql(key, "DURATION")) {
                    part.duration_ms = parseDecimalMillis(val)?;
                } else if (bytesEql(key, "URI")) {
                    part.uri_off = v_off;
                    part.uri_len = v_len;
                } else if (bytesEql(key, "INDEPENDENT")) {
                    part.independent = bytesEql(val, "YES");
                }
            }
            out_parts[info.part_count] = part;
            info.part_count += 1;
        } else if (bytesEql(line, "#EXT-X-ENDLIST")) {
            info.end_list = true;
        } else if (line[0] == '#') {
            continue;
        } else if (has_pending_extinf) {
            if (info.segment_count >= out_segments.len) {
                return StreamManifestError.OutputSliceTooSmall;
            }
            out_segments[info.segment_count] = HlsSegment{
                media_sequence: info.media_sequence_base + @as(u64, @intCast(info.segment_count)),
                duration_ms: pending_dur_ms,
                discontinuity: next_discontinuity,
                has_byte_range: has_pending_br,
                byte_range: pending_br,
                key_method_aes128: cur_aes128,
                uri_off: s,
                uri_len: e - s,
            };
            if (has_pending_br) {
                prev_br_end = pending_br.offset + pending_br.length;
                has_prev_br = true;
            }
            info.segment_count += 1;
            info.total_duration_ms += pending_dur_ms;
            has_pending_extinf = false;
            has_pending_br = false;
            next_discontinuity = false;
        }
    }

    if (explicit_vod || info.end_list) {
        info.kind = HlsPlaylistKind.media_vod;
    } else if (explicit_event) {
        info.kind = HlsPlaylistKind.media_event;
    } else {
        info.kind = HlsPlaylistKind.media_live;
    }

    return info;
}

/// Formats an RFC 8216 HLS Media Playlist from `info` and `segments_spec` into `out_buf`.
pub fn formatHlsMediaPlaylist(
    info: HlsMediaPlaylistInfo,
    segments_spec: []const HlsSegmentSpec,
    out_buf: *[]u8,
): usize ! StreamManifestError {
    let pos: usize = 0;
    if (!appendBytes(out_buf, *pos, "#EXTM3U\n#EXT-X-VERSION:") ||
        !appendU64Padded(out_buf, *pos, @as(u64, info.version), 0, '0') ||
        !appendBytes(out_buf, *pos, "\n#EXT-X-TARGETDURATION:") ||
        !appendU64Padded(out_buf, *pos, @as(u64, info.target_duration_sec), 0, '0') ||
        !appendBytes(out_buf, *pos, "\n#EXT-X-MEDIA-SEQUENCE:") ||
        !appendU64Padded(out_buf, *pos, info.media_sequence_base, 0, '0') ||
        !appendByte(out_buf, *pos, '\n'))
    {
        return StreamManifestError.BufferTooSmall;
    }

    if (info.kind == HlsPlaylistKind.media_vod) {
        if (!appendBytes(out_buf, *pos, "#EXT-X-PLAYLIST-TYPE:VOD\n")) {
            return StreamManifestError.BufferTooSmall;
        }
    } else if (info.kind == HlsPlaylistKind.media_event) {
        if (!appendBytes(out_buf, *pos, "#EXT-X-PLAYLIST-TYPE:EVENT\n")) {
            return StreamManifestError.BufferTooSmall;
        }
    }

    let i: usize = 0;
    while (i < segments_spec.len) : (i += 1) {
        const seg = segments_spec[i];
        if (seg.discontinuity) {
            if (!appendBytes(out_buf, *pos, "#EXT-X-DISCONTINUITY\n")) {
                return StreamManifestError.BufferTooSmall;
            }
        }
        if (seg.has_byte_range) {
            if (!appendBytes(out_buf, *pos, "#EXT-X-BYTERANGE:") ||
                !appendU64Padded(out_buf, *pos, seg.byte_range.length, 0, '0'))
            {
                return StreamManifestError.BufferTooSmall;
            }
            if (seg.byte_range.has_offset) {
                if (!appendByte(out_buf, *pos, '@') ||
                    !appendU64Padded(out_buf, *pos, seg.byte_range.offset, 0, '0'))
                {
                    return StreamManifestError.BufferTooSmall;
                }
            }
            if (!appendByte(out_buf, *pos, '\n')) return StreamManifestError.BufferTooSmall;
        }
        if (!appendBytes(out_buf, *pos, "#EXTINF:") ||
            !appendMillisAsSec(out_buf, *pos, seg.duration_ms) ||
            !appendBytes(out_buf, *pos, ",\n") ||
            !appendBytes(out_buf, *pos, seg.uri) ||
            !appendByte(out_buf, *pos, '\n'))
        {
            return StreamManifestError.BufferTooSmall;
        }
    }

    if (info.end_list) {
        if (!appendBytes(out_buf, *pos, "#EXT-X-ENDLIST\n")) {
            return StreamManifestError.BufferTooSmall;
        }
    }
    return pos;
}

// ============================================================================
// 5. MPEG-DASH SegmentTemplate URL Expander (ISO/IEC 23009-1)
// ============================================================================

/// Parameters substituted into an MPEG-DASH `SegmentTemplate` string.
pub struct DashTemplateParams {
    representation_id: []const u8 = "";
    number: u64 = 0;
    bandwidth_bps: u64 = 0;
    time: u64 = 0;
}

fn parseDashFormatTag(
    tag: []const u8,
    expected_name: []const u8,
    out_width: *usize,
    out_pad_char: *u8,
): bool {
    if (!startsWith(tag, expected_name)) return false;
    if (tag.len == expected_name.len) {
        out_width.* = 0;
        out_pad_char.* = '0';
        return true;
    }
    const fmt: []const u8 = tag[expected_name.len..tag.len];
    if (fmt.len < 2 || fmt[0] != '%' || fmt[fmt.len - 1] != 'd') {
        return false;
    }
    let idx: usize = 1;
    let pad: u8 = ' ';
    if (idx < fmt.len - 1 && fmt[idx] == '0') {
        pad = '0';
        idx += 1;
    }
    let width: usize = 0;
    while (idx < fmt.len - 1) : (idx += 1) {
        const ch: u8 = fmt[idx];
        if (ch < '0' || ch > '9') return false;
        width = width * 10 + @as(usize, ch - '0');
    }
    if (width > 20) return false;
    out_width.* = width;
    out_pad_char.* = pad;
    return true;
}

/// Expands an ISO/IEC 23009-1 MPEG-DASH `SegmentTemplate` string into `out_buf`.
/// Supports `$RepresentationID$`, `$Number[%0Nd]$`, `$Bandwidth[%0Nd]$`, `$Time[%0Nd]$`, and `$$`.
pub fn expandDashSegmentTemplate(
    template: []const u8,
    params: DashTemplateParams,
    out_buf: *[]u8,
): usize ! StreamManifestError {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < template.len) {
        if (template[i] != '$') {
            if (!appendByte(out_buf, *pos, template[i])) return StreamManifestError.BufferTooSmall;
            i += 1;
            continue;
        }
        // Found '$': locate closing '$'
        let close: usize = i + 1;
        while (close < template.len && template[close] != '$') {
            close += 1;
        }
        if (close >= template.len) {
            return StreamManifestError.InvalidTemplate;
        }
        const tag: []const u8 = template[(i + 1)..close];
        if (tag.len == 0) {
            // Literal "$$" -> "$"
            if (!appendByte(out_buf, *pos, '$')) return StreamManifestError.BufferTooSmall;
        } else if (bytesEql(tag, "RepresentationID")) {
            if (!appendBytes(out_buf, *pos, params.representation_id)) {
                return StreamManifestError.BufferTooSmall;
            }
        } else {
            let width: usize = 0;
            let pad: u8 = '0';
            if (parseDashFormatTag(tag, "Number", *width, *pad)) {
                if (!appendU64Padded(out_buf, *pos, params.number, width, pad)) {
                    return StreamManifestError.BufferTooSmall;
                }
            } else if (parseDashFormatTag(tag, "Bandwidth", *width, *pad)) {
                if (!appendU64Padded(out_buf, *pos, params.bandwidth_bps, width, pad)) {
                    return StreamManifestError.BufferTooSmall;
                }
            } else if (parseDashFormatTag(tag, "Time", *width, *pad)) {
                if (!appendU64Padded(out_buf, *pos, params.time, width, pad)) {
                    return StreamManifestError.BufferTooSmall;
                }
            } else {
                return StreamManifestError.InvalidTemplate;
            }
        }
        i = close + 1;
    }
    return pos;
}

// ============================================================================
// 6. Production Adaptive Bitrate Controller (Dual-EWMA + BOLA + Hysteresis)
// ============================================================================

/// Adaptive bitrate selection strategy.
pub enum AbrStrategy {
    throughput_ewma,
    bola_buffer,
    hybrid_safe,
}

/// Video/audio rendition quality rung in an ABR ladder.
pub struct AbrLevel {
    level_id: u16 = 0;
    bitrate_bps: u64 = 0;
    width: u32 = 0;
    height: u32 = 0;
}

/// Decision emitted by `AbrController.selectLevel`.
pub struct AbrDecision {
    selected_index: usize = 0;
    selected_bitrate_bps: u64 = 0;
    estimated_bw_bps: u64 = 0;
    reason_downgrade_panic: bool = false;
    reason_hysteresis_hold: bool = false;
}

/// Production Adaptive Bitrate (ABR) controller combining Fast/Slow Dual-EWMA throughput
/// estimation, BOLA logarithmic buffer utility, upgrade hysteresis, and low-buffer panic drop.
pub struct AbrController {
    strategy: AbrStrategy = AbrStrategy.hybrid_safe;
    safety_factor: f64 = 0.85;
    panic_buffer_ms: u64 = 2_000;
    max_buffer_ms: u64 = 20_000;
    upgrade_cooldown_ms: u64 = 4_000;

    fast_ewma_bps: f64 = 0.0;
    slow_ewma_bps: f64 = 0.0;
    has_bw_sample: bool = false;

    current_level_idx: usize = 0;
    last_switch_ms: u64 = 0;
    has_switched: bool = false;

    /// Initializes an `AbrController` with tuned production defaults.
    static fn init(
        strategy: AbrStrategy,
        safety_factor: f64,
        panic_buffer_ms: u64,
        max_buffer_ms: u64,
        upgrade_cooldown_ms: u64,
    ): AbrController ! StreamManifestError {
        if (safety_factor <= 0.0 || safety_factor > 1.0 || max_buffer_ms == 0 || panic_buffer_ms >= max_buffer_ms) {
            return StreamManifestError.InvalidConfig;
        }
        return AbrController{
            strategy: strategy,
            safety_factor: safety_factor,
            panic_buffer_ms: panic_buffer_ms,
            max_buffer_ms: max_buffer_ms,
            upgrade_cooldown_ms: upgrade_cooldown_ms,
        };
    }

    /// Feeds a completed segment download measurement into the Fast/Slow Dual-EWMA estimator.
    fn recordDownload(*_, bytes: u64, duration_ms: u64) {
        if (bytes == 0 || duration_ms == 0) return;
        const sample_bps: f64 = (@as(f64, @floatFromInt(bytes)) * 8000.0) / @as(f64, @floatFromInt(duration_ms));
        if (!has_bw_sample) {
            fast_ewma_bps = sample_bps;
            slow_ewma_bps = sample_bps;
            has_bw_sample = true;
            return;
        }
        fast_ewma_bps = 0.35 * sample_bps + 0.65 * fast_ewma_bps;
        slow_ewma_bps = 0.12 * sample_bps + 0.88 * slow_ewma_bps;
    }

    /// Returns the conservative Dual-EWMA bandwidth estimate `min(fast_ewma, slow_ewma)` in bps.
    fn estimatedBandwidthBps(): u64 {
        if (!has_bw_sample) return 0;
        const est: f64 = if (fast_ewma_bps < slow_ewma_bps) fast_ewma_bps else slow_ewma_bps;
        return if (est <= 0.0) 0 else @as(u64, @intFromFloat(est));
    }

    fn pickThroughputLevel(levels: []const AbrLevel, est_bw: u64): usize {
        const budget: u64 = @as(u64, @intFromFloat(@as(f64, @floatFromInt(est_bw)) * safety_factor));
        let best_idx: usize = 0;
        let i: usize = 0;
        while (i < levels.len) : (i += 1) {
            if (levels[i].bitrate_bps <= budget && levels[i].bitrate_bps >= levels[best_idx].bitrate_bps) {
                best_idx = i;
            }
        }
        return best_idx;
    }

    fn pickBolaLevel(levels: []const AbrLevel, buffer_level_ms: u64): usize {
        // Find min and max bitrates for BOLA V & gamma calibration (Spiteri et al., INFOCOM 2016).
        let min_idx: usize = 0;
        let max_idx: usize = 0;
        let i: usize = 1;
        while (i < levels.len) : (i += 1) {
            if (levels[i].bitrate_bps < levels[min_idx].bitrate_bps) min_idx = i;
            if (levels[i].bitrate_bps > levels[max_idx].bitrate_bps) max_idx = i;
        }
        const min_br: f64 = @as(f64, @floatFromInt(levels[min_idx].bitrate_bps));
        const max_br: f64 = @as(f64, @floatFromInt(levels[max_idx].bitrate_bps));
        if (min_br <= 0.0 || max_br <= min_br) return min_idx;

        const max_u: f64 = std.math.log(f64, std.math.e, max_br / min_br);
        const gamma_p: f64 = 5.0; // standard BOLA rebuffer-avoidance weight
        const max_buf_sec: f64 = @as(f64, @floatFromInt(max_buffer_ms)) / 1000.0;
        const v_param: f64 = (max_buf_sec - 1.0) / (max_u + gamma_p);
        const q_sec: f64 = @as(f64, @floatFromInt(buffer_level_ms)) / 1000.0;

        let best_idx: usize = min_idx;
        let best_obj: f64 = -1.0e30;
        let m: usize = 0;
        while (m < levels.len) : (m += 1) {
            const br: f64 = @as(f64, @floatFromInt(levels[m].bitrate_bps));
            if (br <= 0.0) continue;
            const u_m: f64 = std.math.log(f64, std.math.e, br / min_br);
            const obj: f64 = (v_param * (u_m + gamma_p) - q_sec) / br;
            if (obj > best_obj) {
                best_obj = obj;
                best_idx = m;
            }
        }
        return best_idx;
    }

    /// Selects the optimal ABR quality level index from `levels` (assumed sorted ascending by bitrate).
    fn selectLevel(
        *_,
        levels: []const AbrLevel,
        buffer_level_ms: u64,
        now_ms: u64,
    ): AbrDecision {
        if (levels.len == 0) {
            return AbrDecision{};
        }
        if (current_level_idx >= levels.len) {
            current_level_idx = 0;
        }

        const est_bw: u64 = estimatedBandwidthBps();

        // 1. Emergency low-buffer panic downgrade -> immediately drop to lowest rung.
        if (buffer_level_ms <= panic_buffer_ms) {
            current_level_idx = 0;
            last_switch_ms = now_ms;
            has_switched = true;
            return AbrDecision{
                selected_index: 0,
                selected_bitrate_bps: levels[0].bitrate_bps,
                estimated_bw_bps: est_bw,
                reason_downgrade_panic: true,
                reason_hysteresis_hold: false,
            };
        }

        // 2. Compute raw target index per configured ABR strategy.
        let target_idx: usize = 0;
        if (strategy == AbrStrategy.throughput_ewma) {
            target_idx = pickThroughputLevel(levels, est_bw);
        } else if (strategy == AbrStrategy.bola_buffer) {
            target_idx = pickBolaLevel(levels, buffer_level_ms);
        } else {
            const tp_idx: usize = pickThroughputLevel(levels, est_bw);
            const bola_idx: usize = pickBolaLevel(levels, buffer_level_ms);
            target_idx = if (tp_idx < bola_idx) tp_idx else bola_idx;
        }

        // 3. Upgrade hysteresis: hold current level if upgrading within cooldown window;
        //    downgrades execute immediately to prevent playback stalls.
        let held: bool = false;
        if (target_idx > current_level_idx && has_switched && now_ms < last_switch_ms + upgrade_cooldown_ms) {
            target_idx = current_level_idx;
            held = true;
        } else if (target_idx != current_level_idx || !has_switched) {
            current_level_idx = target_idx;
            last_switch_ms = now_ms;
            has_switched = true;
        }

        return AbrDecision{
            selected_index: current_level_idx,
            selected_bitrate_bps: levels[current_level_idx].bitrate_bps,
            estimated_bw_bps: est_bw,
            reason_downgrade_panic: false,
            reason_hysteresis_hold: held,
        };
    }
}
