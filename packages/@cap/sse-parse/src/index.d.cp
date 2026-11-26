// Type definitions for CP

export fn sseBytesEq(a: &[]u8, b: &[]u8): bool;
export class SseParser {
    data_buf: [2048]u8,
    data_len: usize,
    event_buf: [128]u8,
    event_len: usize,
    id_buf: [128]u8,
    id_len: usize,
    line_buf: [512]u8,
    line_len: usize,
    has_data: bool,
    retry_ms: i64,
    pending_cr: bool,
    fn init(): SseParser;
    fn processLine(_: auto): i32;
    fn pushByte(_: auto, b: u8): i32;
    fn eventType(_: auto): &[]u8;
    fn eventData(_: auto): &[]u8;
    fn lastEventId(_: auto): &[]u8;
    fn retryMs(): i64;
    fn consumeEvent(_: auto);
    fn reset(_: auto);
}

