// Type definitions for CP

import { eqIgnoreCase, isAlnum, hexCharUpper, findByte } from "cap:ascii";
import { writeStr } from "cap:fmt";
export class MailtoUri {
    to: string,
    cc: string,
    bcc: string,
    subject: string,
    body: string,
    valid: bool,
}

export fn invalidMailto(): MailtoUri;
export fn mailtoIsSpace(b: u8): bool;
export fn trimSlice(s: string): string;
export fn parseMailto(uri: string): MailtoUri;
export fn countRecipients(to_list: string): usize;
export fn isBasicEmail(addr: string): bool;
export fn isUnreserved(b: u8): bool;
export fn pushSlice(dst: []u8, pos: usize, src: string): i64;
export fn pushPctEncoded(dst: []u8, pos: usize, src: string, allow_addr_chars: bool): i64;
export fn formatMailto(to: string, subject: string, body: string, cc: string, out: []u8): usize;
