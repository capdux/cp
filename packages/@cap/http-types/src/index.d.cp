// Type definitions for CP

import { eqAscii } from "../../unicase/src/index.cp";
export fn htpStrEq(a: string, b: string): bool;
export fn htpIsTchar(b: u8): bool;
export fn parseMethod(s: string): u8;
export fn methodAsStr(m: u8): string;
export fn isSafeMethod(m: u8): bool;
export fn isIdempotentMethod(m: u8): bool;
export class StatusCode {
    code: u16,
    fn fromU16(code: u16): StatusCode?;
    fn asU16(): u16;
    fn canonicalReason(): string;
    fn isInformational(): bool;
    fn isSuccess(): bool;
    fn isRedirection(): bool;
    fn isClientError(): bool;
    fn isServerError(): bool;
}

export fn parseHttpVersion(s: string): u8;
export fn httpVersionAsStr(v: u8): string;
export fn isValidHeaderName(name: string): bool;
export fn headerNameEq(a: string, b: string): bool;
