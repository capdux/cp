// Official @web/compress Plugin for `cap:web`
// Pure-CP zero-heap Accept-Encoding negotiation & RFC 1952 Gzip stored-block encoder.

import {
    App,
    AppMethod,
    Context,
    Response,
    Plugin,
    PluginScope,
} from "../../../../lib/web/app.cp";

pub const CMP_MAX_OUT_BUF: usize = 65536;

pub struct CompressConfig {
    min_bytes: usize = 256;
    prefer_gzip: bool = true;
}

pub enum EncodingKind {
    None,
    Gzip,
    Deflate,
}

let g_cmp_cfg: CompressConfig = CompressConfig{};
let g_cmp_out_buf: [65536]u8 = [0; 65536];

fn cmpAsciiLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn cmpEqlIgnoreCase(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (cmpAsciiLower(a[i]) != cmpAsciiLower(b[i])) return false;
    }
    return true;
}

fn cmpTrimSpaces(s: string): string {
    let start: usize = 0;
    while (start < s.len && (s[start] == ' ' || s[start] == '\t')) : (start += 1) {}
    let end: usize = s.len;
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t')) : (end -= 1) {}
    return s[start..end];
}

/// Parse q-value string (e.g. "0", "0.0", "0.8", "1.0") into integer 0..1000.
fn cmpParseQValue(raw: string): i32 {
    const s = cmpTrimSpaces(raw);
    if (s.len == 0) return 1000;
    if (s[0] == '1') {
        return 1000;
    }
    if (s[0] != '0') return 0;
    if (s.len == 1) return 0;
    if (s[1] != '.') return 0;

    let scale: u32 = 100;
    let val: u32 = 0;
    let i: usize = 2;
    while (i < s.len && scale >= 1) : (i += 1) {
        const c = s[i];
        if (c < '0' || c > '9') break;
        val += ((c - '0') as u32) * scale;
        scale = scale / 10;
    }
    return val as i32;
}

/// Negotiate best supported content encoding from `Accept-Encoding` header.
/// Supports RFC 9110 quality weights and explicit `q=0` rejections.
pub fn negotiateEncoding(accept_encoding: string, prefer_gzip: bool): EncodingKind {
    if (accept_encoding.len == 0) return EncodingKind.None;

    let gzip_q: i32 = -1;
    let deflate_q: i32 = -1;
    let star_q: i32 = -1;

    let pos: usize = 0;
    while (pos < accept_encoding.len) {
        let end: usize = pos;
        while (end < accept_encoding.len && accept_encoding[end] != ',') : (end += 1) {}
        const item = cmpTrimSpaces(accept_encoding[pos..end]);
        if (item.len > 0) {
            let semi: usize = 0;
            while (semi < item.len && item[semi] != ';') : (semi += 1) {}
            const coding = cmpTrimSpaces(item[0..semi]);
            let q: i32 = 1000;
            if (semi < item.len) {
                let p_pos: usize = semi + 1;
                while (p_pos < item.len) {
                    let p_end: usize = p_pos;
                    while (p_end < item.len && item[p_end] != ';') : (p_end += 1) {}
                    const param = cmpTrimSpaces(item[p_pos..p_end]);
                    if (param.len >= 2 && (param[0] == 'q' || param[0] == 'Q')) {
                        let eq: usize = 1;
                        while (eq < param.len && (param[eq] == ' ' || param[eq] == '\t')) : (eq += 1) {}
                        if (eq < param.len && param[eq] == '=') {
                            q = cmpParseQValue(param[eq + 1 ..]);
                        }
                    }
                    if (p_end >= item.len) break;
                    p_pos = p_end + 1;
                }
            }

            if (cmpEqlIgnoreCase(coding, "gzip") || cmpEqlIgnoreCase(coding, "x-gzip")) {
                gzip_q = q;
            } else if (cmpEqlIgnoreCase(coding, "deflate")) {
                deflate_q = q;
            } else if (cmpEqlIgnoreCase(coding, "*")) {
                star_q = q;
            }
        }
        if (end >= accept_encoding.len) break;
        pos = end + 1;
    }

    const eff_gzip: i32 = if (gzip_q >= 0) gzip_q else if (star_q > 0) star_q else 0;
    const eff_deflate: i32 = if (deflate_q >= 0) deflate_q else if (star_q > 0) star_q else 0;

    if (eff_gzip <= 0 && eff_deflate <= 0) return EncodingKind.None;
    if (eff_gzip > eff_deflate) return EncodingKind.Gzip;
    if (eff_deflate > eff_gzip) return EncodingKind.Deflate;
    return if (prefer_gzip) EncodingKind.Gzip else EncodingKind.Deflate;
}

/// Pure-CP IEEE 802.3 CRC32 (polynomial 0xEDB88320).
fn cmpCrc32(data: string): u32 {
    let crc: u32 = 0xFFFFFFFF;
    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        crc = crc ^ (data[i] as u32);
        let bit: u32 = 0;
        while (bit < 8) : (bit += 1) {
            if ((crc & 1) != 0) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc = crc >> 1;
            }
        }
    }
    return crc ^ 0xFFFFFFFF;
}

