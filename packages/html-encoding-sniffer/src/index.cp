// @cap/html-encoding-sniffer — Zero-allocation WHATWG HTML §13.2.3.2 encoding sniffer
// Clean-room MIT implementation inspired by jsdom html-encoding-sniffer

import { findByteInBytes } from "cap:bin";

pub struct SniffOptions {
    transport_layer_encoding: string,
    default_encoding: string,
}

type HesVec16 = @Vector(16, u8);
type HesBool16 = @Vector(16, bool);

const HES_FLAG_WS: u8 = 1;
const HES_FLAG_ATTR_STOP: u8 = 2;

fn hesBuildLowerLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    let c: usize = 0;
    while (c < 256) {
        lut[c] = if (c >= 65 && c <= 90) ((c + 32) as u8) else (c as u8);
        c += 1;
    }
    return lut;
}

fn hesBuildFlagsLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    lut[0x09] = HES_FLAG_WS | HES_FLAG_ATTR_STOP;
    lut[0x0A] = HES_FLAG_WS | HES_FLAG_ATTR_STOP;
    lut[0x0C] = HES_FLAG_WS | HES_FLAG_ATTR_STOP;
    lut[0x0D] = HES_FLAG_WS | HES_FLAG_ATTR_STOP;
    lut[0x20] = HES_FLAG_WS | HES_FLAG_ATTR_STOP;
    lut['=' as usize] = HES_FLAG_ATTR_STOP;
    lut['>' as usize] = HES_FLAG_ATTR_STOP;
    lut['/' as usize] = HES_FLAG_ATTR_STOP;
    return lut;
}

const HES_LOWER_LUT: [256]u8 = hesBuildLowerLut();
const HES_FLAGS_LUT: [256]u8 = hesBuildFlagsLut();

fn hesToLower(ch: u8): u8 {
    return HES_LOWER_LUT[ch as usize];
}

fn hesEqIgnoreCase(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    const len = a.len;
    const v_a: HesVec16 = @splat(65 as u8);
    const v_z: HesVec16 = @splat(90 as u8);
    const v_bit: HesVec16 = @splat(32 as u8);
    const v_false: HesBool16 = @splat(false);
    while (i + 16 <= len) {
        const va: HesVec16 = a[i .. i + 16][0..16].*;
        const vb: HesVec16 = b[i .. i + 16][0..16].*;
        const la: HesVec16 = @select(u8, @select(bool, va >= v_a, va <= v_z, v_false), va | v_bit, va);
        const lb: HesVec16 = @select(u8, @select(bool, vb >= v_a, vb <= v_z, v_false), vb | v_bit, vb);
        if (@reduce(.Or, la != lb)) return false;
        i += 16;
    }
    while (i < len) : (i += 1) {
        if (HES_LOWER_LUT[a[i] as usize] != HES_LOWER_LUT[b[i] as usize]) return false;
    }
    return true;
}

fn hesIsAsciiWs(ch: u8): bool {
    return (HES_FLAGS_LUT[ch as usize] & HES_FLAG_WS) != 0;
}

