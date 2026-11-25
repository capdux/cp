// Type definitions for CP

export error MimeError { BufferTooSmall, InvalidMime }

export class MimeInfo {
    mime: &[]u8,
    default_ext: &[]u8,
    compressible: bool,
    utf8_charset: bool,
}

export class ExtEntry {
    ext: &[]u8,
    info: MimeInfo,
}

export class MimeAlias {
    alias: &[]u8,
    info: MimeInfo,
}

export fn isAsciiSpace(b: u8): bool;
export fn toAsciiLower(b: u8): u8;
export fn isTokenChar(b: u8): bool;
export fn trimAscii(s: &[]u8): &[]u8;
export fn eqlIgnoreCase(a: &[]u8, b: &[]u8): bool;
export fn startsWithIgnoreCase(s: &[]u8, prefix: &[]u8): bool;
export fn endsWithIgnoreCase(s: &[]u8, suffix: &[]u8): bool;
export fn findByte(s: &[]u8, target: u8): usize?;
export fn extractExtension(ext_or_path: &[]u8): (&[]u8)?;
export fn stripMimeParams(mime: &[]u8): &[]u8;
export fn isValidMimeEssence(essence: &[]u8): bool;
export fn isKnownTopLevelType(t: &[]u8): bool;
export fn hasCharsetParam(params: &[]u8): bool;
export fn copySlice(dst: []u8, offset: usize, src: &[]u8): usize ! MimeError;
export fn lookupInfo(ext_or_path: &[]u8): MimeInfo?;
export fn lookupInfoByMime(mime: &[]u8): MimeInfo?;
export fn lookupByExt(ext_or_path: &[]u8): (&[]u8)?;
export fn lookupOrOctetStream(ext_or_path: &[]u8): &[]u8;
export fn extensionForMime(mime: &[]u8): (&[]u8)?;
export fn defaultCharset(mime: &[]u8): (&[]u8)?;
export fn isCompressible(mime: &[]u8): bool;
export fn contentTypeHeader(ext_or_mime: &[]u8, out: []u8): &[]u8 ! MimeError;
