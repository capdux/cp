// Type definitions for CP

export class SyslogMessage {
    facility: u8,
    severity: u8,
    version: u8,
    ts_start: usize,
    ts_len: usize,
    host_start: usize,
    host_len: usize,
    app_start: usize,
    app_len: usize,
    proc_start: usize,
    proc_len: usize,
    msgid_start: usize,
    msgid_len: usize,
    msg_start: usize,
    msg_len: usize,
    valid: bool,
}

export fn encodePri(facility: u8, severity: u8): u8;
export fn decodeFacility(pri: u8): u8;
export fn decodeSeverity(pri: u8): u8;
export class TokenSpan {
    start: usize,
    len: usize,
    next_pos: usize,
    ok: bool,
}

export fn readSpToken(raw: &[]u8, pos: usize): TokenSpan;
export fn parseRfc5424(raw: &[]u8): SyslogMessage;
export fn appendSlice(dst: []u8, pos: usize, s: &[]u8): i64;
export fn appendFieldOrNil(dst: []u8, pos: usize, field: &[]u8): i64;
export fn appendU8Dec(dst: []u8, pos: usize, val: u8): i64;
export fn formatRfc5424(facility: u8, severity: u8, timestamp: &[]u8, host: &[]u8, app: &[]u8, proc_id: &[]u8, msg_id: &[]u8, msg: &[]u8, dst: []u8): i64;
