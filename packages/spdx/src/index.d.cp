// Type definitions for CP

import { strEq, startsWith, isSpace } from "cap:ascii";
export enum LicenseCategory {
    Permissive,
    WeakCopyleft,
    StrongCopyleft,
    NetworkCopyleft,
    PublicDomain,
    Unknown
}

export class LicenseInfo {
    id: string,
    category: LicenseCategory,
    isOsiApproved: bool,
}

export class SpdxResult {
    valid: bool,
    allKnown: bool,
    errorOffset: usize,
    licenses: [MAX_LICENSES]string,
    licenseCount: usize,
    fn getLicense(idx: usize): string?;
}

export fn isIdChar(c: u8): bool;
export fn isPermissiveCategory(cat: LicenseCategory): bool;
export fn lookupBaseLicense(id: string): LicenseInfo?;
export fn lookupLicense(id: string): LicenseInfo?;
export enum EvalMode {
    ValidateOnly,
    PermissiveOnly,
    Satisfies
}

export class Parser {
    src: string,
    pos: usize,
    depth: usize,
    failed: bool,
    errorOffset: usize,
    allKnown: bool,
    licenses: [MAX_LICENSES]string,
    licenseCount: usize,
    mode: EvalMode,
    allowed: &[]string,
}

export fn initParser(src: string, mode: EvalMode, allowed: &[]string): Parser;
export fn skipSpaces(p: &mut Parser);
export fn failAt(p: &mut Parser, offset: usize): bool;
export fn isKeyword(tok: string): bool;
export fn peekKeyword(p: &mut Parser, kw: string): bool;
export fn consumeKeyword(p: &mut Parser, kw: string): bool;
export fn recordLicense(p: &mut Parser, lic: string);
export fn isValidRefId(s: string): bool;
export fn licenseTokenMatches(licBase: string, hasPlus: bool, allowedLic: string): bool;
export fn matchAllowedEntry(licBase: string, hasPlus: bool, hasExc: bool, exc: string, entry: string): bool;
export fn evalSimple(p: &mut Parser, licBase: string, hasPlus: bool, hasExc: bool, exc: string, fullLic: string): bool;
export fn parseSimple(p: &mut Parser): bool;
export fn parsePrimary(p: &mut Parser): bool;
export fn parseAnd(p: &mut Parser): bool;
export fn parseOr(p: &mut Parser): bool;
export fn runParse(p: &mut Parser): bool;
export fn validateExpression(expr: string): SpdxResult;
export fn isPermissiveOnly(expr: string): bool;
export fn satisfies(expr: string, allowed: &[]string): bool;
