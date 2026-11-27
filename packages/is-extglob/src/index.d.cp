// Type definitions for CP

export enum ExtglobKind {
    Optional,
    ZeroOrMore,
    OneOrMore,
    ExactlyOne,
    Negated,
}

export struct ExtglobSpan {
    kind: ExtglobKind,
    start: usize,
    end: usize,
}

export fn isExtglobPrefix(ch: u8): bool;
export fn extglobKindFromByte(ch: u8): ExtglobKind?;
export fn extglobPrefixByte(kind: ExtglobKind): u8;
export fn matchExtglobEnd(str: string, start: usize): usize;
export fn findExtglobFrom(str: string, start_pos: usize): ExtglobSpan?;
export fn findExtglob(str: string): ExtglobSpan?;
export fn findExtglobStart(str: string): i64;
export fn isExtglob(str: string): bool;
export fn hasNestedExtglob(str: string): bool;
export fn isNegatedExtglob(str: string): bool;
export fn countExtglobs(str: string): usize;
