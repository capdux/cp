// Type definitions for CP

export error BytesError { EmptyInput, InvalidFormat, UnknownUnit, Overflow, BufferTooSmall }

export enum ByteUnitMode {
    BinaryIec,
    DecimalSi
}

export class FormatOptions {
    mode: ByteUnitMode,
    decimals: u8,
    space: bool,
}

export fn isAsciiSpace(b: u8): bool;
export fn isAsciiDigit(b: u8): bool;
export fn toAsciiLower(b: u8): u8;
export fn sliceEqlLower(a: &[]u8, b: string): bool;
export fn resolveUnitMultiplier(unit: &[]u8, si_default: bool): u64 ! BytesError;
export fn parseBytesCore(input: &[]u8, si_default: bool): u64 ! BytesError;
export fn parseBytes(input: &[]u8): u64 ! BytesError;
export fn parseBytesBinary(input: &[]u8): u64 ! BytesError;
export fn parseBytesSi(input: &[]u8): u64 ! BytesError;
export fn pow10U64(exp: u8): u64;
export fn unitDivisor(idx: usize, is_si: bool): u64;
export fn unitLabel(idx: usize, is_si: bool): string;
export fn appendBytes(out: []u8, pos: usize, src: &[]u8): usize ! BytesError;
export fn appendU64(out: []u8, pos: usize, value: u64): usize ! BytesError;
export fn formatBytes(n: u64, opts: FormatOptions, out: []u8): &[]u8 ! BytesError;
export fn formatBinary(n: u64, out: []u8): &[]u8 ! BytesError;
export fn formatDecimal(n: u64, out: []u8): &[]u8 ! BytesError;
export fn formatCompact(n: u64, out: []u8): &[]u8 ! BytesError;
