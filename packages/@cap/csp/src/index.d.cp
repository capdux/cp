// Type definitions for CP

export fn toLowerAscii(c: u8): u8;
export fn isAsciiSpace(c: u8): bool;
export fn eqIgnoreCase(a: &[]u8, b: &[]u8): bool;
export fn isValidDirectiveName(name: &[]u8): bool;
export fn isValidSourceValue(src: &[]u8): bool;
export class CspBuilder {
    buf: []u8,
    pos: usize,
    count: usize,
    overflow: bool,
    fn init(buf: []u8): CspBuilder;
    fn addDirective(_: auto, name: &[]u8, sources: &[]&[]u8): bool;
    fn written(): &[]u8;
    fn len(): usize;
    fn isOverflow(): bool;
}

export class CspDirectiveSpan {
    name_start: usize,
    name_len: usize,
    val_start: usize,
    val_len: usize,
}

export class CspPolicy {
    directives: [32]CspDirectiveSpan,
    count: usize,
    fn parse(header: &[]u8): CspPolicy;
    fn len(): usize;
    fn getDirective(header: &[]u8, name: &[]u8): &[]u8;
    fn hasDirective(header: &[]u8, name: &[]u8): bool;
    fn allowsUnsafeInline(header: &[]u8, directive: &[]u8): bool;
}

