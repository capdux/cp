// Type definitions for CP

export class XnvDecoded {
    cp: u32,
    len: usize,
    ok: bool,
}

export fn xnvDecodeUtf8(s: string, pos: usize): XnvDecoded;
export fn xnvValidateName(s: string, allow_colon: bool): bool;
export fn isXmlChar(cp: u32): bool;
export fn isNameStartChar(cp: u32): bool;
export fn isNameChar(cp: u32): bool;
export fn isValidXmlName(s: string): bool;
export fn isValidNCName(s: string): bool;
export fn isValidQName(s: string): bool;
