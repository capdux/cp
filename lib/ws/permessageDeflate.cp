// Source: cp/lib/ws/permessage_deflate.cp (RFC 7692 permessage-deflate)
// lib/ws/permessage_deflate.cp
// RFC 7692 WebSocket permessage-deflate Extension Negotiation & Payload Codec

const std = @import("std");
import {
    NativeZlib,
    ZStream,
    zlibVersion,
    deflateRaw,
    inflateRaw,
} from "cap:pack";
import { WsExtensionHeader } from "./subprotocol";

pub const PMD_SYNC_TRAILER: [4]u8 = [0x00, 0x00, 0xFF, 0xFF];

pub struct PerMessageDeflateConfig {
    server_no_context_takeover: bool = false,
    client_no_context_takeover: bool = false,
    server_max_window_bits: u8 = 15,
    client_max_window_bits: u8 = 15,
    compression_threshold: usize = 128,
    compression_level: i32 = 6,
    max_decompressed_bytes: usize = 16 * 1024 * 1024,

    pub fn shouldCompress(payload_len: usize): bool {
        return payload_len >= _.compression_threshold;
    }

    pub fn isValidWindowBits(bits: u8): bool {
        return bits >= 8 && bits <= 15;
    }
}

pub struct PerMessageDeflateNegotiation {
    accepted: bool = false,
    config: PerMessageDeflateConfig = PerMessageDeflateConfig{},
    response_header_buf: [256]u8 = undefined,
    response_header_len: usize = 0,

    pub fn responseHeader(*_): []const u8 {
        return _.response_header_buf[0.._.response_header_len];
    }
}

pub struct PerMessageDeflate {
    config: PerMessageDeflateConfig = PerMessageDeflateConfig{},
    comp_strm: ZStream = undefined,
    decomp_strm: ZStream = undefined,
    comp_active: bool = false,
    decomp_active: bool = false,

    static fn newZeroZStream(): ZStream {
        return ZStream{
            next_in: null,
            avail_in: 0,
            total_in: 0,
            next_out: null,
            avail_out: 0,
            total_out: 0,
            msg: null,
            state: null,
            zalloc: null,
            zfree: null,
            opaque_ctx: null,
            data_type: 0,
            adler: 0,
            reserved: 0,
        };
    }

    static fn parseWindowBitsParam(val: []const u8): u8? {
        if (val.len == 0 || val.len > 2) return null;
        if (val.len == 2 && val[0] == '0') return null;
        let parsed: u8 = 0;
        let i: usize = 0;
        while (i < val.len) : (i += 1) {
            const c = val[i];
            if (c < '0' || c > '9') return null;
            parsed = parsed * @as(u8, 10) + (c - @as(u8, '0'));
        }
        if (parsed < 8 || parsed > 15) return null;
        return parsed;
    }

    static fn formatU8WindowBits(bits: u8, buf: []u8): []const u8 {
        if (bits >= 10) {
            buf[0] = @as(u8, '0') + (bits / @as(u8, 10));
            buf[1] = @as(u8, '0') + (bits % @as(u8, 10));
            return buf[0..2];
        }
        buf[0] = @as(u8, '0') + bits;
        return buf[0..1];
    }

