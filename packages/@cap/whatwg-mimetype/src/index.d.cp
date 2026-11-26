// Type definitions for CP

export class MimeParam {
    name: string,
    value: string,
}

export fn wmimeIsHttpWs(b: u8): bool;
export fn wmimeIsTokenChar(b: u8): bool;
export fn wmimeToLower(b: u8): u8;
export fn wmimeEqIgnoreCase(a: string, b: string): bool;
export fn wmimeEndsWithIgnoreCase(s: string, suffix: string): bool;
export fn wmimePush(out: []u8, pos: usize, s: string): usize;
export fn wmimePushLower(out: []u8, pos: usize, s: string): usize;
export fn wmimeEmptyParam(): MimeParam;
export class MimeType {
    valid: bool,
    main_type: string,
    subtype: string,
    params: [8]MimeParam,
    param_count: usize,
    fn paramCount(): usize;
    fn getParam(name: string): string?;
    fn essence(out: []u8): usize;
    fn isHTML(): bool;
    fn isXML(): bool;
    fn isJavaScript(): bool;
    fn isJSON(): bool;
    fn serialize(out: []u8): usize;
}

export fn parseMimeType(input: string): MimeType;
