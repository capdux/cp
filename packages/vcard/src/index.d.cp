// Type definitions for CP

import { eqIgnoreCase } from "cap:ascii";
export fn isSpaceOrTab(c: u8): bool;
export class VCardData {
    full_name: &[]u8,
    org: &[]u8,
    title: &[]u8,
    email: &[]u8,
    tel: &[]u8,
    url: &[]u8,
}

export class ParsedVCard {
    fn_start: usize,
    fn_len: usize,
    org_start: usize,
    org_len: usize,
    title_start: usize,
    title_len: usize,
    email_start: usize,
    email_len: usize,
    tel_start: usize,
    tel_len: usize,
    url_start: usize,
    url_len: usize,
    version_start: usize,
    version_len: usize,
    valid: bool,
}

export fn writeRaw(dst: []u8, pos: &mut usize, src: &[]u8): bool;
export fn writeEscaped(dst: []u8, pos: &mut usize, src: &[]u8): bool;
export fn writeProp(dst: []u8, pos: &mut usize, propPrefix: &[]u8, val: &[]u8): bool;
export fn formatVCard(card: VCardData, dst: []u8): i64;
export fn parseVCard(vcf: &[]u8): ParsedVCard;