    static fn tryNegotiateSingleOffer(
        ext: WsExtensionHeader,
        server_prefs: PerMessageDeflateConfig,
    ): PerMessageDeflateNegotiation? {
        if (!std.ascii.eqlIgnoreCase(ext.name, "permessage-deflate")) {
            return null;
        }

        let neg_cfg = PerMessageDeflateConfig{
            server_no_context_takeover: server_prefs.server_no_context_takeover,
            client_no_context_takeover: server_prefs.client_no_context_takeover,
            server_max_window_bits: if (server_prefs.server_max_window_bits >= 8 && server_prefs.server_max_window_bits <= 15) server_prefs.server_max_window_bits else 15,
            client_max_window_bits: 15,
            compression_threshold: server_prefs.compression_threshold,
            compression_level: server_prefs.compression_level,
            max_decompressed_bytes: server_prefs.max_decompressed_bytes,
        };

        let seen_snc = false;
        let seen_cnc = false;
        let seen_smw = false;
        let seen_cmw = false;
        let client_supports_cmw = false;
        let client_explicit_cmw: u8? = null;
        let client_explicit_smw: u8? = null;

        let i: usize = 0;
        while (i < ext.param_count) : (i += 1) {
            const p = ext.params[i];
            if (std.ascii.eqlIgnoreCase(p.name, "server_no_context_takeover")) {
                if (seen_snc || p.value != null) return null;
                seen_snc = true;
                neg_cfg.server_no_context_takeover = true;
            } else if (std.ascii.eqlIgnoreCase(p.name, "client_no_context_takeover")) {
                if (seen_cnc || p.value != null) return null;
                seen_cnc = true;
                neg_cfg.client_no_context_takeover = true;
            } else if (std.ascii.eqlIgnoreCase(p.name, "server_max_window_bits")) {
                if (seen_smw) return null;
                seen_smw = true;
                const val = p.value ?? return null;
                const bits = PerMessageDeflate.parseWindowBitsParam(val) ?? return null;
                client_explicit_smw = bits;
                if (bits < neg_cfg.server_max_window_bits) {
                    neg_cfg.server_max_window_bits = bits;
                }
            } else if (std.ascii.eqlIgnoreCase(p.name, "client_max_window_bits")) {
                if (seen_cmw) return null;
                seen_cmw = true;
                client_supports_cmw = true;
                if (val := p.value) {
                    const bits = PerMessageDeflate.parseWindowBitsParam(val) ?? return null;
                    client_explicit_cmw = bits;
                    neg_cfg.client_max_window_bits = bits;
                }
            } else {
                // Unknown extension parameter must reject this offer per RFC 7692 §7.1
                return null;
            }
        }

        if (client_supports_cmw && server_prefs.client_max_window_bits >= 8 && server_prefs.client_max_window_bits <= 15) {
            if (server_prefs.client_max_window_bits < neg_cfg.client_max_window_bits) {
                neg_cfg.client_max_window_bits = server_prefs.client_max_window_bits;
            }
        }

        let resp_ext = WsExtensionHeader.init("permessage-deflate");
        if (neg_cfg.server_no_context_takeover) {
            resp_ext.addParam("server_no_context_takeover", null) catch return null;
        }
        if (neg_cfg.client_no_context_takeover) {
            resp_ext.addParam("client_no_context_takeover", null) catch return null;
        }

        let smw_buf: [4]u8 = undefined;
        let smw_val_slice: []const u8 = "";
        if (client_explicit_smw != null || neg_cfg.server_max_window_bits < 15) {
            smw_val_slice = PerMessageDeflate.formatU8WindowBits(neg_cfg.server_max_window_bits, &smw_buf);
            resp_ext.addParam("server_max_window_bits", smw_val_slice) catch return null;
        }

        let cmw_buf: [4]u8 = undefined;
        let cmw_val_slice: []const u8 = "";
        if (client_supports_cmw && (client_explicit_cmw != null || neg_cfg.client_max_window_bits < 15)) {
            cmw_val_slice = PerMessageDeflate.formatU8WindowBits(neg_cfg.client_max_window_bits, &cmw_buf);
            resp_ext.addParam("client_max_window_bits", cmw_val_slice) catch return null;
        }

        let res = PerMessageDeflateNegotiation{
            accepted: true,
            config: neg_cfg,
            response_header_buf: undefined,
            response_header_len: 0,
        };
        const written = resp_ext.serialize(&res.response_header_buf) catch return null;
        res.response_header_len = written.len;
        return res;
    }

    /// Negotiates `permessage-deflate` from a client `Sec-WebSocket-Extensions` header using default server config.
    pub static fn negotiate(client_ext_header: []const u8): PerMessageDeflateNegotiation! {
        return PerMessageDeflate.negotiateWithConfig(client_ext_header, PerMessageDeflateConfig{});
    }

