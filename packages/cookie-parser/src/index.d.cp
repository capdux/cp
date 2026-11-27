// Type definitions for CP

import { getCookieHeaderValue, cjTrim } from "../../cookie-jar/src/index.cp";
import { sign, unsign } from "../../cookie-signature/src/index.cp";
export class CookieEntry {
    name: string,
    value: string,
}

export fn cprStrEq(a: string, b: string): bool;
export fn cprStartsWith2(s: string, c0: u8, c1: u8): bool;
export fn isSignedCookie(val: string): bool;
export fn isJsonCookie(val: string): bool;
export fn stripJsonCookiePrefix(val: string): string?;
export fn signCookieValue(val: string, secret: string, out: []u8): usize;
export fn signedCookie(val: string, secret: string): string?;
export fn signedCookieWithSecrets(val: string, secrets: [4]string, secret_count: usize): string?;
export fn lookupRawCookie(header: string, name: string): string?;
export class ParsedCookies {
    cookies: [32]CookieEntry,
    cookie_count: usize,
    signed_cookies: [32]CookieEntry,
    signed_count: usize,
    fn empty(): ParsedCookies;
    fn get(name: string): string?;
    fn getSigned(name: string): string?;
    fn totalCount(): usize;
}

export fn cprParseInto(header: string, secrets: [4]string, secret_count: usize): ParsedCookies;
export fn parseCookieHeader(header: string, secret: string): ParsedCookies;
export fn parseCookieHeaderWithSecrets(header: string, secrets: [4]string, secret_count: usize): ParsedCookies;