fn cmpWriteU16Le(out_buf: []u8, pos: usize, val: u16) {
    out_buf[pos] = (val & 0xFF) as u8;
    out_buf[pos + 1] = ((val >> 8) & 0xFF) as u8;
}

fn cmpWriteU32Le(out_buf: []u8, pos: usize, val: u32) {
    out_buf[pos] = (val & 0xFF) as u8;
    out_buf[pos + 1] = ((val >> 8) & 0xFF) as u8;
    out_buf[pos + 2] = ((val >> 16) & 0xFF) as u8;
    out_buf[pos + 3] = ((val >> 24) & 0xFF) as u8;
}

/// Pure-CP zero-heap RFC 1952 Gzip frame encoder using RFC 1951 Stored Deflate blocks.
/// Output is 100% compliant with `gzip -d` and all browsers.
/// Returns total encoded bytes written into `out_buf`, or `null` if `out_buf` is too small.
pub fn encodeGzipStored(src: string, out_buf: []u8): usize? {
    // Calculate required size: 10B gzip header + N * 5B block headers + src.len + 8B trailer
    const max_block: usize = 65535;
    const num_blocks: usize = if (src.len == 0) 1 else ((src.len + max_block - 1) / max_block);
    const needed: usize = src.len + num_blocks * 5 + 18;
    if (out_buf.len < needed) return null;

    // 1. RFC 1952 10-byte Gzip Header
    out_buf[0] = 0x1F; // ID1
    out_buf[1] = 0x8B; // ID2
    out_buf[2] = 0x08; // CM = deflate
    out_buf[3] = 0x00; // FLG = none
    out_buf[4] = 0x00; // MTIME[0..3]
    out_buf[5] = 0x00;
    out_buf[6] = 0x00;
    out_buf[7] = 0x00;
    out_buf[8] = 0x00; // XFL
    out_buf[9] = 0xFF; // OS = unknown

    let pos: usize = 10;

    // 2. RFC 1951 Stored Deflate Blocks
    if (src.len == 0) {
        out_buf[pos] = 0x01; // BFINAL=1, BTYPE=00
        cmpWriteU16Le(out_buf, pos + 1, 0);
        cmpWriteU16Le(out_buf, pos + 3, 0xFFFF);
        pos += 5;
    } else {
        let offset: usize = 0;
        while (offset < src.len) {
            const rem = src.len - offset;
            const chunk_len: usize = if (rem > max_block) max_block else rem;
            const is_final: bool = (offset + chunk_len) >= src.len;
            const len16: u16 = chunk_len as u16;
            const nlen16: u16 = ~len16;

            out_buf[pos] = if (is_final) 0x01 else 0x00;
            cmpWriteU16Le(out_buf, pos + 1, len16);
            cmpWriteU16Le(out_buf, pos + 3, nlen16);
            pos += 5;

            let j: usize = 0;
            while (j < chunk_len) : (j += 1) {
                out_buf[pos + j] = src[offset + j];
            }
            pos += chunk_len;
            offset += chunk_len;
        }
    }

    // 3. RFC 1952 8-byte Trailer: IEEE CRC32 + ISIZE (mod 2^32)
    const crc = cmpCrc32(src);
    const isize32: u32 = (src.len & 0xFFFFFFFF) as u32;
    cmpWriteU32Le(out_buf, pos, crc);
    cmpWriteU32Le(out_buf, pos + 4, isize32);
    pos += 8;

    return pos;
}

/// `onAfterHandle` response hook: attaches `Vary: Accept-Encoding` and `Content-Encoding: gzip`
/// and encodes the payload when body length >= `min_bytes` and the client accepts `gzip`.
pub fn compressResponseHook(ctx: *Context, resp: *Response) {
    if (resp.header("Content-Encoding") != null) return;

    const body: string = if (resp.body_buf_len > 0)
        resp.body_buf[0..resp.body_buf_len]
    else
        resp.body;

    if (body.len < g_cmp_cfg.min_bytes) return;

    const ae = ctx.header("Accept-Encoding") ?? return;
    const chosen = negotiateEncoding(ae, g_cmp_cfg.prefer_gzip);
    if (chosen != EncodingKind.Gzip) return;

    * = resp.setHeader("Vary", "Accept-Encoding");
    * = resp.setHeader("Content-Encoding", "gzip");

    if (written := encodeGzipStored(body, g_cmp_out_buf[0..])) {
        resp.body = g_cmp_out_buf[0..written];
        resp.body_buf_len = 0;
    }
}

/// Build a reusable `cap:web` `Plugin` for `@web/compress`.
pub fn createCompressPlugin(cfg: CompressConfig): Plugin {
    g_cmp_cfg = cfg;
    let p = Plugin.init("@web/compress");
    * = p.withScope(PluginScope.Global).onAfterHandle(compressResponseHook);
    return p;
}

/// One-call registration onto a `cap:web` `App`.
pub fn registerCompress(app: *App, cfg: CompressConfig): *App {
    const p = createCompressPlugin(cfg);
    return app.usePlugin(&p);
}