    /// Negotiates `permessage-deflate` against explicit server configuration preferences (RFC 7692 §7.1).
    pub static fn negotiateWithConfig(
        client_ext_header: []const u8,
        server_prefs: PerMessageDeflateConfig,
    ): PerMessageDeflateNegotiation! {
        let offers = WsExtensionHeader.parseAll(client_ext_header)?;
        let i: usize = 0;
        while (i < offers.len) : (i += 1) {
            if (accepted := PerMessageDeflate.tryNegotiateSingleOffer(offers.items[i], server_prefs)) {
                return accepted;
            }
        }
        return PerMessageDeflateNegotiation{
            accepted: false,
            config: server_prefs,
            response_header_buf: undefined,
            response_header_len: 0,
        };
    }

    /// Returns true if `data` ends with the RFC 7692 `0x00 0x00 0xFF 0xFF` sync flush trailer.
    pub static fn hasSyncTrailer(data: []const u8): bool {
        if (data.len < 4) return false;
        const tail = data[data.len - 4 ..];
        return tail[0] == 0x00 && tail[1] == 0x00 && tail[2] == 0xFF && tail[3] == 0xFF;
    }

    /// Strips the trailing `0x00 0x00 0xFF 0xFF` sync flush marker if present (RFC 7692 §7.2.1).
    pub static fn stripSyncTrailer(data: []const u8): []const u8 {
        if (PerMessageDeflate.hasSyncTrailer(data)) {
            return data[0 .. data.len - 4];
        }
        return data;
    }

    static fn initDeflateRawStream(strm: *ZStream, level: i32, window_bits: u8): ! {
        let wb: i32 = if (window_bits < 9) 9 else if (window_bits > 15) 15 else (window_bits as i32);
        const rc = NativeZlib.deflateInit2_(
            strm,
            level,
            8, // Z_DEFLATED
            -wb,
            8,
            0,
            zlibVersion(),
            @sizeOf(ZStream) as i32,
        );
        if (rc != 0) return err.DeflateInitFailed;
    }

    static fn initInflateRawStream(strm: *ZStream, window_bits: u8): ! {
        let wb: i32 = if (window_bits < 8) 15 else if (window_bits > 15) 15 else (window_bits as i32);
        // zlib inflateInit2_ with -15 decodes any raw deflate window size <= 15.
        if (wb < 15) wb = 15;
        const rc = NativeZlib.inflateInit2_(
            strm,
            -wb,
            zlibVersion(),
            @sizeOf(ZStream) as i32,
        );
        if (rc != 0) return err.InflateInitFailed;
    }

    static fn compressOnStream(strm: *ZStream, payload: []const u8): ([]u8)! {
        if (payload.len == 0) {
            // RFC 7692 §7.2.3.6: empty payload -> single 0x00 octet (after stripping 00 00 FF FF)
            let empty_raw: [*]u8 = @alloc(1);
            let empty_out = unsafe { empty_raw[0..1] };
            empty_out[0] = 0x00;
            return empty_out;
        }

        const bound: usize = NativeZlib.deflateBound(strm, payload.len) + 64;
        let out_buf: [*]u8 = @alloc(bound);
        strm.next_in = payload.ptr;
        strm.avail_in = payload.len as u32;
        strm.next_out = out_buf;
        strm.avail_out = bound as u32;

        const base_total: usize = strm.total_out;
        while (true) {
            const rc = NativeZlib.deflate(strm, 2); // Z_SYNC_FLUSH
            if (rc != 0 && rc != 1) {
                return err.DeflateFailed;
            }
            if (strm.avail_in == 0 && strm.avail_out > 0) {
                break;
            }
            if (strm.avail_out == 0) {
                return err.DeflateBufferExhausted;
            }
        }

        const produced: usize = strm.total_out - base_total;
        const raw_slice = unsafe { out_buf[0..produced] };
        if (PerMessageDeflate.hasSyncTrailer(raw_slice)) {
            return raw_slice[0 .. produced - 4];
        }
        return raw_slice;
    }

