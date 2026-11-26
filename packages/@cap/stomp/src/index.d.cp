// Type definitions for CP

export class StompHeader {
    key: string,
    value: string,
}

export class StompFrame {
    command: string,
    headers: [16]StompHeader,
    header_count: usize,
    body: string,
    consumed: usize,
    fn empty(): StompFrame;
}

export fn strEq(a: string, b: string): bool;
export fn escapeHeader(input: string, out: []u8): usize;
export fn unescapeHeader(input: string, out: []u8): usize;
export fn encodeFrame(command: string, headers: &[]StompHeader, body: string, out: []u8): usize;
export fn parseContentLength(val: string): i64;
export fn decodeFrame(data: string, out_frame: &mut StompFrame): i32;
export fn getHeader(frame: StompFrame, key: string): string;
