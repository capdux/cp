// Type definitions for CP

import { isSpace, trim, eqIgnoreCase, hexCharUpper, hexVal, isAlnum, findByte } from "cap:ascii";
import { writeStr } from "cap:fmt";
export class DataUri {
    ok: bool,
    media_type: string,
    charset: string,
    is_base64: bool,
    raw_payload: string,
    fn invalid(): DataUri;
}

export fn appendByte(out: []u8, pos: usize, c: u8): usize;
export fn appendStr(out: []u8, pos: usize, s: string): usize;
export fn isUnreservedUriByte(c: u8): bool;
export fn parseDataUri(uri: string): DataUri;
export fn isDataUri(s: string): bool;
export fn b64Val(c: u8): u8?;
export fn decodeBase64Payload(payload: string, out: []u8): ([]u8)?;
export fn decodePercentPayload(payload: string, out: []u8): ([]u8)?;
export fn decodeDataUri(uri: string, out: []u8): ([]u8)?;
export fn encodeDataUriBase64(media_type: string, payload: []u8, out: []u8): string;
export fn encodeDataUriText(media_type: string, text: string, out: []u8): string;
