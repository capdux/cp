// Type definitions for CP

import { isValidSpdxExpression } from "../../spdx-expression-parse/src/index.cp";
export class LicenseValidation {
    valid_for_new_packages: bool,
    valid_for_old_packages: bool,
    spdx: bool,
    unlicensed: bool,
    in_file: string,
}

export fn vnlStrEq(a: string, b: string): bool;
export fn vnlStartsWith(s: string, prefix: string): bool;
export fn vnlContainsWhitespace(s: string): bool;
export fn vnlIsLegacyLicense(license: string): bool;
export fn validateLicense(license: string): LicenseValidation;
export fn isValidNewLicense(license: string): bool;
