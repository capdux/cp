// Type definitions for CP

import { trim, eqIgnoreCase, findByte } from "cap:ascii";
import { eqAscii, startsWithIgnoreCase } from "../../unicase/src/index.cp";
import { NEG_MAX_SPECS, SPEC_WILDCARD, SPEC_PARTIAL, SPEC_EXACT, NegSpec, parseAcceptList, bestEncoding, bestMediaType, bestLanguage } from "./core.cp";
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
