// Type definitions for CP

export class TraceParent {
    version: u8,
    trace_id: [16]u8,
    parent_id: [8]u8,
    trace_flags: u8,
    valid: bool,
    fn isSampled(): bool;
    fn format(dst: []u8): i64;
}

export fn hexValLower(c: u8): i32;
export fn parseHexByteLower(hi: u8, lo: u8): i32;
export fn invalidTraceParent(): TraceParent;
export fn parseTraceParent(header: &[]u8): TraceParent;
export fn makeChild(parent: TraceParent, new_span_id: [8]u8): TraceParent;
export fn getTraceState(header: &[]u8, vendor_key: &[]u8): &[]u8;
