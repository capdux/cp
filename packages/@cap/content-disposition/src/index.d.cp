// Type definitions for CP

import { AsciiSet, componentSet, percentEncode, percentDecode } from "../../percent-encoding/src/index.cp";
import { eqAscii } from "../../unicase/src/index.cp";
export fn cdspIsOws(b: u8): bool;
export fn cdspIsTokenChar(b: u8): bool;
export fn cdspSkipOws(s: string, start: usize): usize;
export fn cdspHasNonAscii(s: string): bool;
export fn cdspWriteByte(out: []u8, pos: usize, b: u8): usize;
export fn cdspWriteStr(out: []u8, pos: usize, s: string): usize;
export fn cdspWriteAsciiFallback(out: []u8, pos: usize, s: string): usize;
export fn cdspRfc5987Set(): AsciiSet;
export fn cdspIsControl(b: u8): bool;
export class ContentDisposition {
    valid: bool,
    disp_type: string,
    filename: string,
    filename_star: string,
    fn invalid(): ContentDisposition;
    fn isAttachment(): bool;
    fn isInline(): bool;
    fn hasFilename(): bool;
}

export fn parseContentDisposition(header: string): ContentDisposition;
export fn sanitizeFilename(raw: string, out: []u8): usize;
export fn decodeFilenameStar(raw_star: string, out: []u8): usize;
export fn formatContentDisposition(disp_type: string, filename: string, out: []u8): usize;
