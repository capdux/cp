// Type definitions for CP

import { eqAscii, startsWithIgnoreCase } from "../../unicase/src/index.cp";
export class QualitySpec {
    value: string,
    q_milli: u32,
    order: usize,
}

export fn negIsWs(b: u8): bool;
export fn negTrim(s: string): string;
export fn parseQValue(raw: string): u32;
export fn parseWeightedList(header: string, out: []QualitySpec): usize;
export fn negTokenSpecificity(spec: string, candidate: string, is_lang: bool): u32;
export fn negNegotiateToken(header: string, available: &[]string, is_lang: bool): string?;
export fn negotiateEncoding(accept_encoding: string, available: &[]string): string?;
export fn negotiateLanguage(accept_language: string, available: &[]string): string?;
export fn negotiateCharset(accept_charset: string, available: &[]string): string?;
