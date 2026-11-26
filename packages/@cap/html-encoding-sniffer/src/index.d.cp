// Type definitions for CP

export class SniffOptions {
    transport_layer_encoding: string,
    default_encoding: string,
}

export fn hesToLower(ch: u8): u8;
export fn hesIsAsciiWs(ch: u8): bool;
export fn hesTrimAsciiWs(s: string): string;
export fn hesEqIgnoreCase(a: string, b: string): bool;
export fn hesStartsWithIgnoreCase(bytes: &[]u8, pos: usize, needle: string): bool;
export fn normalizeEncodingLabel(label: string): string;
export fn hesMapMetaEncoding(label: string): string;
export fn hesExtractCharsetFromContent(content: string): string;
export fn hesPrescanMeta(bytes: &[]u8): string;
export fn sniffHtmlEncoding(bytes: &[]u8, opts: SniffOptions): string;
