// Type definitions for CP

export class IsoDuration {
    negative: bool,
    years: u32,
    months: u32,
    weeks: u32,
    days: u32,
    hours: u32,
    minutes: u32,
    seconds: u32,
    millis: u32,
    valid: bool,
}

export fn invalidDuration(): IsoDuration;
export fn parseIsoDuration(src: &[]u8): IsoDuration;
export fn pushByte(dst: []u8, pos: usize, b: u8): i64;
export fn pushU32(dst: []u8, pos: usize, val: u32): i64;
export fn pushComp(dst: []u8, pos: usize, val: u32, unit: u8): i64;
export fn formatIsoDuration(d: IsoDuration, dst: []u8): i64;
export fn toApproxMillis(d: IsoDuration): i64;
