// Type definitions for CP

import { eqIgnoreCase, trim, findByte } from "cap:ascii";
import { parseI64 } from "cap:fmt";
export class ParsedCookie {
    name: string,
    value: string,
    path: string,
    domain: string,
    expires: string,
    same_site: string,
    max_age: i64,
    has_max_age: bool,
    secure: bool,
    http_only: bool,
    partitioned: bool,
}

export fn scpStripQuotes(s: string): string;
export fn parseSetCookie(header: string): ParsedCookie;
export fn splitCookiesCount(combined: string): usize;
export fn splitCookiesGet(combined: string, target_idx: usize): string?;
