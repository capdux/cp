// @cap/ogg — Xiph libogg 1.3.5 bitstream container framing engine for CP
// License: BSD-3-Clause (Upstream: xiph/ogg v1.3.5)

pub @c {
    fn cp_ogg_stream_init(serialno: i32): *!?;
    fn cp_ogg_stream_packetin(os: *!?, data: [*]const u8!, len: isize, bos: i32, eos: i32, granulepos: i64, packetno: i64): i32;
    fn cp_ogg_stream_pageout(os: *!?, out_buf: [*]u8!, out_cap: usize, out_len: *usize!): i32;
    fn cp_ogg_stream_flush(os: *!?, out_buf: [*]u8!, out_cap: usize, out_len: *usize!): i32;
    fn cp_ogg_stream_clear(os: *!?);
}

pub err OggError {
    InitFailed,
    InvalidState,
    PacketFailed,
    BufferTooSmall,
    PageFailed,
}

/// Owned RAII Ogg logical bitstream muxer. Automatically cleared on scope exit.
pub struct OggStream {
    raw: *!? = null,
    pub serialno: i32 = 0,

    pub static fn init(serialno: i32): OggStream! {
        let handle: *!? = null;
        unsafe {
            handle = cp_ogg_stream_init(serialno);
        }
        if (handle == null) return OggError.InitFailed;
        return OggStream{
            .raw = handle,
            .serialno = serialno,
        };
    }

    pub fn packetIn(data: []const u8, bos: bool, eos: bool, granulepos: i64, packetno: i64): void! {
        if (_.raw == null) return OggError.InvalidState;
        const bos_i: i32 = if (bos) 1 else 0;
        const eos_i: i32 = if (eos) 1 else 0;
        let rc: i32 = 0;
        unsafe {
            rc = cp_ogg_stream_packetin(_.raw, data.ptr, @intCast(data.len), bos_i, eos_i, granulepos, packetno);
        }
        if (rc != 0) return OggError.PacketFailed;
    }

    /// Emits a complete Ogg page into `out_buf` if enough packet data is buffered.
    /// Returns 0 when no page is ready yet, or the written page length in bytes.
    pub fn pageOut(out_buf: []u8): usize! {
        if (_.raw == null) return OggError.InvalidState;
        let written: usize = 0;
        let rc: i32 = 0;
        unsafe {
            rc = cp_ogg_stream_pageout(_.raw, out_buf.ptr, out_buf.len, &written);
        }
        if (rc == 0) return 0;
        if (rc == -2) return OggError.BufferTooSmall;
        if (rc < 0) return OggError.PageFailed;
        return written;
    }

    /// Forces remaining buffered packets into an Ogg page in `out_buf`.
    /// Returns 0 when no data remains to flush, or the written page length in bytes.
    pub fn flush(out_buf: []u8): usize! {
        if (_.raw == null) return OggError.InvalidState;
        let written: usize = 0;
        let rc: i32 = 0;
        unsafe {
            rc = cp_ogg_stream_flush(_.raw, out_buf.ptr, out_buf.len, &written);
        }
        if (rc == 0) return 0;
        if (rc == -2) return OggError.BufferTooSmall;
        if (rc < 0) return OggError.PageFailed;
        return written;
    }

    ~init() {
        if (p := _.raw) {
            _.raw = null;
            unsafe { cp_ogg_stream_clear(p); }
        }
    }
}