    static fn decompressOnStream(strm: *ZStream, compressed: []const u8, max_bytes: usize): ([]u8)! {
        if (compressed.len == 0) {
            return err.InflateFailed;
        }

        // RFC 7692 §7.2.2: append 0x00 0x00 0xFF 0xFF to tail before inflate
        const in_len: usize = compressed.len + 4;
        let in_raw: [*]u8 = @alloc(in_len);
        let in_buf = unsafe { in_raw[0..in_len] };
        @memcpy(in_buf[0..compressed.len], compressed);
        in_buf[compressed.len] = 0x00;
        in_buf[compressed.len + 1] = 0x00;
        in_buf[compressed.len + 2] = 0xFF;
        in_buf[compressed.len + 3] = 0xFF;

        strm.next_in = in_raw;
        strm.avail_in = in_len as u32;

        let cur_cap: usize = compressed.len * (4 as usize) + (256 as usize);
        if (cur_cap > max_bytes && max_bytes > (0 as usize)) {
            cur_cap = max_bytes;
        }
        if (cur_cap < (64 as usize)) cur_cap = 64 as usize;

        let out_buf: [*]u8 = @alloc(cur_cap);
        strm.next_out = out_buf;
        strm.avail_out = cur_cap as u32;

        const base_total: usize = strm.total_out;
        while (true) {
            const rc = NativeZlib.inflate(strm, 2); // Z_SYNC_FLUSH
            const produced: usize = strm.total_out - base_total;
            if (produced > max_bytes) {
                return err.MessageTooBig;
            }
            if (rc == 1) { // Z_STREAM_END
                return unsafe { out_buf[0..produced] };
            }
            if (rc != 0) {
                return err.InflateFailed;
            }
            if (strm.avail_in == 0 && strm.avail_out > 0) {
                return unsafe { out_buf[0..produced] };
            }
            if (strm.avail_out == 0) {
                if (cur_cap >= max_bytes) {
                    return err.MessageTooBig;
                }
                let next_cap: usize = cur_cap + cur_cap;
                if (next_cap > max_bytes) {
                    next_cap = max_bytes;
                }
                if (next_cap <= cur_cap) {
                    return err.MessageTooBig;
                }
                let new_buf: [*]u8 = @alloc(next_cap);
                unsafe { @memcpy(new_buf[0..produced], out_buf[0..produced]); }
                out_buf = new_buf;
                cur_cap = next_cap;
                strm.next_out = unsafe { &out_buf[produced] };
                strm.avail_out = (next_cap - produced) as u32;
            }
        }
    }

    /// Stateless RFC 7692 §7.2.1 payload compression: raw deflate with `Z_SYNC_FLUSH` and
    /// automatic stripping of the trailing `0x00 0x00 0xFF 0xFF` flush marker.
    pub static fn compressPayload(payload: []const u8, level: i32 = 6, window_bits: u8 = 15): ([]u8)! {
        if (payload.len == 0) {
            let empty_raw: [*]u8 = @alloc(1);
            let empty_out = unsafe { empty_raw[0..1] };
            empty_out[0] = 0x00;
            return empty_out;
        }
        let strm = PerMessageDeflate.newZeroZStream();
        PerMessageDeflate.initDeflateRawStream(&strm, level, window_bits)?;
        defer * = NativeZlib.deflateEnd(&strm);
        return PerMessageDeflate.compressOnStream(&strm, payload);
    }