fn hesFindByte(buf: []const u8, target: u8, start: usize): usize {
    let i: usize = start;
    const len = buf.len;
    const vt: HesVec16 = @splat(target);
    while (i + 16 <= len) {
        const v: HesVec16 = buf[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == vt)) {
            let k: usize = 0;
            while (k < 16) {
                if (buf[i + k] == target) return i + k;
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len) {
        if (buf[i] == target) return i;
        i += 1;
    }
    return len;
}

fn hesFindTagSkip(buf: []const u8, start: usize): usize {
    let i: usize = start;
    const len = buf.len;
    const v_gt: HesVec16 = @splat('>' as u8);
    const v_dq: HesVec16 = @splat('"' as u8);
    const v_sq: HesVec16 = @splat('\'' as u8);
    while (i + 16 <= len) {
        const v: HesVec16 = buf[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_gt) || @reduce(.Or, v == v_dq) || @reduce(.Or, v == v_sq)) {
            let k: usize = 0;
            while (k < 16) {
                const c = buf[i + k];
                if (c == '>' || c == '"' || c == '\'') return i + k;
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len) {
        const c = buf[i];
        if (c == '>' || c == '"' || c == '\'') return i;
        i += 1;
    }
    return len;
}

fn hesTrimAsciiWs(s: string): string {
    let start: usize = 0;
    while (start < s.len && hesIsAsciiWs(s[start])) {
        start += 1;
    }
    let end = s.len;
    while (end > start && hesIsAsciiWs(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn hesStartsWithIgnoreCase(bytes: []const u8, pos: usize, needle: string): bool {
    if (pos + needle.len > bytes.len) return false;
    return hesEqIgnoreCase(bytes[pos..pos + needle.len], needle);
}

pub fn normalizeEncodingLabel(label: string): string {
    const trimmed = hesTrimAsciiWs(label);
    if (trimmed.len == 0) return "";

    if (hesEqIgnoreCase(trimmed, "utf-8") ||
        hesEqIgnoreCase(trimmed, "utf8") ||
        hesEqIgnoreCase(trimmed, "unicode-1-1-utf-8") ||
        hesEqIgnoreCase(trimmed, "unicode11utf8") ||
        hesEqIgnoreCase(trimmed, "unicode20utf8") ||
        hesEqIgnoreCase(trimmed, "x-unicode20utf8")) {
        return "UTF-8";
    }

    if (hesEqIgnoreCase(trimmed, "windows-1252") ||
        hesEqIgnoreCase(trimmed, "latin1") ||
        hesEqIgnoreCase(trimmed, "l1") ||
        hesEqIgnoreCase(trimmed, "ascii") ||
        hesEqIgnoreCase(trimmed, "us-ascii") ||
        hesEqIgnoreCase(trimmed, "iso-8859-1") ||
        hesEqIgnoreCase(trimmed, "iso8859-1") ||
        hesEqIgnoreCase(trimmed, "iso88591") ||
        hesEqIgnoreCase(trimmed, "iso_8859-1") ||
        hesEqIgnoreCase(trimmed, "cp1252") ||
        hesEqIgnoreCase(trimmed, "x-cp1252") ||
        hesEqIgnoreCase(trimmed, "ansi_x3.4-1968") ||
        hesEqIgnoreCase(trimmed, "cp819") ||
        hesEqIgnoreCase(trimmed, "csisolatin1") ||
        hesEqIgnoreCase(trimmed, "ibm819")) {
        return "windows-1252";
    }

    if (hesEqIgnoreCase(trimmed, "gbk") ||
        hesEqIgnoreCase(trimmed, "gb2312") ||
        hesEqIgnoreCase(trimmed, "gb_2312") ||
        hesEqIgnoreCase(trimmed, "gb_2312-80") ||
        hesEqIgnoreCase(trimmed, "chinese") ||
        hesEqIgnoreCase(trimmed, "csgb2312") ||
        hesEqIgnoreCase(trimmed, "csiso58gb231280") ||
        hesEqIgnoreCase(trimmed, "iso-ir-58") ||
        hesEqIgnoreCase(trimmed, "x-gbk")) {
        return "GBK";
    }

    if (hesEqIgnoreCase(trimmed, "gb18030")) {
        return "gb18030";
    }

    if (hesEqIgnoreCase(trimmed, "big5") ||
        hesEqIgnoreCase(trimmed, "big5-hkscs") ||
        hesEqIgnoreCase(trimmed, "cn-big5") ||
        hesEqIgnoreCase(trimmed, "csbig5") ||
        hesEqIgnoreCase(trimmed, "x-x-big5")) {
        return "Big5";
    }

    if (hesEqIgnoreCase(trimmed, "utf-16be") || hesEqIgnoreCase(trimmed, "utf16be")) {
        return "UTF-16BE";
    }

    if (hesEqIgnoreCase(trimmed, "utf-16le") ||
        hesEqIgnoreCase(trimmed, "utf-16") ||
        hesEqIgnoreCase(trimmed, "utf16") ||
        hesEqIgnoreCase(trimmed, "utf16le") ||
        hesEqIgnoreCase(trimmed, "ucs-2") ||
        hesEqIgnoreCase(trimmed, "unicode") ||
        hesEqIgnoreCase(trimmed, "unicodefeff")) {
        return "UTF-16LE";
    }

    if (hesEqIgnoreCase(trimmed, "shift_jis") ||
        hesEqIgnoreCase(trimmed, "shift-jis") ||
        hesEqIgnoreCase(trimmed, "sjis") ||
        hesEqIgnoreCase(trimmed, "csshiftjis") ||
        hesEqIgnoreCase(trimmed, "ms932") ||
        hesEqIgnoreCase(trimmed, "ms_kanji") ||
        hesEqIgnoreCase(trimmed, "windows-31j") ||
        hesEqIgnoreCase(trimmed, "x-sjis")) {
        return "Shift_JIS";
    }

    if (hesEqIgnoreCase(trimmed, "euc-jp") ||
        hesEqIgnoreCase(trimmed, "cseucpkdfmtjapanese") ||
        hesEqIgnoreCase(trimmed, "x-euc-jp")) {
        return "EUC-JP";
    }

    if (hesEqIgnoreCase(trimmed, "euc-kr") ||
        hesEqIgnoreCase(trimmed, "cseuckr") ||
        hesEqIgnoreCase(trimmed, "csksc56011987") ||
        hesEqIgnoreCase(trimmed, "iso-ir-149") ||
        hesEqIgnoreCase(trimmed, "korean") ||
        hesEqIgnoreCase(trimmed, "ks_c_5601-1987") ||
        hesEqIgnoreCase(trimmed, "ks_c_5601-1989") ||
        hesEqIgnoreCase(trimmed, "ksc5601") ||
        hesEqIgnoreCase(trimmed, "ksc_5601") ||
        hesEqIgnoreCase(trimmed, "windows-949")) {
        return "EUC-KR";
    }

    if (hesEqIgnoreCase(trimmed, "iso-8859-2") ||
        hesEqIgnoreCase(trimmed, "iso8859-2") ||
        hesEqIgnoreCase(trimmed, "iso88592") ||
        hesEqIgnoreCase(trimmed, "latin2") ||
        hesEqIgnoreCase(trimmed, "l2")) {
        return "ISO-8859-2";
    }

    if (hesEqIgnoreCase(trimmed, "windows-1250") ||
        hesEqIgnoreCase(trimmed, "cp1250") ||
        hesEqIgnoreCase(trimmed, "x-cp1250")) {
        return "windows-1250";
    }

    if (hesEqIgnoreCase(trimmed, "windows-1251") ||
        hesEqIgnoreCase(trimmed, "cp1251") ||
        hesEqIgnoreCase(trimmed, "x-cp1251")) {
        return "windows-1251";
    }

    if (hesEqIgnoreCase(trimmed, "koi8-r") ||
        hesEqIgnoreCase(trimmed, "cskoi8r") ||
        hesEqIgnoreCase(trimmed, "koi") ||
        hesEqIgnoreCase(trimmed, "koi8") ||
        hesEqIgnoreCase(trimmed, "koi8_r")) {
        return "KOI8-R";
    }

    return "";
}

fn hesMapMetaEncoding(label: string): string {
    const norm = normalizeEncodingLabel(label);
    if (norm.len == 0) return "";
    // WHATWG HTML §13.2.3.2: UTF-16BE / UTF-16LE declared in <meta> map to UTF-8
    if (hesEqIgnoreCase(norm, "UTF-16BE") || hesEqIgnoreCase(norm, "UTF-16LE")) {
        return "UTF-8";
    }
    return norm;
}

fn hesExtractCharsetFromContent(content: string): string {
    let i: usize = 0;
    while (i + 7 <= content.len) : (i += 1) {
        if (hesStartsWithIgnoreCase(content, i, "charset")) {
            let p = i + 7;
            while (p < content.len && hesIsAsciiWs(content[p])) {
                p += 1;
            }
            if (p >= content.len || content[p] != '=') {
                continue;
            }
            p += 1;
            while (p < content.len && hesIsAsciiWs(content[p])) {
                p += 1;
            }
            if (p >= content.len) return "";

            if (content[p] == '"' || content[p] == '\'') {
                const q = content[p];
                p += 1;
                if (end_q := findByteInBytes(content, q, p)) {
                    return content[p..end_q];
                }
                return "";
            }
            const start = p;
            while (p < content.len && !hesIsAsciiWs(content[p]) && content[p] != ';') {
                p += 1;
            }
            return content[start..p];
        }
    }
    return "";
}

fn hesPrescanMeta(bytes: []const u8): string {
    const limit = if (bytes.len > 1024) 1024 else bytes.len;
    const head = bytes[0..limit];
    let pos: usize = 0;

    while (pos < limit) {
        const lt = findByteInBytes(head, '<', pos) ?? break;
        pos = lt;

        // Skip HTML comments: <!-- ... --> (SIMD '-' search)
        if (pos + 4 <= limit && head[pos + 1] == '!' && head[pos + 2] == '-' && head[pos + 3] == '-') {
            pos += 4;
            let closed = false;
            while (pos + 2 < limit) {
                pos = hesFindByte(head, '-', pos);
                if (pos + 2 >= limit) break;
                if (head[pos + 1] == '-' && head[pos + 2] == '>') {
                    pos += 3;
                    closed = true;
                    break;
                }
                pos += 1;
            }
            if (!closed) break;
            continue;
        }

        // Check <meta followed by ASCII whitespace or '/'
        if (pos + 6 <= limit && hesStartsWithIgnoreCase(head, pos, "<meta")) {
            const after = head[pos + 5];
            if (hesIsAsciiWs(after) || after == '/') {
                pos += 5;
                let charset_val: string = "";
                let http_equiv_val: string = "";
                let content_val: string = "";

                while (pos < limit) {
                    while (pos < limit && (hesIsAsciiWs(head[pos]) || head[pos] == '/')) {
                        pos += 1;
                    }
                    if (pos >= limit) break;
                    if (head[pos] == '>') {
                        pos += 1;
                        break;
                    }

                    // Parse attribute name (LUT single-lookup)
                    const n_start = pos;
                    while (pos < limit && (HES_FLAGS_LUT[head[pos] as usize] & HES_FLAG_ATTR_STOP) == 0) {
                        pos += 1;
                    }
                    const attr_name = head[n_start..pos];

                    while (pos < limit && hesIsAsciiWs(head[pos])) {
                        pos += 1;
                    }

                    let attr_val: string = "";
                    if (pos < limit && head[pos] == '=') {
                        pos += 1;
                        while (pos < limit && hesIsAsciiWs(head[pos])) {
                            pos += 1;
                        }
                        if (pos < limit) {
                            if (head[pos] == '"' || head[pos] == '\'') {
                                const q = head[pos];
                                pos += 1;
                                const v_start = pos;
                                pos = findByteInBytes(head, q, pos) ?? limit;
                                attr_val = head[v_start..pos];
                                if (pos < limit && head[pos] == q) {
                                    pos += 1;
                                }
                            } else {
                                const v_start = pos;
                                while (pos < limit && !hesIsAsciiWs(head[pos]) && head[pos] != '>') {
                                    pos += 1;
                                }
                                attr_val = head[v_start..pos];
                            }
                        }
                    }

                    if (hesEqIgnoreCase(attr_name, "charset") && charset_val.len == 0) {
                        charset_val = attr_val;
                    } else if (hesEqIgnoreCase(attr_name, "http-equiv") && http_equiv_val.len == 0) {
                        http_equiv_val = attr_val;
                    } else if (hesEqIgnoreCase(attr_name, "content") && content_val.len == 0) {
                        content_val = attr_val;
                    }
                }

                if (charset_val.len > 0) {
                    const mapped = hesMapMetaEncoding(charset_val);
                    if (mapped.len > 0) return mapped;
                }
                if (hesEqIgnoreCase(hesTrimAsciiWs(http_equiv_val), "content-type") && content_val.len > 0) {
                    const extracted = hesExtractCharsetFromContent(content_val);
                    if (extracted.len > 0) {
                        const mapped = hesMapMetaEncoding(extracted);
                        if (mapped.len > 0) return mapped;
                    }
                }
                continue;
            }
        }

        // Skip other tags safely (respecting quoted attributes, SIMD accelerated)
        pos += 1;
        while (pos < limit) {
            pos = hesFindTagSkip(head, pos);
            if (pos >= limit) break;
            if (head[pos] == '>') {
                pos += 1;
                break;
            }
            const q = head[pos];
            pos = hesFindByte(head, q, pos + 1);
            if (pos < limit) pos += 1;
        }
    }

    return "";
}

pub fn sniffHtmlEncoding(bytes: []const u8, opts: SniffOptions): string {
    // 1. BOM sniffing (highest priority per WHATWG)
    if (bytes.len >= 3 && bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF) {
        return "UTF-8";
    }
    if (bytes.len >= 2 && bytes[0] == 0xFE && bytes[1] == 0xFF) {
        return "UTF-16BE";
    }
    if (bytes.len >= 2 && bytes[0] == 0xFF && bytes[1] == 0xFE) {
        return "UTF-16LE";
    }

    // 2. Transport layer encoding (e.g. HTTP Content-Type charset)
    if (opts.transport_layer_encoding.len > 0) {
        const norm = normalizeEncodingLabel(opts.transport_layer_encoding);
        if (norm.len > 0) return norm;
    }

    // 3. Prescan first 1024 bytes for <meta> charset declarations
    const meta_enc = hesPrescanMeta(bytes);
    if (meta_enc.len > 0) return meta_enc;

    // 4. Fallback to default_encoding (or "windows-1252")
    if (opts.default_encoding.len > 0) {
        const def_norm = normalizeEncodingLabel(opts.default_encoding);
        if (def_norm.len > 0) return def_norm;
        return opts.default_encoding;
    }
    return "windows-1252";
}
