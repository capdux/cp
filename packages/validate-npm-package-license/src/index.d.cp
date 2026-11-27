// Type definitions for CP

import { strEq, startsWith, isSpace } from "cap:ascii";
import { isValidSpdxExpression, parseSpdxExpression, SpdxAst, SPDX_NODE_LICENSE, SPDX_NODE_AND, SPDX_NODE_OR } from "../../spdx-expression-parse/src/index.cp";
import { lookupLicense } from "../../spdx/src/index.cp";
export class LicenseValidation {
    valid_for_new_packages: bool,
    valid_for_old_packages: bool,
    spdx: bool,
    unlicensed: bool,
    in_file: string,
}

export fn vnlContainsWhitespace(s: string): bool;
export fn vnlIsLegacyLicense(license: string): bool;
export fn validateLicense(license: string): LicenseValidation;
export fn isValidNewLicense(license: string): bool;
export fn isValidOldLicense(license: string): bool;
export fn isOsiApprovedLicense(license_id: string): bool;
export fn isFsfLibreLicense(license_id: string): bool;
export fn vnlEvalOsiAst(ast: SpdxAst, idx: i32, depth: usize): bool;
export fn vnlEvalFsfAst(ast: SpdxAst, idx: i32, depth: usize): bool;
export fn isOsiApprovedExpression(expr: string): bool;
export fn isFsfLibreExpression(expr: string): bool;
export class LicenseDetails {
    valid_for_new_packages: bool,
    valid_for_old_packages: bool,
    spdx: bool,
    unlicensed: bool,
    in_file: string,
    osi_approved: bool,
    fsf_libre: bool,
    has_compound_expr: bool,
    has_exception: bool,
}

export fn inspectLicense(license: string): LicenseDetails;