    /// Stateless RFC 7692 §7.2.2 payload decompression: restores trailing `0x00 0x00 0xFF 0xFF`
    /// and inflates raw deflate data (compatible with both sync-flush and BFINAL=1 `deflateRaw` payloads).
    pub static fn decompressPayload(
        compressed: []const u8,
        max_bytes: usize = 16777216,
        window_bits: u8 = 15,
    ): ([]u8)! {
        if (compressed.len == 0) {
            return err.InflateFailed;
        }
        let strm = PerMessageDeflate.newZeroZStream();
        PerMessageDeflate.initInflateRawStream(&strm, window_bits)?;
        defer * = NativeZlib.inflateEnd(&strm);
        const res = PerMessageDeflate.decompressOnStream(&strm, compressed, max_bytes) catch e => {
            if (e == @as(anyerror, error.MessageTooBig)) return e;
            // Fallback to cap:pack inflateRaw for non-sync-flushed BFINAL=1 streams if needed
            if (raw_ok := inflateRaw(compressed)) {
                if (raw_ok.len > max_bytes) return err.MessageTooBig;
                return raw_ok;
            }
            return e;
        };
        return res;
    }

    /// Also provides a `deflateRaw`-backed helper that strips `0x00 0x00 0xFF 0xFF` if present.
    pub static fn compressRawNoContext(payload: []const u8, level: i32 = 6): []u8 {
        const raw = deflateRaw(payload, level);
        return PerMessageDeflate.stripSyncTrailer(raw);
    }

    /// Initializes a stateful `PerMessageDeflate` codec supporting context takeover / no-context-takeover.
    /// Underlying zlib streams are bound lazily on `*PerMessageDeflate` so `z_stream` addresses never move.
    pub static fn init(config: PerMessageDeflateConfig): PerMessageDeflate! {
        if (config.client_max_window_bits < 8 || config.client_max_window_bits > 15) {
            return err.InvalidWindowBits;
        }
        if (config.server_max_window_bits < 8 || config.server_max_window_bits > 15) {
            return err.InvalidWindowBits;
        }
        return PerMessageDeflate{
            config: config,
            comp_strm: PerMessageDeflate.newZeroZStream(),
            decomp_strm: PerMessageDeflate.newZeroZStream(),
            comp_active: false,
            decomp_active: false,
        };
    }

    fn ensureCompStream(*_): ! {
        if (_.comp_active) return;
        PerMessageDeflate.initDeflateRawStream(&_.comp_strm, _.config.compression_level, _.config.client_max_window_bits)?;
        _.comp_active = true;
    }

    fn ensureDecompStream(*_): ! {
        if (_.decomp_active) return;
        PerMessageDeflate.initInflateRawStream(&_.decomp_strm, _.config.server_max_window_bits)?;
        _.decomp_active = true;
    }

    pub fn deinit(*_) {
        if (_.comp_active) {
            * = NativeZlib.deflateEnd(&_.comp_strm);
            _.comp_active = false;
        }
        if (_.decomp_active) {
            * = NativeZlib.inflateEnd(&_.decomp_strm);
            _.decomp_active = false;
        }
    }

    /// Compresses a message on the stateful session stream, resetting LZ77 window when `no_context_takeover` is active.
    pub fn compress(*_, payload: []const u8): ([]u8)! {
        return _.compressWithReset(payload, false);
    }

    pub fn compressWithReset(*_, payload: []const u8, reset_after: bool): ([]u8)! {
        _.ensureCompStream()?;
        const out = PerMessageDeflate.compressOnStream(&_.comp_strm, payload)?;
        if (reset_after || _.config.client_no_context_takeover) {
            * = NativeZlib.deflateReset(&_.comp_strm);
        }
        return out;
    }

    /// Decompresses a message on the stateful session stream, resetting LZ77 window when `no_context_takeover` is active.
    pub fn decompress(*_, compressed: []const u8): ([]u8)! {
        return _.decompressWithReset(compressed, false);
    }

    pub fn decompressWithReset(*_, compressed: []const u8, reset_after: bool): ([]u8)! {
        _.ensureDecompStream()?;
        const out = PerMessageDeflate.decompressOnStream(&_.decomp_strm, compressed, _.config.max_decompressed_bytes)?;
        if (reset_after || _.config.server_no_context_takeover) {
            * = NativeZlib.inflateReset(&_.decomp_strm);
        }
        return out;
    }
}
