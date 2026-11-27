// Type definitions for CP

export fn cp_ogg_stream_init(serialno: i32): *auto!?;
export fn cp_ogg_stream_packetin(os: *auto!?, data: [*]&u8!, len: isize, bos: i32, eos: i32, granulepos: i64, packetno: i64): i32;
export fn cp_ogg_stream_pageout(os: *auto!?, out_buf: [*]u8!, out_cap: usize, out_len: *usize!): i32;
export fn cp_ogg_stream_flush(os: *auto!?, out_buf: [*]u8!, out_cap: usize, out_len: *usize!): i32;
export fn cp_ogg_stream_clear(os: *auto!?);
export error OggError { InitFailed, InvalidState, PacketFailed, BufferTooSmall, PageFailed }

export class OggStream {
    raw: *auto!?,
    serialno: i32,
    fn init(serialno: i32): !OggStream;
    fn packetIn(data: &[]u8, bos: bool, eos: bool, granulepos: i64, packetno: i64): !void;
    fn pageOut(out_buf: []u8): !usize;
    fn flush(out_buf: []u8): !usize;
    fn __cp_deinit(_: &mut OggStream);
}

