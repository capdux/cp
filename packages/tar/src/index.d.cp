// Type definitions for CP

export class TarHeader {
    name: string,
    mode: u32,
    uid: u32,
    gid: u32,
    size: u64,
    mtime: u64,
    typeflag: u8,
    checksum: u32,
    is_ustar: bool,
    valid: bool,
}

export class TarEntrySpan {
    header: TarHeader,
    data_offset: usize,
    data_len: usize,
}

export fn tarEmptyHeader(): TarHeader;
export fn tarWriteOctal(out: []u8, offset: usize, digits: usize, val: u64): bool;
export fn tarParseOctal(buf: []u8, offset: usize, width: usize, require_digit: bool): u64?;
export fn tarComputeChecksum(block512: []u8): u32;
export fn tarIsUstarMagic(block512: []u8): bool;
export fn isZeroBlock(block512: []u8): bool;
export fn tarPaddedSize(raw_size: u64): u64;
export fn encodeTarHeader(name: string, size: u64, mode: u32, mtime: u64, typeflag: u8, out512: []u8): bool;
export fn parseTarHeader(block512: []u8): TarHeader;
export class TarArchiveCursor {
    offset: usize,
    finished: bool,
    fn init(): TarArchiveCursor;
    fn reset(_: auto);
    fn nextEntry(_: auto, archive: []u8): TarEntrySpan?;
}

