// Type definitions for CP

import { isDigit, isAlpha, isAlnum, isHexDigit, toLowerByte, eqIgnoreCase, strEq, isAscii, startsWith, endsWith } from "cap:ascii";
import { indexOfBytes } from "cap:bin";
export class ValidationError {
    field: string,
    code: string,
    message: string,
}

export fn parseUintDec(s: string, start: usize, end: usize): i64;
export fn isAlphanumeric(s: string): bool;
export fn isNumeric(s: string): bool;
export fn isHex(s: string): bool;
export fn isBase64(s: string): bool;
export fn isSlug(s: string): bool;
export fn hasMinLen(s: string, min_len: usize): bool;
export fn hasMaxLen(s: string, max_len: usize): bool;
export fn hasLenBetween(s: string, min_len: usize, max_len: usize): bool;
export fn containsStr(s: string, needle: string): bool;
export fn isOneOf(s: string, allowed: []string): bool;
export fn isValidDomainLabel(s: string, start: usize, end: usize): bool;
export fn isValidDomain(s: string, start: usize, end: usize): bool;
export fn isEmail(s: string): bool;
export fn isIpv4Range(s: string, start: usize, end: usize): bool;
export fn isIpv4(s: string): bool;
export fn isIpv6Range(s: string, start: usize, end: usize): bool;
export fn isIpv6(s: string): bool;
export fn isCidr(s: string): bool;
export fn isUrl(s: string): bool;
export fn isUuid(s: string): bool;
export fn isCrockfordBase32(c: u8): bool;
export fn isUlid(s: string): bool;
export fn isSemver(s: string): bool;
export fn isLeapYear(y: i64): bool;
export fn daysInMonth(y: i64, m: i64): i64;
export fn isIsoDate(s: string): bool;
export fn isIsoDateTime(s: string): bool;
export fn isLuhn(s: string): bool;
export fn isE164Phone(s: string): bool;
export fn isPort(p: i64): bool;
export fn inRangeI64(v: i64, min_v: i64, max_v: i64): bool;
export fn inRangeF64(v: f64, min_v: f64, max_v: f64): bool;
export fn isMultipleOf(v: i64, step: i64): bool;
export class Validator {
    errors: [MAX_VALIDATION_ERRORS]ValidationError,
    count: usize,
    fn init(): Validator;
    fn addError(_: auto, field: string, code: string, message: string);
    fn isValid(): bool;
    fn errorCount(): usize;
    fn errorAt(idx: usize): ValidationError;
    fn required(_: auto, field: string, value: string): bool;
    fn minLen(_: auto, field: string, value: string, n: usize): bool;
    fn maxLen(_: auto, field: string, value: string, n: usize): bool;
    fn email(_: auto, field: string, value: string): bool;
    fn url(_: auto, field: string, value: string): bool;
    fn uuid(_: auto, field: string, value: string): bool;
    fn ulid(_: auto, field: string, value: string): bool;
    fn ipv4(_: auto, field: string, value: string): bool;
    fn ipv6(_: auto, field: string, value: string): bool;
    fn cidr(_: auto, field: string, value: string): bool;
    fn semver(_: auto, field: string, value: string): bool;
    fn isoDate(_: auto, field: string, value: string): bool;
    fn isoDateTime(_: auto, field: string, value: string): bool;
    fn luhn(_: auto, field: string, value: string): bool;
    fn phone(_: auto, field: string, value: string): bool;
    fn slug(_: auto, field: string, value: string): bool;
    fn oneOf(_: auto, field: string, value: string, allowed: []string): bool;
    fn rangeI64(_: auto, field: string, value: i64, min_v: i64, max_v: i64): bool;
    fn rangeF64(_: auto, field: string, value: f64, min_v: f64, max_v: f64): bool;
    fn port(_: auto, field: string, value: i64): bool;
}

